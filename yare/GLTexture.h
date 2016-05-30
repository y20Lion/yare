#pragma once

#include <GL/glew.h>
#include "tools.h"

namespace yare {



class GLTexture
{
public:
    GLTexture();
    virtual ~GLTexture();
    GLuint id() const { return _texture_id; }
    virtual int width() const = 0;
    virtual int height() const = 0;
    int levelCount() const { return _level_count;  }

    void buildMipmaps();

protected:
    DISALLOW_COPY_AND_ASSIGN(GLTexture)
    GLuint _texture_id;
    GLuint _level_count;
};

struct GLTexture2DDesc
{
   int width, height;
   bool mipmapped;
   void* texture_pixels;
   bool texture_pixels_in_bgr;
   GLenum internal_format;
};

class GLTexture2D : public GLTexture
{
public:
   GLTexture2D(const GLTexture2DDesc& desc);
   virtual ~GLTexture2D();

   int width() const override { return _width;  }
   int height() const override { return _height;  }
   GLenum internalFormat() const { return _internal_format;  }

private:
   GLenum _internal_format;
   int _width, _height;
   DISALLOW_COPY_AND_ASSIGN(GLTexture2D)
};

struct GLTextureCubemapDesc
{
   int width;
   bool mipmapped; 
   GLenum internal_format;
};

class GLTextureCubemap : public GLTexture
{
public:
   GLTextureCubemap(const GLTextureCubemapDesc& desc);
   virtual ~GLTextureCubemap();

   int width() const override { return _width; }
   int height() const override  { return _width; }

private:
   int _width;
   DISALLOW_COPY_AND_ASSIGN(GLTextureCubemap)
};

Uptr<GLTexture2D> createMipmappedTexture2D(int width, int height, GLenum internal_format, void* pixels, bool pixels_in_bgr=false);
Uptr<GLTexture2D> createTexture2D(int width, int height, GLenum internal_format);
Uptr<GLTextureCubemap> createMipmappedTextureCubemap(int width, GLenum internal_format);

}