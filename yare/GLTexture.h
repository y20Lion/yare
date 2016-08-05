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
    virtual int width() const { return 1; };
    virtual int height() const { return 1; };
    virtual int depth() const { return 1; };
    int levelCount() const { return _level_count;  }

    void buildMipmaps();

protected:
    DISALLOW_COPY_AND_ASSIGN(GLTexture)
    GLuint _texture_id;
    GLuint _level_count;
};

struct GLTexture1DDesc
{
   int width;
   bool mipmapped;
   void* texture_pixels;
   bool texture_pixels_in_bgr;
   GLenum internal_format;
};

class GLTexture1D : public GLTexture
{
public:
   GLTexture1D(const GLTexture1DDesc& desc);
   virtual ~GLTexture1D();

   int width() const override { return _width; }
   GLenum internalFormat() const { return _internal_format; }

private:
   GLenum _internal_format;
   int _width;
   DISALLOW_COPY_AND_ASSIGN(GLTexture1D)
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

   void readbackPixels(void* ptr, int level = 0) const;
   int readbackBufferSize() const;

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

Uptr<GLTexture1D> createMipmappedTexture1D(int width, GLenum internal_format, void* pixels, bool pixels_in_bgr = false);
Uptr<GLTexture1D> createTexture1D(int width, GLenum internal_format, void* pixels);

Uptr<GLTexture2D> createMipmappedTexture2D(int width, int height, GLenum internal_format, void* pixels, bool pixels_in_bgr=false);
Uptr<GLTexture2D> createTexture2D(int width, int height, GLenum internal_format, void* pixels = nullptr);


Uptr<GLTextureCubemap> createMipmappedTextureCubemap(int width, GLenum internal_format);

}