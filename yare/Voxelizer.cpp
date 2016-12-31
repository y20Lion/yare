#include "Voxelizer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <random>
#include <functional>

#include "GLFramebuffer.h"
#include "GLProgram.h"
#include "GLDevice.h"
#include "GLBuffer.h"
#include "GLGPUTimer.h"
#include "RenderEngine.h"
#include "RenderResources.h"
#include "Scene.h"
#include "glsl_voxelizer_defines.h"
#include "glsl_global_defines.h"

namespace yare {

float radicalInverseVdC(unsigned int bits) 
{
   bits = (bits << 16u) | (bits >> 16u);
   bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
   bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
   bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
   bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
   return float(bits) * 2.3283064365386963e-10f; // / 0x100000000
}

vec2 hammersley2D(unsigned int i, unsigned int N)
{
   return vec2(float(i) / float(N), radicalInverseVdC(i));
}

vec3 hemisphereSampleUniform(float u, float v)
{
   float phi = v * 2.0f * float(M_PI);
   float cosTheta = 1.0f - u;
   float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
   return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

vec3 hemisphereSampleCos(float u, float v)
{
   float phi = v * 2.0f * float(M_PI);
   float cosTheta = sqrt(1.0f - u);
   float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
   return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}


Voxelizer::Voxelizer(const RenderResources& render_resources, const RenderSettings& render_settings)
   : _rr(render_resources), _settings(render_settings)
{
   _texture_size = 128;

   _empty_framebuffer = createEmptyFramebuffer(ImageSize(_texture_size, _texture_size));
   _rasterize_voxels = createProgramFromFile("rasterize_voxels.glsl");
   _fill_occlusion_voxels = createProgramFromFile("fill_occlusion_voxels.glsl");
   _shade_voxels = createProgramFromFile("shade_voxels.glsl");
   _gather_gi_horizontal = createProgramFromFile("gather_gi.glsl", "BLUR_HORIZONTAL");
   _gather_gi_vertical = createProgramFromFile("gather_gi.glsl", "BLUR_VERTICAL");

   _debug_draw_voxels = createProgramFromFile("debug_draw_voxels.glsl");  
   _debug_voxels_wireframe = createBuffer(_texture_size * _texture_size * _texture_size * sizeof(vec3) * 24, GL_MAP_WRITE_BIT); 

   _voxels_gbuffer = createTexture3D(_texture_size, _texture_size, _texture_size, GL_RGBA16F);
   _voxels_occlusion = createTexture3D(_texture_size, _texture_size, _texture_size, GL_R8);
   _voxels_illumination = createTexture3D(_texture_size, _texture_size, _texture_size, GL_RGBA16F);
   

   auto real_rand = std::bind(std::uniform_real_distribution<float>(0.0, 1.0), std::mt19937());

   constexpr int hammersley_sample_count = SAMPLES_SIDE*SAMPLES_SIDE*RAY_COUNT;
   vec3 samples[hammersley_sample_count];
   for (int i = 0; i < hammersley_sample_count; ++i)
   {
      vec2 uv = hammersley2D(i, hammersley_sample_count);
      //samples[i] = hemisphereSampleCos(real_rand(), real_rand()); //hemisphereSampleCos(uv.x, uv.y);
      samples[i] = hemisphereSampleCos(uv.x, uv.y);
   }
   _hemisphere_samples = createTexture1D(hammersley_sample_count, GL_RGB16F, samples);

   float clear_color[] = { 0.0f, 0.0f, 0.0f, 0.0f };
   glClearTexImage(_voxels_gbuffer->id(), 0, GL_RGBA, GL_FLOAT, clear_color);
   
   float size = 8.0;
   _voxels_aabb.extend(-vec3(size));
   _voxels_aabb.extend(vec3(size));

   _trace_gi_rays = createProgramFromFile("trace_gi_rays.glsl");
   _gi_framebuffer = createFramebuffer(_rr.framebuffer_size, GL_RGB16F, 2);

   vec3* vertices = (vec3*)_debug_voxels_wireframe->map(GL_MAP_WRITE_BIT);

   vec3 scale;
   scale.x = 1.0f / float(_texture_size);
   scale.y = 1.0f / float(_texture_size);
   scale.z = 1.0f / float(_texture_size);
   for (int z = 0; z <= _texture_size - 1; z++)
   {
      for (int y = 0; y <= _texture_size - 1; y++)
      {
         for (int x = 0; x <= _texture_size - 1; x++)
         {
            vertices[0] = vec3(x, y, z) * scale;
            vertices[1] = vec3(x + 1, y, z) * scale;
            vertices[2] = vec3(x + 1, y, z) * scale;
            vertices[3] = vec3(x + 1, y + 1, z) * scale;
            vertices[4] = vec3(x + 1, y + 1, z) * scale;
            vertices[5] = vec3(x, y + 1, z) * scale;
            vertices[6] = vec3(x, y + 1, z) * scale;
            vertices[7] = vec3(x, y, z) * scale;
            vertices += 8;

            vertices[0] = vec3(x, y, z + 1) * scale;
            vertices[1] = vec3(x + 1, y, z + 1) * scale;
            vertices[2] = vec3(x + 1, y, z + 1) * scale;
            vertices[3] = vec3(x + 1, y + 1, z + 1) * scale;
            vertices[4] = vec3(x + 1, y + 1, z + 1) * scale;
            vertices[5] = vec3(x, y + 1, z + 1) * scale;
            vertices[6] = vec3(x, y + 1, z + 1) * scale;
            vertices[7] = vec3(x, y, z + 1) * scale;
            vertices += 8;

            vertices[0] = vec3(x, y, z) * scale;
            vertices[1] = vec3(x, y, z + 1) * scale;
            vertices[2] = vec3(x + 1, y, z) * scale;
            vertices[3] = vec3(x + 1, y, z + 1) * scale;
            vertices[4] = vec3(x + 1, y + 1, z) * scale;
            vertices[5] = vec3(x + 1, y + 1, z + 1) * scale;
            vertices[6] = vec3(x, y + 1, z) * scale;
            vertices[7] = vec3(x, y + 1, z + 1) * scale;
            vertices += 8;
         }
      }
   }
   _debug_voxels_wireframe->unmap();

   _debug_voxel_wireframe_vertex_source = std::make_unique<GLVertexSource>();
   _debug_voxel_wireframe_vertex_source->setVertexBuffer(*_debug_voxels_wireframe);
   _debug_voxel_wireframe_vertex_source->setPrimitiveType(GL_LINES);
   _debug_voxel_wireframe_vertex_source->setVertexCount(_texture_size * _texture_size * _texture_size * 24);
   _debug_voxel_wireframe_vertex_source->setVertexAttribute(0, 3, GL_FLOAT, GLSLVecType::vec);
   
}

Voxelizer::~Voxelizer() {}

void Voxelizer::bakeVoxels(RenderEngine* render_engine, const RenderData& render_data)
{
   _rr.voxelize_timer->start();
   GLDevice::bindFramebuffer(_empty_framebuffer.get(), 0);
   //GLDevice::bindFramebuffer(_rr.main_framebuffer.get(), 0);
   glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
   glDepthMask(GL_FALSE);

   GLDevice::bindImage(BI_VOXELS_GBUFFER_IMAGE, *_voxels_gbuffer, GL_WRITE_ONLY);
   GLDevice::bindProgram(*_rasterize_voxels);
   glViewport(0, 0, _texture_size, _texture_size);
   mat4 ortho_matrix_world[3];
   /*
   3 rasterization basis for ortho rendering (matrix_world_camera):
    ▲ Y            ▲ Z            ▲ Z    
    |              |              |
    |              |              |
    oㅡㅡ► X        oㅡㅡ► Y        oㅡㅡ► -X
    Z              X              Y
   */
   vec4 X = vec4(1.0, 0.0, 0.0, 0.0);
   vec4 Y = vec4(0.0, 1.0, 0.0, 0.0);
   vec4 Z = vec4(0.0, 0.0, 1.0, 0.0);
   vec4 O = vec4(0.0, 0.0, 0.0, 1.0);

   ortho_matrix_world[0] = ortho(_voxels_aabb.pmin.y, _voxels_aabb.pmax.y, _voxels_aabb.pmin.z, _voxels_aabb.pmax.z, _voxels_aabb.pmax.x, _voxels_aabb.pmin.x) * mat4( Z, X, Y, O); // compose with inverse(matrix_world_camera)
   ortho_matrix_world[1] = ortho(_voxels_aabb.pmax.x, _voxels_aabb.pmin.x, _voxels_aabb.pmin.z, _voxels_aabb.pmax.z, _voxels_aabb.pmax.y, _voxels_aabb.pmin.y) * mat4(-X, Z, Y, O);
   ortho_matrix_world[2] = ortho(_voxels_aabb.pmin.x, _voxels_aabb.pmax.x, _voxels_aabb.pmin.y, _voxels_aabb.pmax.y, _voxels_aabb.pmax.z, _voxels_aabb.pmin.z) * mat4( X, Y, Z, O);

   GLDevice::bindUniformMatrix4Array(BI_ORTHO_MATRICES, ortho_matrix_world, 3);
   render_engine->drawSurfaces(render_data);

   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
   glDepthMask(GL_TRUE);

   glMemoryBarrier(GL_ALL_BARRIER_BITS);
   
   GLDevice::bindProgram(*_fill_occlusion_voxels);
   GLDevice::bindImage(BI_VOXELS_GBUFFER_IMAGE, *_voxels_gbuffer, GL_READ_ONLY);
   GLDevice::bindImage(BI_VOXELS_OCCLUSION_IMAGE, *_voxels_occlusion, GL_WRITE_ONLY);
   glDispatchCompute(_texture_size / 4, _texture_size / 4, _texture_size / 4);
   //glMemoryBarrier(GL_ALL_BARRIER_BITS);
   GLDevice::bindProgram(*_shade_voxels);
   GLDevice::bindImage(BI_VOXELS_GBUFFER_IMAGE, *_voxels_gbuffer, GL_READ_ONLY);
   GLDevice::bindImage(BI_VOXELS_ILLUMINATION_IMAGE, *_voxels_illumination, GL_WRITE_ONLY);
   glDispatchCompute(_texture_size / 4, _texture_size / 4, _texture_size / 4);

   glMemoryBarrier(GL_ALL_BARRIER_BITS);
   _rr.voxelize_timer->stop();
}

void Voxelizer::debugDrawVoxels(const RenderData& render_data)
{
   if (!_settings.show_voxel_grid)
      return;

   GLDevice::bindTexture(BI_VOXELS_OCCLUSION_TEXTURE, *_voxels_occlusion, *_rr.samplers.linear_clampToEdge);

   GLDevice::bindProgram(*_debug_draw_voxels);   
   glUniform3fv(BI_VOXELS_AABB_PMIN, 1, value_ptr(_voxels_aabb.pmin));
   glUniform3fv(BI_VOXELS_AABB_PMAX, 1, value_ptr(_voxels_aabb.pmax));
   
   GLDevice::draw(*_debug_voxel_wireframe_vertex_source);
}

void Voxelizer::shadeVoxels(const RenderData& render_data)
{

}

void Voxelizer::traceGlobalIlluminationRays(const RenderData& render_data)
{
   /*float clear_color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
   glClearTexImage(_voxels_illumination->id(), 0, GL_RGBA, GL_FLOAT, clear_color);
   glMemoryBarrier(GL_ALL_BARRIER_BITS);*/
   _rr.raytrace_timer->start();
   // fixme glviewport
   GLDevice::bindFramebuffer(_gi_framebuffer.get(), 0);
   GLDevice::bindProgram(*_trace_gi_rays);
   GLDevice::bindTexture(BI_DEPTH_TEXTURE, _rr.main_framebuffer->attachedTexture(GL_DEPTH_ATTACHMENT), *_rr.samplers.nearest_clampToEdge);
   GLDevice::bindTexture(BI_VOXELS_ILLUMINATION_TEXTURE, *_voxels_illumination, *_rr.samplers.linear_clampToEdge);   
   GLDevice::bindTexture(BI_VOXELS_OCCLUSION_TEXTURE, *_voxels_occlusion, *_rr.samplers.linear_clampToEdge);
   GLDevice::bindTexture(BI_HEMISPHERE_SAMPLES_TEXTURE, *_hemisphere_samples, *_rr.samplers.nearest_clampToEdge);
   glUniform3fv(BI_VOXELS_AABB_PMIN, 1, value_ptr(_voxels_aabb.pmin));
   glUniform3fv(BI_VOXELS_AABB_PMAX, 1, value_ptr(_voxels_aabb.pmax));
   GLDevice::bindUniformMatrix4(BI_MATRIX_WORLD_PROJ, inverse(render_data.matrix_proj_world));
   GLDevice::draw(*_rr.fullscreen_triangle_source);

   gatherGlobalIllumination();
   _rr.raytrace_timer->stop();   
}

void Voxelizer::gatherGlobalIllumination()
{
   GLDevice::bindFramebuffer(_gi_framebuffer.get(), 1);
   GLDevice::bindTexture(BI_GATHER_GI_TEXTURE, _gi_framebuffer->attachedTexture(GL_COLOR_ATTACHMENT0), *_rr.samplers.nearest_clampToEdge);
   GLDevice::bindProgram(*_gather_gi_horizontal);
   GLDevice::draw(*_rr.fullscreen_triangle_source);

   GLDevice::bindFramebuffer(_gi_framebuffer.get(), 0);
   GLDevice::bindTexture(BI_GATHER_GI_TEXTURE, _gi_framebuffer->attachedTexture(GL_COLOR_ATTACHMENT1), *_rr.samplers.nearest_clampToEdge);
   GLDevice::bindProgram(*_gather_gi_vertical);
   GLDevice::draw(*_rr.fullscreen_triangle_source);
}

void Voxelizer::bindGlobalIlluminationTexture()
{
   GLDevice::bindTexture(BI_GI_TEXTURE, _gi_framebuffer->attachedTexture(GL_COLOR_ATTACHMENT0), *_rr.samplers.nearest_clampToEdge);
}

}