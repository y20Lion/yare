#include "CubemapConverter.h"

#include <cmath>

#include "RenderResources.h"
#include "GLFramebuffer.h"
#include "GLTexture.h"
#include "GLDevice.h"
#include "glsl_cubemap_converter_defines.h"
#include "GLSampler.h"
#include "GLProgram.h"
#include "GLGPUTimer.h"

namespace yare {

   CubemapConverter::CubemapConverter(const RenderResources& render_resources)
 : _render_resources(render_resources)
{
   GLSamplerDesc sampler_desc;
   sampler_desc.min_filter = GL_LINEAR;
   sampler_desc.mag_filter = GL_LINEAR;
   sampler_desc.wrap_mode_u = GL_REPEAT;
   sampler_desc.wrap_mode_v = GL_CLAMP_TO_EDGE;
   
   _latlong_sampler = createSampler(sampler_desc);
   _render_cubemap_face_prog = createProgramFromFile("latlong_to_cubemap.glsl");
   _render_diffuse_cubemap_face_prog = createProgramFromFile("render_diffuse_cubemap.glsl");
}

Uptr<GLTextureCubemap> CubemapConverter::createCubemapFromLatlong(const GLTexture2D& latlong_texture) const
{
   Uptr<GLTextureCubemap> cubemap = createMipmappedTextureCubemap(latlong_texture.height(), GL_RGB16F); 
   Sptr<GLTextureCubemap> cubemap_without_deleter(cubemap.get(), [](GLTextureCubemap*){});

   GLFramebufferDesc desc;
   desc.attachments.push_back(GLFramebufferAttachmentDesc{ cubemap_without_deleter, GL_COLOR_ATTACHMENT0, 0 });
   GLFramebuffer render_to_cubemap(desc);

   GLDevice::bindProgram(*_render_cubemap_face_prog);   
   GLDevice::bindVertexSource(*_render_resources.fullscreen_triangle_source);
   GLDevice::bindTexture(BI_LATLONG_TEXTURE, latlong_texture, *_latlong_sampler);

   for (int i = 0; i < 6; ++i)
   {
      GLDevice::bindFramebuffer(&render_to_cubemap, i);
      glViewport(0, 0, latlong_texture.height(), latlong_texture.height());
      glUniform1i(BI_FACE, i);
      GLDevice::draw(0, 6);
   }

   cubemap->buildMipmaps();

   return cubemap;
}

Uptr<GLTextureCubemap> CubemapConverter::createDiffuseCubemap(const GLTextureCubemap& input_cubemap) const
{
   Uptr<GLTextureCubemap> diffuse_cubemap = createMipmappedTextureCubemap(64, GL_RGB16F);
   Sptr<GLTextureCubemap> diffuse_cubemap_without_deleter(diffuse_cubemap.get(), [](GLTextureCubemap*) {});

   GLFramebufferDesc desc;
   desc.attachments.push_back(GLFramebufferAttachmentDesc{ diffuse_cubemap_without_deleter, GL_COLOR_ATTACHMENT0, 0 });
   GLFramebuffer render_to_cubemap(desc);
   _render_resources.timer->start();
   int last_level = input_cubemap.levelCount() - 1;
   int size_64_level = last_level - int(std::log2(64));
   GLDevice::bindProgram(*_render_diffuse_cubemap_face_prog);
   glUniform1i(BI_INPUT_CUBEMAP_LEVEL, size_64_level);
   GLDevice::bindVertexSource(*_render_resources.fullscreen_triangle_source);
   GLDevice::bindTexture(BI_INPUT_CUBEMAP, input_cubemap, *_render_resources.sampler_nearest_clampToEdge);

   // brute force: for each texel of the output cubemap we sample all texels of the input cubemap
   // That makes a total of 603 979 776 texture reads, which is done in 190ms on my GTX 770(I guess we can thank Mr cache)
   for (int i = 0; i < 6; ++i)
   {
      GLDevice::bindFramebuffer(&render_to_cubemap, i);
      glViewport(0, 0, diffuse_cubemap->width(), diffuse_cubemap->height());
      glUniform1i(BI_FACE, i);
      GLDevice::draw(0, 6);
   }

   diffuse_cubemap->buildMipmaps();
   _render_resources.timer->stop();
   _render_resources.timer->printElapsedTimeInMs(TimerResult::CurrentFrame);

   return diffuse_cubemap;
}

}