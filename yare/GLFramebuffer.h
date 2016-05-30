#pragma once

#include <vector>
#include <map>
#include <GL/glew.h>

#include "tools.h"
#include "GLTexture.h"
#include "ImageSize.h"

namespace yare {

struct GLGLFramebufferAttachmentDesc
{
    Sptr<GLTexture> texture;    
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

   void setDrawColorBuffer(int color_attachment);
   GLTexture& attachedTexture(GLenum attachment_type) const;

private:
   DISALLOW_COPY_AND_ASSIGN(GLFramebuffer)
   std::map<GLenum, Sptr<GLTexture>> _attachments;
   GLuint _framebuffer_id;
   int _width;
   int _height;
};

static GLFramebuffer* default_framebuffer = nullptr;

Uptr<GLFramebuffer> createFramebuffer(const ImageSize& size, GLenum color_format, int color_attachments_count, GLenum depth_format = GL_NONE);
void blitColor(const GLFramebuffer& source, int src_attachment, const GLFramebuffer* destination, int dst_attachment);

}