#pragma once

#include <vector>
#include <GL/glew.h>

#include "tools.h"
#include "GLTexture.h"

namespace yare {

struct GLGLFramebufferAttachmentDesc
{
    GLTexture* texture;    
    GLenum attachment_type;
    int texture_level;
};

struct GLFramebufferDesc
{
    std::vector<GLGLFramebufferAttachmentDesc> attachments;
};

class GLFramebuffer
{
public:
   GLFramebuffer(const GLFramebufferDesc& desc);
   virtual ~GLFramebuffer();

   GLuint id() const { return _framebuffer_id; }
   int width() const { return _width; }
   int height() const { return _height; }

private:
   DISALLOW_COPY_AND_ASSIGN(GLFramebuffer)
   GLuint _framebuffer_id;
   int _width;
   int _height;
};

static GLFramebuffer* default_framebuffer = nullptr;

void blitColor(const GLFramebuffer& source, int src_attachment, const GLFramebuffer* destination, int dst_attachment);

}