#pragma once

#include <GL/glew.h>
#include "tools.h"

namespace yare {



class GLTexture
{
public:
    GLTexture();
    virtual ~GLTexture();
    DISALLOW_COPY_AND_ASSIGN(GLTexture)

    GLuint id() const { return _texture_id; }
    virtual int width() const { return 1; };
    virtual int height() const { return 1; };
    virtual int depth() const { return 1; };
    int levelCount() const { return _level_count;  }

    void buildMipmaps();

protected:    
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
   DISALLOW_COPY_AND_ASSIGN(GLTexture1D)

   int width() const override { return _width; }
   GLenum internalFormat() const { return _internal_format; }

private:
   GLenum _internal_format;
   int _width;   
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
   DISALLOW_COPY_AND_ASSIGN(GLTexture2D)

   void readbackTexels(void* ptr, int level = 0) const;
   int readbackBufferSize() const;

   int width() const override { return _width;  }
   int height() const override { return _height;  }
   GLenum internalFormat() const { return _internal_format;  }

private:
   GLenum _internal_format;
   int _width, _height;   
};

struct GLTexture3DDesc
{
   int width, height, depth;
   bool mipmapped;
   void* texture_pixels;
   GLenum internal_format;
};

class GLTexture3D : public GLTexture
{
public:
   GLTexture3D(const GLTexture3DDesc& desc);
   virtual ~GLTexture3D();
   DISALLOW_COPY_AND_ASSIGN(GLTexture3D)

   void readbackTexels(void* ptr, int level = 0) const;
   int readbackBufferSize() const;

   int width() const override { return _width; }
   int height() const override { return _height; }
   int depth() const override { return _depth; }
   GLenum internalFormat() const { return _internal_format; }

private:
   GLenum _internal_format;
   int _width, _height, _depth;   
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
Uptr<GLTexture1D> createTexture1D(int width, GLenum internal_format, void* pixels = nullptr);

Uptr<GLTexture2D> createMipmappedTexture2D(int width, int height, GLenum internal_format, void* pixels, bool pixels_in_bgr=false);
Uptr<GLTexture2D> createTexture2D(int width, int height, GLenum internal_format, void* pixels = nullptr);

Uptr<GLTexture3D> createMipmappedTexture3D(int width, int height, int depth, GLenum internal_format, void* pixels);
Uptr<GLTexture3D> createTexture3D(int width, int height, int depth, GLenum internal_format, void* pixels = nullptr);

Uptr<GLTextureCubemap> createMipmappedTextureCubemap(int width, GLenum internal_format);

}