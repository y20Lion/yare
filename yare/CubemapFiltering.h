#pragma once

#include <glm/vec3.hpp>
#include <vector>

#include "tools.h"

namespace yare {

struct RenderResources;
class GLTextureCubemap;
class GLTexture2D;
class GLProgram;
class GLSampler;
class GLFramebuffer;
class GLBuffer;

enum class DiffuseFilteringMethod {BruteForce, SphericalHarmonics};


class CubemapFiltering
{
public:
   CubemapFiltering(const RenderResources& render_resources);
   
   Uptr<GLTextureCubemap> createCubemapFromLatlong(const GLTexture2D& latlong_texture) const;

   
   Uptr<GLTextureCubemap> createDiffuseCubemap(const GLTextureCubemap& cubemap_texture, DiffuseFilteringMethod method) const;


   struct ExtractedLight
   {
      glm::vec3 color;
      glm::vec3 direction;
   };
   std::vector<ExtractedLight> extractDirectionalLightSourcesFromLatlong(const GLTexture2D& latlong_texture) const;

private:
   void _computeDiffuseEnvWithBruteForce(const GLTextureCubemap& input_cubemap,
                                         const int used_input_cubemap_level,
                                         const GLFramebuffer& diffuse_cubemap_framebuffer,
                                         GLTextureCubemap& diffuse_cubemap) const;

   void _computeDiffuseEnvWithSphericalHarmonics(const GLTextureCubemap& input_cubemap,
                                                 const int used_input_cubemap_level,
                                                 const GLFramebuffer& diffuse_cubemap_framebuffer,
                                                 GLTextureCubemap& diffuse_cubemap) const;

public: // TODO
   DISALLOW_COPY_AND_ASSIGN(CubemapFiltering)
   Uptr<GLProgram> _render_cubemap_face;
   Uptr<GLProgram> _render_diffuse_cubemap_face;
   Uptr<GLProgram> _compute_env_spherical_harmonics;
   Uptr<GLProgram> _spherical_harmonics_to_env;
   Uptr<GLSampler> _latlong_sampler;   
   Uptr<GLBuffer> _spherical_harmonics_ssbo;
   Uptr<GLProgram> _parallel_reduce_env;
   Uptr<GLTexture2D> _parallel_reduce_result;
   const RenderResources& _render_resources;
};

}