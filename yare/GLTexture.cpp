#include "GLTexture.h"

#include <assert.h>
#include <algorithm>

namespace yare {

static int _mipmapLevelCount(int width, int height)
{
    int dimension = std::max(width, height);
    int i = 0;
    while (dimension > 0)
    {
        dimension >>= 1;
        i++;
    }
    return i;
}

std::pair<GLenum, GLenum> _getExternalFormatAndType(GLenum internal_format, bool bgr)
{
    GLenum RGB = bgr ? GL_BGR : GL_RGB;
    GLenum RGBA = bgr ? GL_BGRA : GL_RGBA;
    
    switch (internal_format)
    {
        case GL_RGBA8: return std::make_pair(RGBA, GL_UNSIGNED_BYTE);
        case GL_RGB8: return std::make_pair(RGB, GL_UNSIGNED_BYTE);
        case GL_RG8: return std::make_pair(GL_RG, GL_UNSIGNED_BYTE);
        case GL_R8: return std::make_pair(GL_RED, GL_UNSIGNED_BYTE);

        case GL_RGBA16: return std::make_pair(RGBA, GL_UNSIGNED_SHORT);
        case GL_RGB16: return std::make_pair(RGB, GL_UNSIGNED_SHORT);
        case GL_RG16: return std::make_pair(GL_RG, GL_UNSIGNED_SHORT);
        case GL_R16: return std::make_pair(GL_RED, GL_UNSIGNED_SHORT);

        case GL_RGBA16F: return std::make_pair(RGBA, GL_FLOAT);
        case GL_RGB16F: return std::make_pair(RGB, GL_FLOAT);
        case GL_RG16F: return std::make_pair(GL_RG, GL_FLOAT);
        case GL_R16F: return std::make_pair(GL_RED, GL_FLOAT);

        case GL_RGBA32F: return std::make_pair(RGBA, GL_FLOAT);
        case GL_RGB32F: return std::make_pair(RGB, GL_FLOAT);
        case GL_RG32F: return std::make_pair(GL_RG, GL_FLOAT);
        case GL_R32F: return std::make_pair(GL_RED, GL_FLOAT);
    }
    assert(false);
    return std::make_pair(0, 0);
}

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
    int levels = desc.mipmapped ? _mipmapLevelCount(desc.width, desc.height) : 1;

    glCreateTextures(GL_TEXTURE_2D, 1, &_texture_id);
    glTextureStorage2D(_texture_id, levels, desc.internal_format, desc.width, desc.height);

    if (desc.texture_pixels != nullptr)
    {
        auto format = _getExternalFormatAndType(desc.internal_format, desc.texture_pixels_in_bgr);
        glTextureSubImage2D(_texture_id, 0, 0, 0, _width, _height, format.first, format.second, desc.texture_pixels);

        if (desc.mipmapped)
        {
            glTextureParameteri(_texture_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTextureParameteri(_texture_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTextureParameteri(_texture_id, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16);
            glGenerateTextureMipmap(_texture_id);            
        }
        else
        {
            glTextureParameteri(_texture_id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTextureParameteri(_texture_id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }
    }
}

GLTexture2D::~GLTexture2D()
{
    
}

Uptr<GLTexture2D> createMipmappedTexture2D(int width, int height, GLenum internal_format, void* pixels, bool pixels_in_bgr)
{
    GLTexture2DDesc desc;
    desc.mipmapped = true;
    desc.width = width;
    desc.height = height;
    desc.internal_format = internal_format;
    desc.texture_pixels = pixels;
    desc.texture_pixels_in_bgr = pixels_in_bgr;
    
    return std::make_unique<GLTexture2D>(desc);
}

}