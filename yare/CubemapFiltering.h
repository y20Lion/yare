#pragma once

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

private:
   void _computeDiffuseEnvWithBruteForce(const GLTextureCubemap& input_cubemap,
                                         const int used_input_cubemap_level,
                                         const GLFramebuffer& diffuse_cubemap_framebuffer,
                                         GLTextureCubemap& diffuse_cubemap) const;

   void _computeDiffuseEnvWithSphericalHarmonics(const GLTextureCubemap& input_cubemap,
                                                 const int used_input_cubemap_level,
                                                 const GLFramebuffer& diffuse_cubemap_framebuffer,
                                                 GLTextureCubemap& diffuse_cubemap) const;

private:
   DISALLOW_COPY_AND_ASSIGN(CubemapFiltering)
   Uptr<GLProgram> _render_cubemap_face;
   Uptr<GLProgram> _render_diffuse_cubemap_face;
   Uptr<GLProgram> _compute_env_spherical_harmonics;
   Uptr<GLProgram> _spherical_harmonics_to_env;
   Uptr<GLSampler> _latlong_sampler;   
   Uptr<GLBuffer> _spherical_harmonics_ssbo;
   const RenderResources& _render_resources;
};

}