#include "GLFramebuffer.h"

#include "GLTexture.h"
namespace yare {

GLFramebuffer::GLFramebuffer(const GLFramebufferDesc& desc)
   : _width(0)
   , _height(0)
{
   glGenFramebuffers(1, &_framebuffer_id);
   glNamedFramebufferParameteri(_framebuffer_id, GL_FRAMEBUFFER_DEFAULT_WIDTH, desc.default_width);
   glNamedFramebufferParameteri(_framebuffer_id, GL_FRAMEBUFFER_DEFAULT_HEIGHT, desc.default_height);

   for (const auto& attachment : desc.attachments)
   {
      auto cubemap = dynamic_cast<GLTextureCubemap*>(attachment.texture.get());
      if (cubemap)
      {
         for (int i = 0; i < 6; ++i)
            glNamedFramebufferTexture2DEXT(_framebuffer_id, attachment.attachment_type + i, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, attachment.texture->id(), attachment.texture_level);         
      }
      else
      {
         glNamedFramebufferTexture2DEXT(_framebuffer_id, attachment.attachment_type, GL_TEXTURE_2D, attachment.texture->id(), attachment.texture_level);
      }
      _width = attachment.texture->width();
      _height = attachment.texture->height();
      _attachments[attachment.attachment_type] = attachment.texture;
   }

   auto check = glCheckNamedFramebufferStatus(_framebuffer_id, GL_FRAMEBUFFER);
}

GLFramebuffer::~GLFramebuffer()
{
   glDeleteFramebuffers(1, &_framebuffer_id);
}

void GLFramebuffer::setDrawColorBuffer(int color_attachment)
{
   glNamedFramebufferDrawBuffer(id(), GL_COLOR_ATTACHMENT0 + color_attachment);
}

GLTexture& GLFramebuffer::attachedTexture(GLenum attachment_type) const
{
   return *_attachments.at(attachment_type);
}


void blitColor(const GLFramebuffer& source, int src_attachment, const GLFramebuffer* destination, int dst_attachment)
{
   int destination_id = destination ? destination->id() : 0;
   
   glNamedFramebufferReadBuffer(source.id(), GL_COLOR_ATTACHMENT0 + src_attachment);
   if (destination)
      glNamedFramebufferDrawBuffer(destination_id, GL_COLOR_ATTACHMENT0 + dst_attachment);
   glBlitNamedFramebuffer(source.id(), destination_id, 0, 0, source.width(), source.height(), 0, 0, source.width(), source.height(), GL_COLOR_BUFFER_BIT, GL_NEAREST);
}


Uptr<GLFramebuffer> createEmptyFramebuffer(const ImageSize& size)
{
   GLFramebufferDesc desc;
   desc.default_width = size.width;
   desc.default_height = size.height;
   return std::make_unique<GLFramebuffer>(desc);
}

Uptr<GLFramebuffer> createFramebuffer(const ImageSize& size, GLenum color_format, int color_attachments_count, GLenum depth_format)
{
   GLFramebufferDesc desc;
   for (int i = 0; i < color_attachments_count; ++i)
   {
      auto color = createTexture2D(size.width, size.height, color_format);
      desc.attachments.push_back(GLFramebufferAttachmentDesc{ std::move(color), GLenum(GL_COLOR_ATTACHMENT0+i), 0 });
   }

   if (depth_format != GL_NONE)
   {
      auto depth = createTexture2D(size.width, size.height, depth_format);
      desc.attachments.push_back(GLFramebufferAttachmentDesc{ std::move(depth), GL_DEPTH_ATTACHMENT, 0 });
   }

   return std::make_unique<GLFramebuffer>(desc);
}

}