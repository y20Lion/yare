#include "GLTexture.h"

#include <assert.h>
#include <algorithm>

#include "GLFormats.h"
#include "GLBuffer.h"

namespace yare {

static std::pair<GLenum, GLenum> _getExternalFormatAndType(GLenum internal_format, bool bgr)
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

    GLenum RGB_INTEGER = bgr ? GL_BGR_INTEGER : GL_RGB_INTEGER;
    GLenum RGBA_INTEGER = bgr ? GL_BGRA_INTEGER : GL_RGBA_INTEGER;

    switch (internal_format)
    {
       case GL_RGBA32UI: return std::make_pair(RGBA_INTEGER, GL_UNSIGNED_INT);
       case GL_RGB32UI: return std::make_pair(RGB_INTEGER, GL_UNSIGNED_INT);
       case GL_RG32UI: return std::make_pair(GL_RG_INTEGER, GL_UNSIGNED_INT);
       case GL_R32UI: return std::make_pair(GL_RED_INTEGER, GL_UNSIGNED_INT);

       case GL_RGBA32I: return std::make_pair(RGBA_INTEGER, GL_INT);
       case GL_RGB32I: return std::make_pair(RGB_INTEGER, GL_INT);
       case GL_RG32I: return std::make_pair(GL_RG_INTEGER, GL_INT);
       case GL_R32I: return std::make_pair(GL_RED_INTEGER, GL_INT);
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

void GLTexture::buildMipmaps()
{
   glGenerateTextureMipmap(_texture_id);
}

GLTexture1D::GLTexture1D(const GLTexture1DDesc& desc)
   : _width(desc.width) 
{
   _internal_format = desc.internal_format;
   _level_count = desc.mipmapped ? GLFormats::mipmapLevelCount(desc.width, desc.width) : 1;

   glCreateTextures(GL_TEXTURE_1D, 1, &_texture_id);
   glTextureStorage1D(_texture_id, _level_count, desc.internal_format, desc.width);

   if (desc.texture_pixels != nullptr)
   {
      auto format = _getExternalFormatAndType(desc.internal_format, desc.texture_pixels_in_bgr);
      glTextureSubImage1D(_texture_id, 0, 0, _width, format.first, format.second, desc.texture_pixels);

      if (desc.mipmapped)
      {
         glGenerateTextureMipmap(_texture_id);
      }
   }

   glTextureParameteri(_texture_id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTextureParameteri(_texture_id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

GLTexture1D::~GLTexture1D()
{
}

GLTexture2D::GLTexture2D(const GLTexture2DDesc& desc)
   : _height(desc.height)
   , _width(desc.width)
{
   _internal_format = desc.internal_format;
   _level_count = desc.mipmapped ? GLFormats::mipmapLevelCount(desc.width, desc.height) : 1;

    glCreateTextures(GL_TEXTURE_2D, 1, &_texture_id);
    glTextureStorage2D(_texture_id, _level_count, desc.internal_format, desc.width, desc.height);

    if (desc.texture_pixels != nullptr)
    {
        auto format = _getExternalFormatAndType(desc.internal_format, desc.texture_pixels_in_bgr);
        glTextureSubImage2D(_texture_id, 0, 0, 0, _width, _height, format.first, format.second, desc.texture_pixels);

        if (desc.mipmapped)
        {
            glGenerateTextureMipmap(_texture_id);            
        }
    }

    glTextureParameteri(_texture_id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(_texture_id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

GLTexture2D::~GLTexture2D()
{    
}

void GLTexture2D::readbackTexels(void* ptr, int level) const
{
   auto format = _getExternalFormatAndType(_internal_format, false);
   int buf_size = _width * _height * GLFormats::componentCount(format.first) * GLFormats::sizeOfType(format.second);
   glGetTextureImage(_texture_id, 0, format.first, format.second, buf_size, ptr);
}

int GLTexture2D::readbackBufferSize() const
{
   auto format = _getExternalFormatAndType(_internal_format, false);
   return _width * _height * GLFormats::componentCount(format.first) * GLFormats::sizeOfType(format.second);
}

GLTexture3D::GLTexture3D(const GLTexture3DDesc& desc)
   : _height(desc.height)
   , _width(desc.width)
   , _depth(desc.depth)
{
   _internal_format = desc.internal_format;
   _level_count = desc.mipmapped ? GLFormats::mipmapLevelCount(desc.width, desc.height) : 1;

   glCreateTextures(GL_TEXTURE_3D, 1, &_texture_id);
   glTextureStorage3D(_texture_id, _level_count, desc.internal_format, desc.width, desc.height, desc.depth);

   if (desc.texture_pixels != nullptr)
   {
      auto format = _getExternalFormatAndType(desc.internal_format, false);
      glTextureSubImage3D(_texture_id, 0, 0, 0, 0, _width, _height, _depth, format.first, format.second, desc.texture_pixels);

      if (desc.mipmapped)
      {
         glGenerateTextureMipmap(_texture_id);
      }
   }

   glTextureParameteri(_texture_id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTextureParameteri(_texture_id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

GLTexture3D::~GLTexture3D()
{
}

void GLTexture3D::readbackTexels(void* ptr, int level) const
{
   auto format = _getExternalFormatAndType(_internal_format, false);
   int buf_size = _width * _height* _depth * GLFormats::componentCount(format.first) * GLFormats::sizeOfType(format.second);
   glGetTextureImage(_texture_id, 0, format.first, format.second, buf_size, ptr);
}

int GLTexture3D::readbackBufferSize() const
{
   auto format = _getExternalFormatAndType(_internal_format, false);
   return _width * _height * _depth * GLFormats::componentCount(format.first) * GLFormats::sizeOfType(format.second);
}

void GLTexture3D::updateFromBuffer(const GLBuffer& buffer, std::int64_t buffer_start_offset)
{
   glBindBuffer(GL_PIXEL_UNPACK_BUFFER, buffer.id());   
   auto format = _getExternalFormatAndType(_internal_format, false);
   glTextureSubImage3D(_texture_id, 0, 0, 0, 0, _width, _height, _depth, format.first, format.second, (void*)buffer_start_offset);
   glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

GLTextureCubemap::GLTextureCubemap(const GLTextureCubemapDesc& desc)
   : _width(desc.width)
{
   _internal_format = desc.internal_format;
   _level_count = desc.mipmapped ? GLFormats::mipmapLevelCount(desc.width, desc.width) : 1;
   glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &_texture_id);
   glTextureStorage2D(_texture_id, _level_count, desc.internal_format, desc.width, desc.width);

   if (desc.mipmapped)
   {
      glTextureParameteri(_texture_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTextureParameteri(_texture_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTextureParameteri(_texture_id, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16);
   }
   else
   {
      glTextureParameteri(_texture_id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTextureParameteri(_texture_id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   }
}

GLTextureCubemap::~GLTextureCubemap()
{
}

Uptr<GLTexture1D> createMipmappedTexture1D(int width, GLenum internal_format, void* pixels, bool pixels_in_bgr)
{
   GLTexture1DDesc desc;
   desc.mipmapped = true;
   desc.width = width;
   desc.internal_format = internal_format;
   desc.texture_pixels = pixels;
   desc.texture_pixels_in_bgr = pixels_in_bgr;

   return std::make_unique<GLTexture1D>(desc);
}

Uptr<GLTexture1D> createTexture1D(int width, GLenum internal_format, void* pixels)
{
   GLTexture1DDesc desc;
   desc.mipmapped = false;
   desc.width = width;
   desc.internal_format = internal_format;
   desc.texture_pixels = pixels;
   desc.texture_pixels_in_bgr = false;

   return std::make_unique<GLTexture1D>(desc);
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

Uptr<GLTexture2D> createTexture2D(int width, int height, GLenum internal_format, void* pixels)
{
   GLTexture2DDesc desc;
   desc.mipmapped = false;
   desc.width = width;
   desc.height = height;
   desc.internal_format = internal_format;
   desc.texture_pixels = pixels;
   desc.texture_pixels_in_bgr = false;

   return std::make_unique<GLTexture2D>(desc);
}

Uptr<GLTexture3D> createMipmappedTexture3D(int width, int height, int depth, GLenum internal_format, void* pixels)
{
   GLTexture3DDesc desc;
   desc.mipmapped = true;
   desc.width = width;
   desc.height = height;
   desc.depth = depth;
   desc.internal_format = internal_format;
   desc.texture_pixels = pixels;

   return std::make_unique<GLTexture3D>(desc);
}

Uptr<GLTexture3D> createTexture3D(int width, int height, int depth, GLenum internal_format, void* pixels)
{
   GLTexture3DDesc desc;
   desc.mipmapped = false;
   desc.width = width;
   desc.height = height;
   desc.depth = depth;
   desc.internal_format = internal_format;
   desc.texture_pixels = pixels;

   return std::make_unique<GLTexture3D>(desc);
}


Uptr<GLTextureCubemap> createMipmappedTextureCubemap(int width, GLenum internal_format)
{
   GLTextureCubemapDesc desc;
   desc.mipmapped = true;
   desc.width = width;
   desc.internal_format = internal_format;

   return std::make_unique<GLTextureCubemap>(desc);
}


}