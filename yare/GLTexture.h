#pragma once

#include <GL/glew.h>
#include "tools.h"

struct GLTexture2DDesc
{
    int width, height;
    int levels;
    GLenum internal_format;
};

class GLTexture
{
public:
    GLTexture();
    virtual ~GLTexture();
    GLuint id() const { return _texture_id; }

protected:
    DISALLOW_COPY_AND_ASSIGN(GLTexture)
    GLuint _texture_id;
};

class GLTexture2D : public GLTexture
{
public:
    GLTexture2D(const GLTexture2DDesc& desc);
    virtual ~GLTexture2D();
    
    void setData(int level, GLenum data_format, GLenum data_type, void* data);
private:
    int _width, _height;
    DISALLOW_COPY_AND_ASSIGN(GLTexture2D)
};