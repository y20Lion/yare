#pragma once

#include <GL/glew.h>
#include "tools.h"

namespace yare {

struct GLTexture2DDesc
{
    int width, height;
    bool mipmapped;
    void* texture_pixels;
    bool texture_pixels_in_bgr;
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

private:
    int _width, _height;
    DISALLOW_COPY_AND_ASSIGN(GLTexture2D)
};

Uptr<GLTexture2D> createMipmappedTexture2D(int width, int height, GLenum internal_format, void* pixels, bool pixels_in_bgr=false);

}