﻿#include "Voxelizer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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

Voxelizer::Voxelizer(const RenderResources& render_resources)
   : _rr(render_resources)
{
   _texture_size = 64;

   _empty_framebuffer = createEmptyFramebuffer(ImageSize(_texture_size, _texture_size));
   _rasterize_voxels = createProgramFromFile("rasterize_voxels.glsl");


   _debug_draw_voxels = createProgramFromFile("debug_draw_voxels.glsl");  
   _debug_voxels_wireframe = createBuffer(_texture_size * _texture_size * _texture_size * sizeof(vec3) * 24, GL_MAP_WRITE_BIT); 

   _voxels = createTexture3D(_texture_size, _texture_size, _texture_size, GL_RGBA16F);

   float clear_color[] = { 0.0f, 0.0f, 0.0f, 0.0f };
   glClearTexImage(_voxels->id(), 0, GL_RGBA, GL_FLOAT, clear_color);
   
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

   GLDevice::bindImage(BI_VOXELS_IMAGE, *_voxels, GL_WRITE_ONLY);
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
   _rr.voxelize_timer->stop();
}

void Voxelizer::debugDrawVoxels(const RenderData& render_data)
{
   GLDevice::bindProgram(*_debug_draw_voxels);   
   glUniform3fv(BI_VOXELS_AABB_PMIN, 1, value_ptr(_voxels_aabb.pmin));
   glUniform3fv(BI_VOXELS_AABB_PMAX, 1, value_ptr(_voxels_aabb.pmax));
   GLDevice::bindTexture(BI_VOXELS_TEXTURE, *_voxels, *_rr.samplers.linear_clampToEdge);
   GLDevice::draw(*_debug_voxel_wireframe_vertex_source);
}

void Voxelizer::traceGlobalIlluminationRays(const RenderData& render_data)
{
   _rr.raytrace_timer->start();
   // fixme glviewport
   GLDevice::bindFramebuffer(_gi_framebuffer.get(), 0);
   GLDevice::bindProgram(*_trace_gi_rays);
   GLDevice::bindTexture(BI_DEPTH_TEXTURE, _rr.main_framebuffer->attachedTexture(GL_DEPTH_ATTACHMENT), *_rr.samplers.nearest_clampToEdge);
   GLDevice::bindTexture(BI_VOXELS_TEXTURE, *_voxels, *_rr.samplers.linear_clampToEdge);
   glUniform3fv(BI_VOXELS_AABB_PMIN, 1, value_ptr(_voxels_aabb.pmin));
   glUniform3fv(BI_VOXELS_AABB_PMAX, 1, value_ptr(_voxels_aabb.pmax));
   GLDevice::bindUniformMatrix4(BI_MATRIX_WORLD_PROJ, inverse(render_data.matrix_proj_world));
   GLDevice::draw(*_rr.fullscreen_triangle_source);
   _rr.raytrace_timer->stop();
}

void Voxelizer::gatherGlobalIllumination()
{
   /*GLDevice::bindFramebuffer(_gi_framebuffer.get(), 1);
   GLDevice::bindTexture(BI_GI_TEXTURE, _rr.main_framebuffer->attachedTexture(GL_COLOR_ATTACHMENT0), *_rr.samplers.nearest_clampToEdge);
   GLDevice::bindProgram(*_gather_gi);
   GLDevice::draw(*_rr.fullscreen_triangle_source);*/
}

void Voxelizer::bindGlobalIlluminationTexture()
{
   GLDevice::bindTexture(BI_GI_TEXTURE, _gi_framebuffer->attachedTexture(GL_COLOR_ATTACHMENT0), *_rr.samplers.nearest_clampToEdge);
}

}