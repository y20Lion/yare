#include "CubemapFiltering.h"

#include <cmath>
#include <glm/vec4.hpp>
#include <glm/vec2.hpp>
#include <iostream>

#include "RenderResources.h"
#include "GLFramebuffer.h"
#include "GLTexture.h"
#include "GLDevice.h"
#include "glsl_cubemap_filtering_defines.h"
#include "GLSampler.h"
#include "GLProgram.h"
#include "GLGPUTimer.h"
#include "GLBuffer.h"

#define PARALLEL_REDUCE_RESULT_WIDTH 64

namespace yare {

   using namespace glm;

   CubemapFiltering::CubemapFiltering(const RenderResources& render_resources)
 : _render_resources(render_resources)
{
   GLSamplerDesc sampler_desc;
   sampler_desc.min_filter = GL_LINEAR;
   sampler_desc.mag_filter = GL_LINEAR;
   sampler_desc.wrap_mode_u = GL_REPEAT;
   sampler_desc.wrap_mode_v = GL_CLAMP_TO_EDGE;
   
   _latlong_sampler = createSampler(sampler_desc);
   _render_cubemap_face = createProgramFromFile("latlong_to_cubemap.glsl");
   _render_diffuse_cubemap_face = createProgramFromFile("render_diffuse_cubemap.glsl");
   _compute_env_spherical_harmonics = createProgramFromFile("compute_env_spherical_harmonics.glsl");
   _spherical_harmonics_to_env = createProgramFromFile("spherical_harmonics_to_env.glsl");

   _spherical_harmonics_ssbo = createBuffer(9 * sizeof(glm::vec3), GL_MAP_WRITE_BIT);

   _parallel_reduce_env = createProgramFromFile("parallel_reduce_env.glsl");
   _parallel_reduce_result = createTexture2D(PARALLEL_REDUCE_RESULT_WIDTH, PARALLEL_REDUCE_RESULT_WIDTH/2, GL_RGBA32F);
}

Uptr<GLTextureCubemap> CubemapFiltering::createCubemapFromLatlong(const GLTexture2D& latlong_texture) const
{   
   Uptr<GLTextureCubemap> cubemap = createMipmappedTextureCubemap(latlong_texture.height(), GL_RGB16F); 
   Sptr<GLTextureCubemap> cubemap_without_deleter(cubemap.get(), [](GLTextureCubemap*){});

   GLFramebufferDesc desc;
   desc.attachments.push_back(GLFramebufferAttachmentDesc{ cubemap_without_deleter, GL_COLOR_ATTACHMENT0, 0 });
   GLFramebuffer render_to_cubemap(desc);

   GLDevice::bindProgram(*_render_cubemap_face);   
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

Uptr<GLTextureCubemap> CubemapFiltering::createDiffuseCubemap(const GLTextureCubemap& input_cubemap, DiffuseFilteringMethod method) const
{
   Uptr<GLTextureCubemap> diffuse_cubemap = createMipmappedTextureCubemap(64, GL_RGB16F);
   Sptr<GLTextureCubemap> diffuse_cubemap_without_deleter(diffuse_cubemap.get(), [](GLTextureCubemap*) {});

   GLFramebufferDesc desc;
   desc.attachments.push_back(GLFramebufferAttachmentDesc{ diffuse_cubemap_without_deleter, GL_COLOR_ATTACHMENT0, 0 });
   GLFramebuffer render_to_cubemap(desc);
   
   int last_level = input_cubemap.levelCount() - 1;
   int size_64_level = last_level - int(std::log2(128));
   
   if (method == DiffuseFilteringMethod::BruteForce)
      _computeDiffuseEnvWithBruteForce(input_cubemap, size_64_level, render_to_cubemap, *diffuse_cubemap);
   else
      _computeDiffuseEnvWithSphericalHarmonics(input_cubemap, size_64_level, render_to_cubemap, *diffuse_cubemap);

   diffuse_cubemap->buildMipmaps();
   _render_resources.timer->stop();
   _render_resources.timer->printElapsedTimeInMs(TimerResult::CurrentFrame);

   return diffuse_cubemap;
}

static vec3 _sphericalCoordinatesToDirection(float phi, float theta)
{
   return vec3(sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta));
}

std::vector<CubemapFiltering::ExtractedLight> CubemapFiltering::extractDirectionalLightSourcesFromLatlong(const GLTexture2D& latlong_texture) const
{
   GLDevice::bindTexture(BI_LATLONG_TEXTURE, latlong_texture, *_render_resources.samplers.nearest_clampToEdge);
   GLDevice::bindImage(BI_OUTPUT_IMAGE, *_parallel_reduce_result, GL_WRITE_ONLY);
   GLDevice::bindProgram(*_parallel_reduce_env);
   glDispatchCompute(latlong_texture.width()/TILE_WIDTH, latlong_texture.height()/TILE_WIDTH, 1);
   glMemoryBarrier(GL_ALL_BARRIER_BITS); //GL_TEXTURE_UPDATE_BARRIER_BIT should be enough but I'm paranoid

   auto buf_ptr = std::make_unique<uint8_t[]>(_parallel_reduce_result->readbackBufferSize());
   _parallel_reduce_result->readbackTexels(buf_ptr.get());
   vec4* pixels = (vec4*)buf_ptr.get();
   
   std::vector<CubemapFiltering::ExtractedLight> lights;

   for (int y = 0; y < PARALLEL_REDUCE_RESULT_WIDTH/2; ++y)
   {
      for (int x = 0; x < PARALLEL_REDUCE_RESULT_WIDTH; ++x)
      {
         vec4 val = pixels[x + y * PARALLEL_REDUCE_RESULT_WIDTH];
         if (val.w != 0.0)
         {      
            lights.push_back(CubemapFiltering::ExtractedLight());
            auto& light = lights.back();
            light.color = val.xyz;
            light.direction = _sphericalCoordinatesToDirection((x + 0.5f) / PARALLEL_REDUCE_RESULT_WIDTH, (y + 0.5f) / PARALLEL_REDUCE_RESULT_WIDTH / 2);
         }
      }
   }

   return lights;
}


void CubemapFiltering::_computeDiffuseEnvWithBruteForce(const GLTextureCubemap& input_cubemap,
                                                        const int used_input_cubemap_level,
                                                        const GLFramebuffer& diffuse_cubemap_framebuffer,
                                                        GLTextureCubemap& diffuse_cubemap) const
{
   GLDevice::bindProgram(*_render_diffuse_cubemap_face);
   glUniform1i(BI_INPUT_CUBEMAP_LEVEL, used_input_cubemap_level);
   GLDevice::bindVertexSource(*_render_resources.fullscreen_triangle_source);
   GLDevice::bindTexture(BI_INPUT_CUBEMAP, input_cubemap, *_render_resources.samplers.nearest_clampToEdge);

   // brute force: for each texel of the output cubemap we sample all texels of the input cubemap
   // That makes a total of 603 979 776 texture reads, which is done in 190ms on my GTX 770 (We can thank Mr cache)
   for (int i = 0; i < 6; ++i)
   {
      GLDevice::bindFramebuffer(&diffuse_cubemap_framebuffer, i);
      glViewport(0, 0, diffuse_cubemap.width(), diffuse_cubemap.height());
      glUniform1i(BI_FACE, i);
      GLDevice::draw(0, 6);
   }
}

void CubemapFiltering::_computeDiffuseEnvWithSphericalHarmonics(const GLTextureCubemap& input_cubemap,
                                                                const int used_input_cubemap_level,
                                                                const GLFramebuffer& diffuse_cubemap_framebuffer,
                                                                GLTextureCubemap& diffuse_cubemap) const
{
   _render_resources.timer->start();
   GLDevice::bindProgram(*_compute_env_spherical_harmonics);
   glUniform1i(BI_INPUT_CUBEMAP_LEVEL, used_input_cubemap_level);   
   GLDevice::bindTexture(BI_INPUT_CUBEMAP, input_cubemap, *_render_resources.samplers.nearest_clampToEdge);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BI_SPHERICAL_HARMONICS_SSBO, _spherical_harmonics_ssbo->id());

   glDispatchCompute(9, 1, 1);
   glMemoryBarrier(GL_ALL_BARRIER_BITS);

   /*glm::vec3* coeffs = (glm::vec3*)_spherical_harmonics_ssbo->map(GL_MAP_READ_BIT);
   for (int i = 0; i < 9; ++i)
   {
      std::cout << coeffs[i].x << " "<< coeffs[i].y << " "<< coeffs[i].z << std::endl;
   }
   _spherical_harmonics_ssbo->unmap();*/
   
   GLDevice::bindProgram(*_spherical_harmonics_to_env);   
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BI_SPHERICAL_HARMONICS_SSBO, _spherical_harmonics_ssbo->id());
   GLDevice::bindVertexSource(*_render_resources.fullscreen_triangle_source);   
   for (int i = 0; i < 6; ++i)
   {
      GLDevice::bindFramebuffer(&diffuse_cubemap_framebuffer, i);
      glViewport(0, 0, diffuse_cubemap.width(), diffuse_cubemap.height());
      glUniform1i(BI_FACE, i);
      GLDevice::draw(0, 6);
   }
}

}