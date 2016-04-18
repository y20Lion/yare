#include "GLTexture.h"

GLTexture::GLTexture()
{
    
}

GLTexture::~GLTexture()
{
    glDeleteTextures(1, &_texture_id);
}


GLTexture2D::GLTexture2D(const GLTexture2DDesc& desc)
    : _height(desc.height)
    , _width(desc.width)
{
    glCreateTextures(GL_TEXTURE_2D, 1, &_texture_id);
    glTextureStorage2D(_texture_id, desc.levels, desc.internal_format, desc.width, desc.height);
}

GLTexture2D::~GLTexture2D()
{
    
}
    
void GLTexture2D::setData(int level, GLenum data_format, GLenum data_type, void* data)
{
    glTextureSubImage2D(_texture_id, level, 0, 0, _width, _height, data_format, data_type, data);
}
