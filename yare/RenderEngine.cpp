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
#include "ClusteredLightCuller.h"

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
   vec3 ao_volume_bound_min;
   float delta_time;
   vec3 ao_volume_size;
   float proj_coeff_11;

   vec3 sdf_volume_bound_min;
   float znear;
   vec3 sdf_volume_size;
   float zfar;
   ivec4 viewport;
   float tessellation_edges_per_screen_height;
   float cluster_distribution_factor;
};

struct LightSphereSSBO
{
   vec3 color;
   float size;
   glm::vec3 position;
   float radius;
};

struct LightSpotSSBO
{
   vec3 color;
   float angle_smooth;
   glm::vec3 position;
   float cos_half_angle;
   glm::vec3 direction;
   float radius;
};

struct LightRectangleSSBO
{
   vec3 color;
   float size_x;
   glm::vec3 position;
   float size_y;
   glm::vec3 direction_x;
   float radius;
   glm::vec3 direction_y;
   int padding1;
};

struct LightSunSSBO
{
   vec3 color;
   float size;
   glm::vec3 direction;
   int padding;
};



RenderEngine::RenderEngine(const ImageSize& framebuffer_size)
   : _scene()
   , render_resources(new RenderResources(framebuffer_size))
   , cubemap_converter(new CubemapFiltering(*render_resources))
   , background_sky(new BackgroundSky(*render_resources))
   , film_processor(new FilmPostProcessor(*render_resources))
   , ssao_renderer(new SSAORenderer(*render_resources))
   , clustered_light_culler(new ClusteredLightCuller(*render_resources, _settings))
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
   _computeLightsRadius();
   _createSceneLightsBuffer();
   
   _scene.render_data[0].main_view_surface_data.resize(_scene.surfaces.size());
   _scene.render_data[1].main_view_surface_data.resize(_scene.surfaces.size());

   for (int i = 0; i < _scene.surfaces.size(); ++i)
   {
      auto& surface = _scene.surfaces[i];
      surface.vertex_source_for_material = createVertexSource(*surface.mesh, surface.material->requiredMeshFields(_scene.surfaces[i].material_variant), surface.material->hasTessellation());
      surface.vertex_source_position_normal = createVertexSource(*surface.mesh, int(MeshFieldName::Position)| int(MeshFieldName::Normal), surface.material->hasTessellation());// TODO rename
   }

   _scene.transform_hierarchy->updateNodesWorldToLocalMatrix();

   
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
   
   clustered_light_culler->buildLightLists(_scene, render_data);

   last_update_time = time_lapse;
}

void RenderEngine::renderScene(const RenderData& render_data)
{   
   GLDevice::bindDefaultDepthStencilState();   
   GLDevice::bindDefaultColorBlendState();
   GLDevice::bindDefaultRasterizationState();
   glPatchParameteri(GL_PATCH_VERTICES, 3);
   
   clustered_light_culler->updateLightListHeadTexture();

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
   
   if (_scene.ao_volume)
      GLDevice::bindTexture(BI_AO_VOLUME, *_scene.ao_volume->ao_texture, *render_resources->samplers.mipmap_clampToEdge);
   
   if (_scene.sdf_volume)
      GLDevice::bindTexture(BI_SDF_VOLUME, *_scene.sdf_volume->sdf_texture, *render_resources->samplers.mipmap_clampToEdge);

   counter++;
   if (counter == 240)
      counter = 0;
   glBindBufferRange(GL_UNIFORM_BUFFER, BI_SCENE_UNIFORMS, _scene_uniforms->id(),
                     _scene_uniforms->getRenderSegmentOffset(), _scene_uniforms->segmentSize());

   clustered_light_culler->bindLightLists();
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

   clustered_light_culler->drawClusterGrid(render_data, _settings.x + _settings.y*32 + _settings.z*(32*32));

   render_resources->background_timer->start();
   background_sky->render(render_data);
   render_resources->background_timer->stop();
   GLDevice::bindColorBlendState({ GLBlendingMode::ModulateAdd });
   _renderSurfacesMaterial(_scene.transparent_surfaces);
   GLDevice::bindDefaultColorBlendState();
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
         glUniform1f(42, _settings.bias);         
         GLDevice::bindUniformMatrix4(43, clustered_light_culler->_debug_render_data.matrix_proj_world);
         surface.material->bindTextures();
         current_program = surface.material_program;
      }      

      GLDevice::draw(*surface.vertex_source_for_material);
   }
}

void RenderEngine::_createSceneLightsBuffer()
{  
   int sphere_light_count = 0;
   int spot_light_count = 0;
   int rectangle_light_count = 0;
   int sun_light_count = 0;
   for (const auto& light : _scene.lights)
   {
      switch (light.type)
      {
      case LightType::Sphere: sphere_light_count++; break;
      case LightType::Spot: spot_light_count++; break;
      case LightType::Rectangle: rectangle_light_count++; break;
      case LightType::Sun: sun_light_count++; break;      
      }
   }

   _sphere_lights_ssbo = createBuffer(sizeof(LightSphereSSBO)*sphere_light_count + sizeof(vec4), GL_MAP_WRITE_BIT);
   _spot_lights_ssbo = createBuffer(sizeof(LightSpotSSBO)*spot_light_count + sizeof(vec4), GL_MAP_WRITE_BIT);
   _rectangle_lights_ssbo = createBuffer(sizeof(LightRectangleSSBO)*rectangle_light_count + sizeof(vec4), GL_MAP_WRITE_BIT);
   _sun_lights_ssbo = createBuffer(sizeof(LightSunSSBO)*sun_light_count + sizeof(vec4), GL_MAP_WRITE_BIT);

   char* sphere_data = (char*)_sphere_lights_ssbo->map(GL_MAP_WRITE_BIT);
   *((ivec3*)sphere_data) = clustered_light_culler->clustersDimensions();
   sphere_data += sizeof(vec4);
   char* spot_data = (char*)_spot_lights_ssbo->map(GL_MAP_WRITE_BIT);
   spot_data += sizeof(vec4);
   char* rectangle_data = (char*)_rectangle_lights_ssbo->map(GL_MAP_WRITE_BIT);
   rectangle_data += sizeof(vec4);
   char* sun_data = (char*)_sun_lights_ssbo->map(GL_MAP_WRITE_BIT);
   sun_data += sizeof(vec4);

   for (const auto& light : _scene.lights)
   {
      switch (light.type)
      {
         case LightType::Sphere:
         {
            LightSphereSSBO* buffer_light = (LightSphereSSBO*)sphere_data;
            buffer_light->color = light.color*light.strength; // light power is stored in color (Watt)
            buffer_light->position = light.world_to_local_matrix[3];
            buffer_light->size = light.sphere.size;
            buffer_light->radius = light.radius;
            sphere_data += sizeof(LightSphereSSBO);
            break;
         }
         case LightType::Rectangle:
         {
            LightRectangleSSBO* buffer_light = (LightRectangleSSBO*)rectangle_data;
            buffer_light->color = light.color*light.strength / (light.rectangle.size_x*light.rectangle.size_y); // radiant exitance is stored in color (Watt/m^2)
            buffer_light->position = light.world_to_local_matrix[3];
            buffer_light->direction_x = normalize(light.world_to_local_matrix[0]);
            buffer_light->direction_y = normalize(light.world_to_local_matrix[1]);
            buffer_light->size_x = light.rectangle.size_x;
            buffer_light->size_y = light.rectangle.size_y;
            buffer_light->radius = light.radius;
            rectangle_data += sizeof(LightRectangleSSBO);
            break;
         }
         case LightType::Sun:
         {
            LightSunSSBO* buffer_light = (LightSunSSBO*)sun_data;
            buffer_light->color = light.color*light.strength; // incident radiance is stored in color (Watt/m^2/ster)
            buffer_light->direction = light.world_to_local_matrix[2];
            buffer_light->size = light.sun.size;
            sun_data += sizeof(LightSunSSBO);
            break;
         }
         case LightType::Spot:
         {
            LightSpotSSBO* buffer_light = (LightSpotSSBO*)spot_data;
            buffer_light->color = light.color*light.strength; // light power is stored in color (Watt)
            buffer_light->position = light.world_to_local_matrix[3];
            buffer_light->direction = light.world_to_local_matrix[2];
            buffer_light->cos_half_angle = cos(light.spot.angle / 2.0f);
            buffer_light->angle_smooth = light.spot.angle_blend;
            buffer_light->radius = light.radius;
            spot_data += sizeof(LightSpotSSBO);
            break;
         }
      }
   }

   _sphere_lights_ssbo->unmap();
   _rectangle_lights_ssbo->unmap();
   _spot_lights_ssbo->unmap();
   _sun_lights_ssbo->unmap();
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BI_SPHERE_LIGHTS_SSBO, _sphere_lights_ssbo->id());
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BI_SPOT_LIGHTS_SSBO, _spot_lights_ssbo->id());
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BI_RECTANGLE_LIGHTS_SSBO, _rectangle_lights_ssbo->id());
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BI_SUN_LIGHTS_SSBO, _sun_lights_ssbo->id());  
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
   scene_uniforms->cluster_distribution_factor = _settings.cluster_z_distribution_factor;

   if (_scene.ao_volume)
   {
      scene_uniforms->ao_volume_size = _scene.ao_volume->size;
      scene_uniforms->ao_volume_bound_min = _scene.ao_volume->position - 0.5f*_scene.ao_volume->size;
   }

   if (_scene.sdf_volume)
   {
      scene_uniforms->sdf_volume_size = _scene.sdf_volume->size;
      scene_uniforms->sdf_volume_bound_min = _scene.sdf_volume->position - 0.5f*_scene.sdf_volume->size;
   }

   scene_uniforms->viewport = ivec4(0, 0, render_resources->main_framebuffer->width(), render_resources->main_framebuffer->height());
}

static Frustum _frustum(float fovy, float aspect, float znear, float zfar)
{
   Frustum result;
   result.near = znear;
   result.far = zfar;
   result.top = znear * tan(fovy*0.5f);
   result.bottom = -result.top;
   result.right = result.top * aspect;
   result.left = -result.right;

   return result;
}

void RenderEngine::_updateRenderMatrices(RenderData& render_data)
{
   _scene.camera.frustum = _frustum(3.14f / 2.0f, render_resources->framebuffer_size.ratio(), 0.05f, 200.0f);
   render_data.frustum = _scene.camera.frustum;

   auto matrix_view_world = lookAt(_scene.camera.point_of_view.from, _scene.camera.point_of_view.to, _scene.camera.point_of_view.up);

   const Frustum& f = _scene.camera.frustum;
   mat4 matrix_projection = frustum(f.left, f.right, f.bottom, f.top, f.near, f.far);
   render_data.matrix_proj_world = matrix_projection * matrix_view_world;
   render_data.matrix_view_world = matrix_view_world;
   render_data.matrix_view_proj = inverse(matrix_projection);
   render_data.matrix_proj_view = matrix_projection;

   for (int i = 0; i < render_data.main_view_surface_data.size(); ++i)
   {
      mat4 matrix_world_local = _scene.transform_hierarchy->nodeWorldToLocalMatrix(_scene.surfaces[i].transform_node_index);
      render_data.main_view_surface_data[i].matrix_world_local = matrix_world_local;
      render_data.main_view_surface_data[i].normal_matrix_world_local = normalMatrix(matrix_world_local);
      render_data.main_view_surface_data[i].matrix_proj_local = render_data.matrix_proj_world * matrix_world_local;
   }
}


vec4 _normalizePlane(vec4 plane)
{
   return plane / vec4(length(vec3(plane.xyz)));
}

void RenderEngine::_computeLightsRadius()
{
   for (auto& light : _scene.lights)
   {
      if (light.type == LightType::Sun)
         continue;
      
      light.radius =  sqrtf(light.strength / (_settings.light_contribution_threshold * 4.0f * float(M_PI)));
   }

   for (auto& light : _scene.lights)
   {
      removeScaling(light.world_to_local_matrix);
      
      if (light.type == LightType::Sphere)
      {
         light.frustum_planes_in_local[0] = vec4(1.0, 0.0, 0.0, light.radius);
         light.frustum_planes_in_local[1] = vec4(-1.0, 0.0, 0.0, light.radius);
         light.frustum_planes_in_local[2] = vec4(0.0, 1.0, 0.0, light.radius);
         light.frustum_planes_in_local[3] = vec4(0.0, -1.0, 0.0, light.radius);
         light.frustum_planes_in_local[4] = vec4(0.0, 0.0, 1.0, light.radius);
         light.frustum_planes_in_local[5] = vec4(0.0, 0.0, -1.0, light.radius);
      }
      else if (light.type == LightType::Spot)
      {
         light.radius = 9.0f;
         mat4 matrix_proj_spot = transpose(perspective(light.spot.angle, 1.0f, light.radius*0.1f, light.radius));

         light.frustum_planes_in_local[int(ClippingPlane::Left)] = _normalizePlane(matrix_proj_spot[0] + matrix_proj_spot[3]);
         light.frustum_planes_in_local[int(ClippingPlane::Right)] = _normalizePlane(-matrix_proj_spot[0] + matrix_proj_spot[3]);
         light.frustum_planes_in_local[int(ClippingPlane::Bottom)] = _normalizePlane(matrix_proj_spot[1] + matrix_proj_spot[3]);
         light.frustum_planes_in_local[int(ClippingPlane::Top)] = _normalizePlane(-matrix_proj_spot[1] + matrix_proj_spot[3]);
         light.frustum_planes_in_local[int(ClippingPlane::Far)] = _normalizePlane(-matrix_proj_spot[2] + matrix_proj_spot[3]);         
      }
      else if (light.type == LightType::Rectangle)
      {         
         light.radius = 5.0f;
         float depth = light.radius;
         float width = light.radius;
         float height = light.radius;
         light.rectangle.bounds_width  = width;
         light.rectangle.bounds_height = height;
         light.rectangle.bounds_depth  = depth;
                  
         light.frustum_planes_in_local[0] = vec4(1.0, 0.0, 0.0, width);
         light.frustum_planes_in_local[1] = vec4(-1.0, 0.0, 0.0, width);
         light.frustum_planes_in_local[2] = vec4(0.0, 1.0, 0.0, height);
         light.frustum_planes_in_local[3] = vec4(0.0, -1.0, 0.0, height);
         light.frustum_planes_in_local[4] = vec4(0.0, 0.0, 1.0, depth);
         light.frustum_planes_in_local[5] = vec4(0.0, 0.0, -1.0, 0.0);
      } 
   }
}


} // namespace yare