#include "GLProgram.h"

#include <assert.h>
#include <fstream>

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

}