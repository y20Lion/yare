#pragma once

#include "tools.h"
#include "ImageSize.h"

#include <string>

namespace yare {

class GLBuffer;
class GLVertexSource;
class GLSampler;
class GLFramebuffer;
class GLGPUTimer;
class GLProgram;
class GLTexture1D;

struct Samplers
{
   Uptr<GLSampler> mipmap_repeat;
   Uptr<GLSampler> mipmap_clampToEdge;   
   Uptr<GLSampler> linear_clampToEdge;
   Uptr<GLSampler> nearest_clampToEdge;
};


struct RenderResources
{
   RenderResources(const ImageSize& framebuffer_size);
   ~RenderResources();

   ImageSize framebuffer_size;
   Uptr<GLFramebuffer> main_framebuffer;
   Uptr<GLFramebuffer> ssao_framebuffer;
   Uptr<GLFramebuffer> halfsize_postprocess_fbo;

   Samplers samplers;

   Uptr<GLBuffer> fullscreen_triangle_vbo;
   Uptr<GLVertexSource> fullscreen_triangle_source;

   Uptr<GLGPUTimer> timer;
   Uptr<GLGPUTimer> ssao_timer;
   Uptr<GLGPUTimer> z_pass_timer;
   Uptr<GLGPUTimer> voxelize_timer;
   Uptr<GLGPUTimer> raytrace_timer;
   Uptr<GLGPUTimer> material_pass_timer;
   Uptr<GLGPUTimer> background_timer;
   Uptr<GLGPUTimer> volumetric_fog_timer;

   Uptr<GLBuffer> hammersley_samples;
   Uptr<GLTexture1D> random_texture;

   Uptr<GLProgram> present_texture;

   std::string shade_tree_material_fragment;
   std::string shade_tree_material_vertex;
private:
   DISALLOW_COPY_AND_ASSIGN(RenderResources)
};

}