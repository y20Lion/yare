#pragma once

#include <GL/glew.h>
#include <memory>
#include <vector>

#include "tools.h"

struct ShaderDesc
{
    ShaderDesc(const std::string& code, GLenum type) : code(code), type(type) {}
    std::string code;
    GLenum type;
};

struct GLProgramDesc
{
    std::vector<ShaderDesc> shaders;
};

class GLProgram
{
public:
    GLProgram(const GLProgramDesc& desc);
    virtual ~GLProgram();
    GLuint id() const { return _program_id; }

private:
    GLuint _createAndCompileShader(const ShaderDesc& desc);
    void _linkProgram();

private:
    DISALLOW_COPY_AND_ASSIGN(GLProgram)
    GLuint _program_id;
};
DECLARE_STD_PTR(GLProgram)

GLProgramUptr createProgram(const std::string& vertex_shader_source, const std::string& fragment_shader_source);