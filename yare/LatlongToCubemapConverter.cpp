#include "LatlongToCubemapConverter.h"

#include "RenderResources.h"
#include "GLFramebuffer.h"
#include "GLTexture.h"
#include "GLDevice.h"
#include "glsl_binding_defines.h"
#include "GLSampler.h"
#include "GLProgram.h"

namespace yare {

LatlongToCubemapConverter::LatlongToCubemapConverter(const RenderResources& render_resources)
 : _render_resources(render_resources)
{
   GLSamplerDesc sampler_desc;
   sampler_desc.min_filter = GL_LINEAR;
   sampler_desc.mag_filter = GL_LINEAR;
   sampler_desc.wrap_mode_u = GL_REPEAT;
   sampler_desc.wrap_mode_v = GL_CLAMP_TO_EDGE;
   
   _latlong_sampler = createSampler(sampler_desc);
   _render_cubemap_face_prog = createProgramFromFile("latlong_to_cubemap.glsl");
}

Uptr<GLTextureCubemap> LatlongToCubemapConverter::convert(const GLTexture2D& latlong_texture) const
{
   Uptr<GLTextureCubemap> cubemap = createMipmappedTextureCubemap(latlong_texture.height(), GL_RGB16F);
 
   GLFramebufferDesc desc;
   desc.attachments.push_back(GLGLFramebufferAttachmentDesc{ cubemap.get(), GL_COLOR_ATTACHMENT0, 0 });
   GLFramebuffer render_to_cubemap(desc);
   
   GLDevice::bindProgram(*_render_cubemap_face_prog);
   GLDevice::bindVertexSource(*_render_resources.fullscreen_quad_source);
   GLDevice::bindTexture(BI_LATLONG_TEXTURE, latlong_texture, *_latlong_sampler);

   for (int i = 0; i < 6; ++i)
   {
      GLDevice::bindFramebuffer(&render_to_cubemap, i);
      glViewport(0, 0, latlong_texture.height(), latlong_texture.height());
      glUniform1i(BI_FACE, i);
      GLDevice::draw(0, 6);
   }

   cubemap->buildMipmaps();

   //blitColor(render_to_cubemap, 0, default_framebuffer, 0);
   return cubemap;
}

}