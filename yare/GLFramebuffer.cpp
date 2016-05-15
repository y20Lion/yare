#include "GLFramebuffer.h"

#include "GLTexture.h"
namespace yare {

GLFramebuffer::GLFramebuffer(const GLFramebufferDesc& desc)
   : _width(0)
   , _height(0)
{
   glGenFramebuffers(1, &_framebuffer_id);

   for (const auto& attachment : desc.attachments)
   {
      auto cubemap = dynamic_cast<GLTextureCubemap*>(attachment.texture);
      if (cubemap)
      {
         for (int i = 0; i < 6; ++i)
            glNamedFramebufferTexture2DEXT(_framebuffer_id, attachment.attachment_type + i, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, attachment.texture->id(), attachment.texture_level);         
      }
      else
      {
         glNamedFramebufferTexture(_framebuffer_id, attachment.attachment_type, attachment.texture->id(), attachment.texture_level);
      }
      _width = attachment.texture->width();
      _height = attachment.texture->height();
   }

   auto check = glCheckNamedFramebufferStatus(_framebuffer_id, GL_FRAMEBUFFER);
}

GLFramebuffer::~GLFramebuffer()
{
   glDeleteFramebuffers(1, &_framebuffer_id);
}

void blitColor(const GLFramebuffer& source, int src_attachment, const GLFramebuffer* destination, int dst_attachment)
{
   int destination_id = destination ? destination->id() : 0;
   
   glNamedFramebufferReadBuffer(source.id(), GL_COLOR_ATTACHMENT0 + src_attachment);
   if (destination)
      glNamedFramebufferDrawBuffer(destination_id, GL_COLOR_ATTACHMENT0 + dst_attachment);
   glBlitNamedFramebuffer(source.id(), destination_id, 0, 0, source.width(), source.height(), 0, 0, source.width(), source.height(), GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

}