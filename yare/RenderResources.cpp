#include "RenderResources.h"

#include <random>
#include <functional>
#include <glm/vec2.hpp>

#include "GLVertexSource.h"
#include "GLBuffer.h"
#include "GLProgram.h"
#include "GLSampler.h"
#include "GLFramebuffer.h"
#include "GLGPUTimer.h"
#include "glsl_global_defines.h"

namespace yare {

using namespace glm;

static float quad_vertices[] = { 1.0f,1.0f,   -1.0f,1.0f,   1.0f,-1.0f,  -1.0f,1.0f,   -1.0f,-1.0f,  1.0f,-1.0f };
static float triangle_vertices[] = { -1.0f,-1.0f,  3.0f,-1.0f,  -1.0f, 3.0f };

Samplers createSamplers()
{
   Samplers samplers;

   GLSamplerDesc sampler_desc;
   sampler_desc.min_filter = GL_LINEAR_MIPMAP_LINEAR;
   sampler_desc.mag_filter = GL_LINEAR;
   sampler_desc.anisotropy = 16;
   sampler_desc.wrap_mode_u = GL_REPEAT;
   sampler_desc.wrap_mode_v = GL_REPEAT;
   sampler_desc.wrap_mode_w = GL_REPEAT;
   samplers.mipmap_repeat = createSampler(sampler_desc);

   sampler_desc.min_filter = GL_LINEAR_MIPMAP_LINEAR;
   sampler_desc.mag_filter = GL_LINEAR;
   sampler_desc.anisotropy = 16;
   sampler_desc.wrap_mode_u = GL_CLAMP_TO_EDGE;
   sampler_desc.wrap_mode_v = GL_CLAMP_TO_EDGE;
   sampler_desc.wrap_mode_w = GL_CLAMP_TO_EDGE;
   samplers.mipmap_clampToEdge = createSampler(sampler_desc);

   sampler_desc.min_filter = GL_LINEAR;
   sampler_desc.mag_filter = GL_LINEAR;
   sampler_desc.anisotropy = 1;
   sampler_desc.wrap_mode_u = GL_CLAMP_TO_EDGE;
   sampler_desc.wrap_mode_v = GL_CLAMP_TO_EDGE;
   sampler_desc.wrap_mode_w = GL_CLAMP_TO_EDGE;
   samplers.bilinear_clampToEdge = createSampler(sampler_desc);

   sampler_desc.min_filter = GL_NEAREST;
   sampler_desc.mag_filter = GL_NEAREST;
   sampler_desc.anisotropy = 1;
   sampler_desc.wrap_mode_u = GL_CLAMP_TO_EDGE;
   sampler_desc.wrap_mode_v = GL_CLAMP_TO_EDGE;
   sampler_desc.wrap_mode_w = GL_CLAMP_TO_EDGE;
   samplers.nearest_clampToEdge = createSampler(sampler_desc);

   return samplers;
}


RenderResources::RenderResources(const ImageSize& framebuffer_size_)
{
   timer = std::make_unique<GLGPUTimer>();
   
   framebuffer_size = framebuffer_size_;
   main_framebuffer = createFramebuffer(framebuffer_size, GL_RGBA32F, 1, GL_DEPTH_COMPONENT32F);
   halfsize_postprocess_fbo = createFramebuffer(ImageSize(framebuffer_size.width/2, framebuffer_size.height/2), GL_RGBA32F, 2);

   samplers = createSamplers();   

   fullscreen_triangle_vbo = createBuffer(sizeof(triangle_vertices), 0, triangle_vertices);
   fullscreen_triangle_source = std::make_unique<GLVertexSource>();
   fullscreen_triangle_source->setVertexBuffer(*fullscreen_triangle_vbo);
   fullscreen_triangle_source->setVertexAttribute(0, 2, GL_FLOAT, GLSLVecType::vec);
   fullscreen_triangle_source->setVertexCount(3);

   const int num_samples = 1000;
   hammersley_samples = createBuffer(sizeof(vec2)*num_samples, GL_MAP_WRITE_BIT);
   vec2* samples = (vec2*) hammersley_samples->map(GL_MAP_WRITE_BIT);
   auto real_rand = std::bind(std::uniform_real_distribution<float>(0.0, 1.0), std::mt19937());
   
   for (int i = 0; i < num_samples; ++i)
   {
      samples[i] = vec2(real_rand(), real_rand());
   }   
   hammersley_samples->unmap();
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BI_HAMMERSLEY_SAMPLES_SSBO, hammersley_samples->id());

   auto shade_tree_prog_desc = createProgramDescFromFile("ShadeTreeMaterial.glsl");
   shade_tree_material_vertex = std::move(shade_tree_prog_desc.shaders[0].code);
   shade_tree_material_fragment = std::move(shade_tree_prog_desc.shaders[1].code);   

   present_texture = createProgramFromFile("present_texture.glsl");
}

RenderResources::~RenderResources()
{
}

}