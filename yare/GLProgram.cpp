#include "GLProgram.h"

#include <assert.h>
#include <fstream>
#include <sstream>
#include <regex>
#include <map>

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
      if (line.length() > 1 && line[0] == '~')
      {
         if (shader_type != 0)
            program_desc.shaders.push_back(ShaderDesc(shader_code.str(), shader_type));
         shader_code = std::stringstream();

         shader_type = _toShaderType(line);
      }
      else if (line.length() > 1 && line[0] == '#' && line.find("#include") != std::string::npos)
      {
         shader_code << _getIncludeFileContent(line) << std::endl;
      }
      else
         shader_code << line << std::endl;
   }
   program_desc.shaders.push_back(ShaderDesc(shader_code.str(), shader_type));
   return program_desc;
}

}