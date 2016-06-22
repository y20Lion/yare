#include "GLProgram.h"

#include <assert.h>
#include <fstream>
#include <sstream>
#include <regex>
#include <map>
#include <set>

#include "error.h"

namespace yare {

GLProgram::GLProgram(const GLProgramDesc& desc)
{
    _program_id = glCreateProgram();
    
    std::vector<GLuint> gl_shaders;
    for (const ShaderDesc& shader_desc : desc.shaders)
    {        
        GLuint shader = _createAndCompileShader(shader_desc);
        glAttachShader(_program_id, shader);
        gl_shaders.push_back(shader);
    }
    
    _linkProgram();

    for (const GLuint& shader : gl_shaders)
    {
        glDetachShader(_program_id, shader);
        glDeleteShader(shader);
    }
}

GLProgram::~GLProgram()
{
    glDeleteProgram(_program_id);
}

static void _writeShaderSourceToFile(GLuint shader)
{
    int count = 0;
    glGetShaderiv(shader, GL_SHADER_SOURCE_LENGTH, &count);
    std::string source_code;
    source_code.resize(count);

    int dummy = 0;
    glGetShaderSource(shader, count, &dummy, (GLchar*)source_code.data());

    std::ofstream myfile("D:\\shader_dump.txt");
    myfile << source_code;
    myfile.close();
}

GLuint GLProgram::_createAndCompileShader(const ShaderDesc& shader_desc)
{
   GLuint shader = glCreateShader(shader_desc.type);
   GLint length = (GLint)shader_desc.code.size();
   const GLchar* code_string = shader_desc.code.data();
   glShaderSource(shader, 1, &code_string, &length);
   glCompileShader(shader);
   GLint success = 0;
   glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

   //_writeShaderSourceToFile(shader);
   if (success == GL_FALSE)
   {
      GLint log_length;
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
      std::string error_log;
      error_log.resize(log_length);
      glGetShaderInfoLog(shader, log_length, &log_length, (GLchar*)error_log.data());
      _writeShaderSourceToFile(shader);
      RUNTIME_ERROR(error_log);
   }
   return shader;
}

void GLProgram::_linkProgram()
{
   glLinkProgram(_program_id);
   GLint success = 0;
   glGetProgramiv(_program_id, GL_LINK_STATUS, &success);
   if (success == GL_FALSE)
   {
      GLint log_length;
      glGetProgramiv(_program_id, GL_INFO_LOG_LENGTH, &log_length);
      std::string error_log;
      error_log.resize(log_length);
      glGetProgramInfoLog(_program_id, log_length, &log_length, (GLchar*)error_log.data());        
      RUNTIME_ERROR(error_log);
   }
}

Uptr<GLProgram> createProgram(const std::string& vertex_shader_source, const std::string& fragment_shader_source)
{    
   GLProgramDesc program_desc;
   program_desc.shaders.push_back(ShaderDesc(vertex_shader_source, GL_VERTEX_SHADER));
   program_desc.shaders.push_back(ShaderDesc(fragment_shader_source, GL_FRAGMENT_SHADER));
   return std::make_unique<GLProgram>(program_desc);
}

GLenum _toShaderType(const std::string& line)
{
   if (line.find("VertexShader") != std::string::npos)
      return GL_VERTEX_SHADER;
   else if (line.find("TessControlShader") != std::string::npos)
      return GL_TESS_CONTROL_SHADER;
   else if (line.find("TessEvaluationShader") != std::string::npos)
      return GL_TESS_EVALUATION_SHADER;
   else if (line.find("GeometryShader") != std::string::npos)
      return GL_GEOMETRY_SHADER;
   else if (line.find("FragmentShader") != std::string::npos)
      return GL_FRAGMENT_SHADER;
   else if (line.find("ComputeShader") != std::string::npos)
      return GL_COMPUTE_SHADER;
   else
   {
      assert(false);
      return 0;
   }
}

static std::string _getIncludeFileContent(const std::string& line)
{
   static std::map<std::string, std::string> _included_files;
   static std::regex include_regex("#include.+\"(.+)\".*");

   
   std::smatch regex_result;
   if (!std::regex_search(line, regex_result, include_regex))
      RUNTIME_ERROR("Wrong include directive");

   std::string include_name = regex_result[1];
   auto it = _included_files.find(include_name);
   if (it == _included_files.end())
   {
      std::ifstream file(include_name);
      if (!file.is_open())
         RUNTIME_ERROR("Include file not found");

      std::string include_content((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
      it = _included_files.insert(std::make_pair(include_name, include_content)).first;
   }

   return it->second;
}

struct IncludeCacheData
{
   std::vector<std::string> content;
   std::vector<std::string> includes;
};
static std::map<std::string, IncludeCacheData> _include_cache;
static std::regex _include_regex("#include.+\"(.+)\".*");


bool _getFilenameOfInclude(const std::string& line, std::string& include_name)
{
   std::smatch regex_result;
   if (!std::regex_search(line, regex_result, _include_regex))
      return false;

   include_name = regex_result[1];
   return true;
}

IncludeCacheData* _addIncludeFileContentToCache(const std::string& include_file_name)
{
   std::ifstream file(include_file_name);
   if (!file.is_open())
      RUNTIME_ERROR("Include file not found");

   IncludeCacheData* include_data = &_include_cache[include_file_name];
   include_data->content.push_back(std::string());

   std::string line;
   while (std::getline(file, line))
   {
      std::string include_name;
      if (line.length() > 1 && line[0] == '#' && _getFilenameOfInclude(line, include_name))
      {
         include_data->includes.push_back(include_name);
         include_data->content.push_back(std::string());
      }
      else
      {
         include_data->content.back() += line + "\n";
      }
   }

   return include_data;
}

static std::string _getIncludeFileContent(const std::string& include_file_name, std::set<std::string>& included_files_so_far)
{
   included_files_so_far.insert(include_file_name);

   IncludeCacheData* include_data = nullptr;
   auto it = _include_cache.find(include_file_name);
   if (it != _include_cache.end())
   {
      include_data = &it->second;
   }
   else
   {      
      include_data = _addIncludeFileContentToCache(include_file_name);
   }

   std::string result;
   for (int i = 0; i < include_data->includes.size(); ++i)
   {
      result += include_data->content[i];
      std::string& include_name = include_data->content[i];
      if (included_files_so_far.find(include_file_name) == included_files_so_far.end())
         result += _getIncludeFileContent(include_name, included_files_so_far);
   }
   result += include_data->content.back();

   return result;
}

Uptr<GLProgram> createProgramFromFile(const std::string& filepath)
{      
   auto program_desc = createProgramDescFromFile(filepath);
   return std::make_unique<GLProgram>(program_desc);
}

GLProgramDesc createProgramDescFromFile(const std::string& filepath)
{
   std::ifstream file(filepath);
   std::string line;
   std::stringstream shader_code;
   GLProgramDesc program_desc;
   GLenum shader_type = 0;

   while (std::getline(file, line))
   {
      std::string include_name;
      if (line.length() > 1 && line[0] == '~')
      {
         if (shader_type != 0)
            program_desc.shaders.push_back(ShaderDesc(shader_code.str(), shader_type));
         shader_code = std::stringstream();

         shader_type = _toShaderType(line);
      }
      else if (line.length() > 1 && line[0] == '#' && _getFilenameOfInclude(line, include_name))
      {
         shader_code << _getIncludeFileContent(include_name, std::set<std::string>()) << std::endl;
      }
      else
         shader_code << line << std::endl;
   }
   program_desc.shaders.push_back(ShaderDesc(shader_code.str(), shader_type));
   return program_desc;
}

}