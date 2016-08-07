#include "RenderEngine.h"

#include <algorithm>
#include <chrono>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <iterator>
#include <iostream>

#include "GLDevice.h"
#include "GLBuffer.h"
#include "GLSampler.h"
#include "GLProgram.h"
#include "ShadeTreeMaterial.h"
#include "RenderMesh.h"
#include "RenderResources.h"
#include "CubemapFiltering.h"
#include "GLTexture.h"
#include "glsl_global_defines.h"
#include "OceanMaterial.h"
#include "BackgroundSky.h"
#include "GLFramebuffer.h"
#include "FilmPostProcessor.h"
#include "GLGPUTimer.h"
#include "Skeleton.h"
#include "matrix_math.h"
#include "GLFormats.h"
#include "AnimationPlayer.h"
#include "TransformHierarchy.h"
#include "stl_helpers.h"
#include "SSAORenderer.h"

namespace yare {

using namespace glm;

struct SurfaceUniforms
{
   mat4 matrix_proj_local;
   mat4 normal_matrix_world_local;
   mat4 matrix_world_local;
};

struct SceneUniforms
{
   mat4 matrix_proj_world; 
   vec3 eye_position;
   float time;

   float delta_time;
   float znear;
   float zfar;
   float proj_coeff_11;
   float tessellation_edges_per_screen_height;

};

struct LightSSBO
{
   vec3 color;
   int type;      
   union
   {
      struct Sphere
      {
         glm::vec3 position;
         float size;
         float padding[8];
      } sphere;
      struct Rectangle
      {
         glm::vec3 position;
         float padding;
         glm::vec3 direction_x;
         float size_x;
         glm::vec3 direction_y;
         float size_y;
      } rectangle;
      struct Sun
      {
         glm::vec3 direction;
         float size;
         float padding[8];
      } sun;
      struct Spot
      {
         glm::vec3 position;
         float cos_half_angle;
         glm::vec3 direction;
         float angle_smooth;
         float padding[4];
      } spot;
   };
};

RenderEngine::RenderEngine(const ImageSize& framebuffer_size)
   : _scene()
   , render_resources(new RenderResources(framebuffer_size))
   , cubemap_converter(new CubemapFiltering(*render_resources))
   , background_sky(new BackgroundSky(*render_resources))
   , film_processor(new FilmPostProcessor(*render_resources))
   , ssao_renderer(new SSAORenderer(*render_resources))
{    
   _z_pass_render_program = createProgramFromFile("z_pass_render.glsl");
}

RenderEngine::~RenderEngine()
{   
}

void RenderEngine::offlinePrepareScene()
{
   bindAnimationCurvesToTargets(_scene, *_scene.animation_player);
   
   _sortSurfacesByMaterial();   
   
   int uniform_buffer_align_size;
   glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uniform_buffer_align_size);
   _surface_uniforms_size = GLFormats::alignSize(sizeof(SurfaceUniforms), uniform_buffer_align_size);
      
   int surface_count = int(_scene.surfaces.size());
   _surface_uniforms = createDynamicBuffer(_surface_uniforms_size * surface_count);
   _scene_uniforms = createDynamicBuffer(sizeof(SceneUniforms));
   _createSceneLightsBuffer();
   
   _scene.render_data[0].main_view_surface_data.resize(_scene.surfaces.size());
   _scene.render_data[1].main_view_surface_data.resize(_scene.surfaces.size());

   for (int i = 0; i < _scene.surfaces.size(); ++i)
   {
      auto& surface = _scene.surfaces[i];
      surface.vertex_source_for_material = createVertexSource(*surface.mesh, surface.material->requiredMeshFields(_scene.surfaces[i].material_variant), surface.material->hasTessellation());
      surface.vertex_source_position_normal = createVertexSource(*surface.mesh, int(MeshFieldName::Position)| int(MeshFieldName::Normal), surface.material->hasTessellation());// TODO rename
   }
}

void RenderEngine::updateScene(RenderData& render_data)
{
   static float last_update_time = 0.0f;
   static auto start = std::chrono::steady_clock::now();
   auto now = std::chrono::steady_clock::now();
   float time_lapse = std::chrono::duration<float>(now - start).count();
      
   _scene.animation_player->evaluateAndApplyToTargets(24.0f*time_lapse);
   _scene.transform_hierarchy->updateNodesWorldToLocalMatrix();
   for (const auto& skeleton : _scene.skeletons)
      skeleton->update();

   _updateRenderMatrices(render_data);
   _sortSurfacesByDistanceToCamera(render_data);
   _updateUniformBuffers(render_data, time_lapse, time_lapse - last_update_time);
   
   last_update_time = time_lapse;
}

void RenderEngine::renderScene(const RenderData& render_data)
{   
   GLDevice::bindDefaultDepthStencilState();   
   GLDevice::bindDefaultColorBlendState();
   GLDevice::bindDefaultRasterizationState();
   glPatchParameteri(GL_PATCH_VERTICES, 3);
   
   _renderSurfaces(render_data);
   film_processor->developFilm();
        
   GLGPUTimer::swapCounters();
}

void RenderEngine::presentDebugTexture()
{
   glViewport(0, 0, 1024, 1024);  
   GLDevice::bindProgram(*render_resources->present_texture);
   //GLDevice::bindTexture(BI_INPUT_TEXTURE, *cubemap_converter->_parallel_reduce_result, *render_resources->sampler_bilinear_clampToEdge);
   //GLDevice::bindTexture(BI_INPUT_TEXTURE, *_scene.sky_latlong, *render_resources->sampler_bilinear_clampToEdge);
   GLDevice::draw(*render_resources->fullscreen_triangle_source);
}

static void _debugDrawBasis(const mat4x3& mat)
{
   glLineWidth(3);
   glBegin(GL_LINES);
   glColor3f(1.0, 0.0, 0.0);
   glVertex3fv(glm::value_ptr(mat[0] + mat[3]));
   glVertex3fv(glm::value_ptr(mat[3]));
   glColor3f(0.0, 1.0, 0.0);
   glVertex3fv(glm::value_ptr(mat[1] + mat[3]));
   glVertex3fv(glm::value_ptr(mat[3]));
   glColor3f(0.0, 0.0, 1.0);
   glVertex3fv(glm::value_ptr(mat[2] + mat[3]));
   glVertex3fv(glm::value_ptr(mat[3]));
   glEnd();
}

static void _debugSetMVP(const mat4x3& mat)
{
   glUseProgram(0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glLoadMatrixf(glm::value_ptr(mat));//_scene.render_data[0].matrix_view_world
}

void RenderEngine::_bindSceneUniforms()
{
   static int counter = 0;
   const auto & diffuse_cubemap = counter < 120 ? *_scene.sky_diffuse_cubemap : *_scene.sky_diffuse_cubemap_sh;
   GLDevice::bindTexture(BI_SKY_CUBEMAP, *_scene.sky_cubemap, *(render_resources->samplers.mipmap_clampToEdge));
   GLDevice::bindTexture(BI_SKY_DIFFUSE_CUBEMAP, diffuse_cubemap, *render_resources->samplers.mipmap_clampToEdge);
   counter++;
   if (counter == 240)
      counter = 0;
   glBindBufferRange(GL_UNIFORM_BUFFER, BI_SCENE_UNIFORMS, _scene_uniforms->id(),
                     _scene_uniforms->getRenderSegmentOffset(), _scene_uniforms->segmentSize());
}

void RenderEngine::_bindSurfaceUniforms(int suface_index, const SurfaceInstance& surface)
{
   if (surface.skeleton)
   {
      const GLDynamicBuffer& skinning_ssbo = surface.skeleton->skinningPalette();
      glBindBufferRange(GL_SHADER_STORAGE_BUFFER, BI_SKINNING_PALETTE_SSBO, skinning_ssbo.id(),
                        skinning_ssbo.getRenderSegmentOffset(), skinning_ssbo.segmentSize());
   }

   glBindBufferRange(GL_UNIFORM_BUFFER, BI_SURFACE_DYNAMIC_UNIFORMS,
                     _surface_uniforms->id(),
                     _surface_uniforms->getRenderSegmentOffset() + _surface_uniforms_size*suface_index,
                     _surface_uniforms_size);
}

void RenderEngine::_renderSurfaces(const RenderData& render_data)
{
   glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
   glViewport(0, 0, render_resources->main_framebuffer->width(), render_resources->main_framebuffer->height());
   
   _bindSceneUniforms();   
   
   // Z Pass   
   render_resources->z_pass_timer->start();
   GLDevice::bindFramebuffer(render_resources->main_framebuffer.get(), 1);
   glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
   glClear(GL_DEPTH_BUFFER_BIT);
   GLDevice::bindProgram(*_z_pass_render_program);
   for (auto& sorted_surface : render_data.surfaces_sorted_by_distance)
   {
      int surface_index = sorted_surface.surface_index;
      const auto& surface = _scene.surfaces[surface_index];
      _bindSurfaceUniforms(surface_index, surface);
      
      GLDevice::draw(*surface.vertex_source_position_normal); // TODO dont render for ocean and animated geometry
   }
   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
   render_resources->z_pass_timer->stop();

   // SSAO render
   ssao_renderer->render(render_data);
   auto& ssao_texture = render_resources->ssao_framebuffer->attachedTexture(GL_COLOR_ATTACHMENT0);
   GLDevice::bindTexture(BI_SSAO_TEXTURE, ssao_texture, *render_resources->samplers.nearest_clampToEdge);
        
   // Material Pass
   render_resources->material_pass_timer->start();
   GLDevice::bindFramebuffer(render_resources->main_framebuffer.get(), 0);
   //glClear(GL_DEPTH_BUFFER_BIT);
   _renderSurfacesMaterial(_scene.opaque_surfaces);
   render_resources->material_pass_timer->stop();

   render_resources->background_timer->start();
   background_sky->render();
   render_resources->background_timer->stop();
   GLDevice::bindColorBlendState({ GLBlendingMode::ModulateAdd });
   _renderSurfacesMaterial(_scene.transparent_surfaces);
   GLDevice::bindDefaultColorBlendState();

   std::cout << "z:"<< render_resources->z_pass_timer->elapsedTimeInMs() 
             << " ssao:"<<  render_resources->ssao_timer->elapsedTimeInMs() 
             << " mat:" << render_resources->material_pass_timer->elapsedTimeInMs()
             << " b:" << render_resources->background_timer->elapsedTimeInMs()
             << std::endl;

}

void RenderEngine::_renderSurfacesMaterial(SurfaceRange surfaces)
{
   int surface_index = int(std::distance(surfaces.begin(), _scene.surfaces.begin()));
   const GLProgram* current_program = nullptr;

   for (const auto& surface : surfaces)
   {
      _bindSurfaceUniforms(surface_index++, surface);

      if (surface.material_program != current_program)
      {
         GLDevice::bindProgram(*surface.material_program);
         surface.material->bindTextures();
         current_program = surface.material_program;
      }      

      GLDevice::draw(*surface.vertex_source_for_material);
   }
}

void RenderEngine::_createSceneLightsBuffer()
{  
   _lights_ssbo = createBuffer(sizeof(LightSSBO)*_scene.lights.size()+sizeof(glm::vec4), GL_MAP_WRITE_BIT);

   char* data = (char*)_lights_ssbo->map(GL_MAP_WRITE_BIT);
   data += sizeof(glm::vec4);
   for (const auto& light : _scene.lights)
   {      
      LightSSBO* buffer_light = (LightSSBO*)data;
      buffer_light->type = int(light.type);      
      switch (light.type)
      {
         case LightType::Sphere:
            buffer_light->color = light.color*light.strength; // light power is stored in color (Watt)
            buffer_light->sphere.position = light.world_to_local_matrix[3];
            buffer_light->sphere.size = light.sphere.size;
            break;
         case LightType::Rectangle:
            buffer_light->color = light.color*light.strength/(light.rectangle.size_x*light.rectangle.size_y); // radiant exitance is stored in color (Watt/m^2)
            buffer_light->rectangle.position = light.world_to_local_matrix[3];
            buffer_light->rectangle.direction_x = normalize(light.world_to_local_matrix[0]);
            buffer_light->rectangle.direction_y = normalize(light.world_to_local_matrix[1]);
            buffer_light->rectangle.size_x = light.rectangle.size_x;
            buffer_light->rectangle.size_y = light.rectangle.size_y; 
            break;
         case LightType::Sun:
            buffer_light->color = light.color*light.strength; // incident radiance is stored in color (Watt/m^2/ster)
            buffer_light->sun.direction = light.world_to_local_matrix[2];
            buffer_light->sun.size = light.sun.size;
            break;
         case LightType::Spot:
            buffer_light->color = light.color*light.strength; // light power is stored in color (Watt)
            buffer_light->spot.position = light.world_to_local_matrix[3];
            buffer_light->spot.direction = light.world_to_local_matrix[2];
            buffer_light->spot.cos_half_angle = cos(light.spot.angle/2.0f);
            buffer_light->spot.angle_smooth = light.spot.angle_blend;
            break;
      }

      data += sizeof(LightSSBO);
   }
   _lights_ssbo->unmap();
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BI_LIGHTS_SSBO, _lights_ssbo->id());
}


void RenderEngine::_sortSurfacesByDistanceToCamera(RenderData& render_data)
{
   int surface_count = int(render_data.main_view_surface_data.size());
   auto& surfaces_sorted_by_distance = render_data.surfaces_sorted_by_distance;   
   render_data.surfaces_sorted_by_distance.resize(surface_count);

   for (int i = 0; i < surface_count; ++i)
   {      
      float distance_to_camera = (render_data.main_view_surface_data[i].matrix_proj_local * vec4(_scene.surfaces[i].center_in_local_space, 1.0)).w;
      surfaces_sorted_by_distance[i] = SurfaceDistanceSortItem{ i, distance_to_camera };
   }

   auto sort_by_distance = [](SurfaceDistanceSortItem& a, SurfaceDistanceSortItem& b)
   {
      return a.distance < b.distance;
   };
   std::sort(RANGE(surfaces_sorted_by_distance), sort_by_distance);
}

struct SurfaceMaterialSortItem
{
   int surface_instance_index;
   GLuint program_id;
   bool is_transparent;
};

void RenderEngine::_sortSurfacesByMaterial()
{
   auto& surfaces = _scene.surfaces;
   std::vector<SurfaceMaterialSortItem> temp_surface_list;
   for (int i = 0; i < int(surfaces.size()); ++i)
   {
      const auto& surface = surfaces[i];
      SurfaceMaterialSortItem item;
      item.surface_instance_index = i;
      item.program_id = surface.material_program->id();
      item.is_transparent = surface.material->isTransparent();

      temp_surface_list.push_back(item);
      
   }

   auto sort_by_material = [](SurfaceMaterialSortItem& a, SurfaceMaterialSortItem& b)
   {
      if (a.is_transparent != b.is_transparent)
         return b.is_transparent;

      return a.program_id < b.program_id;
   };
   std::sort(RANGE(temp_surface_list), sort_by_material);

   std::vector<SurfaceInstance> sorted_surfaces;
   for (const auto& item : temp_surface_list)
   {
      sorted_surfaces.push_back(surfaces[item.surface_instance_index]);
   }
         
   surfaces = std::move(sorted_surfaces);

   auto first_transparent_surface_it = std::find_if(RANGE(surfaces), [](SurfaceInstance& surface)
   {
      return surface.material->isTransparent();
   });
   _scene.opaque_surfaces = SurfaceRange(surfaces.begin(), first_transparent_surface_it);
   _scene.transparent_surfaces = SurfaceRange(first_transparent_surface_it, surfaces.end());
}

void RenderEngine::_updateUniformBuffers(const RenderData& render_data, float time, float delta_time)
{
   char* buffer = (char*)_surface_uniforms->getUpdateSegmentPtr(); // hopefully OpenGL will be done using that range at that time (I could use a fence to enforce it but meh I don't care)
   for (int i = 0; i < render_data.main_view_surface_data.size(); ++i)
   {
      ((SurfaceUniforms*)buffer)->matrix_proj_local = render_data.main_view_surface_data[i].matrix_proj_local;
      ((SurfaceUniforms*)buffer)->normal_matrix_world_local = render_data.main_view_surface_data[i].normal_matrix_world_local;
      ((SurfaceUniforms*)buffer)->matrix_world_local = render_data.main_view_surface_data[i].matrix_world_local;
      buffer += _surface_uniforms_size;
   }

   SceneUniforms* scene_uniforms = (SceneUniforms*)_scene_uniforms->getUpdateSegmentPtr();
   scene_uniforms->eye_position = _scene.camera.point_of_view.from;
   scene_uniforms->matrix_proj_world = render_data.matrix_proj_world;
   scene_uniforms->time = time;
   scene_uniforms->delta_time = delta_time;
   scene_uniforms->znear = _scene.camera.frustum.near;
   scene_uniforms->zfar = _scene.camera.frustum.far;
   scene_uniforms->proj_coeff_11 = render_data.matrix_proj_view[1][1];
   float tessellation_pixels_per_edge = 30.0;
   scene_uniforms->tessellation_edges_per_screen_height = render_resources->framebuffer_size.height / tessellation_pixels_per_edge;
}

void RenderEngine::_updateRenderMatrices(RenderData& render_data)
{
   const float znear = _scene.camera.frustum.near;
   const float zfar = _scene.camera.frustum.far;
   auto matrix_view_world = lookAt(_scene.camera.point_of_view.from, _scene.camera.point_of_view.to, _scene.camera.point_of_view.up);
   auto matrix_projection = perspective(3.14f / 2.0f, render_resources->framebuffer_size.ratio(), znear, zfar);
   render_data.matrix_proj_world = matrix_projection * matrix_view_world;
   render_data.matrix_view_world = matrix_view_world;
   render_data.matrix_view_proj = inverse(matrix_projection);
   render_data.matrix_proj_view = matrix_projection;

   for (int i = 0; i < render_data.main_view_surface_data.size(); ++i)
   {
      mat4 matrix_world_local = _scene.transform_hierarchy->nodeWorldToLocalMatrix(_scene.surfaces[i].transform_node_index);
      render_data.main_view_surface_data[i].matrix_world_local = matrix_world_local;
      render_data.main_view_surface_data[i].normal_matrix_world_local = mat3(transpose(inverse(matrix_world_local)));
      render_data.main_view_surface_data[i].matrix_proj_local = render_data.matrix_proj_world * matrix_world_local;
   }
}


} // namespace yare