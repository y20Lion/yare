#include "RenderResources.h"

#include "GLVertexSource.h"
#include "GLBuffer.h"
#include "GLProgram.h"
#include "GLSampler.h"

namespace yare {

static float quad_vertices[] = { 1.0f,1.0f,   -1.0f,1.0f,   1.0f,-1.0f,  -1.0f,1.0f,   -1.0f,-1.0f,  1.0f,-1.0f };

RenderResources::RenderResources()
{
   GLSamplerDesc sampler_desc;
   sampler_desc.min_filter = GL_LINEAR_MIPMAP_LINEAR;
   sampler_desc.mag_filter = GL_LINEAR;
   sampler_desc.anisotropy = 16;
   sampler_desc.wrap_mode_u = GL_CLAMP_TO_EDGE;
   sampler_desc.wrap_mode_v = GL_CLAMP_TO_EDGE;
   sampler_desc.wrap_mode_w = GL_CLAMP_TO_EDGE;
   sampler_cubemap = createSampler(sampler_desc);
   
   
   fullscreen_quad_vbo = createBuffer(sizeof(quad_vertices), quad_vertices);
   fullscreen_quad_source = std::make_unique<GLVertexSource>();
   fullscreen_quad_source->setVertexBuffer(*fullscreen_quad_vbo);
   fullscreen_quad_source->setVertexAttribute(0, 2, GL_FLOAT);
   fullscreen_quad_source->setVertexCount(6);

   auto shade_tree_prog_desc = createProgramDescFromFile("ShadeTreeMaterial.glsl");
   shade_tree_material_vertex = std::move(shade_tree_prog_desc.shaders[0].code);
   shade_tree_material_fragment = std::move(shade_tree_prog_desc.shaders[1].code);   
}

RenderResources::~RenderResources()
{
}

}