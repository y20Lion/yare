#include "RenderEngine.h"

#include <chrono>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include "GLDevice.h"
#include "GLBuffer.h"
#include "GLSampler.h"
#include "GLProgram.h"
#include "ShadeTreeMaterial.h"
#include "RenderMesh.h"
#include "RenderResources.h"
#include "CubemapFiltering.h"
#include "GLTexture.h"
#include "glsl_binding_defines.h"
#include "OceanMaterial.h"
#include "BackgroundSky.h"
#include "GLFramebuffer.h"
#include "FilmPostProcessor.h"
#include "GLGPUTimer.h"

namespace yare {

using namespace glm;

struct SurfaceDynamicUniforms
{
   mat4 matrix_view_local;
   mat4 normal_matrix_world_local;
   mat4 matrix_world_local;
};

struct SceneUniforms
{
   mat4 matrix_view_world; 
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
{    
    
}

RenderEngine::~RenderEngine()
{   
}


static size_t _alignSize(size_t real_size, int aligned_size)
{
    return (real_size + aligned_size - 1) & -aligned_size;
}

static mat4 _toMat4(const mat4x3& matrix)
{
   mat4 result;
   result[0] = vec4(matrix[0], 0.0);
   result[1] = vec4(matrix[1], 0.0);
   result[2] = vec4(matrix[2], 0.0);
   result[3] = vec4(matrix[3], 1.0);
   return result;
}

static mat4x3 _toMat4x3(const mat4& matrix)
{
   mat4x3 result;
   result[0] = matrix[0].xyz;
   result[1] = matrix[1].xyz;
   result[2] = matrix[2].xyz;
   result[3] = matrix[3].xyz;
   return result;
}

void RenderEngine::offlinePrepareScene()
{
   int uniform_buffer_align_size;
   glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uniform_buffer_align_size);
   _surface_dynamic_uniforms_size = _alignSize(sizeof(SurfaceDynamicUniforms), uniform_buffer_align_size);
   _scene_uniforms_size = _alignSize(sizeof(SceneUniforms), uniform_buffer_align_size);

   int surface_count = int(_scene.surfaces.size());
   _surface_dynamic_uniforms = createPersistentBuffer(_surface_dynamic_uniforms_size * surface_count);
   _scene_uniforms = createPersistentBuffer(_scene_uniforms_size);   
   _createSceneLightsBuffer();
   
   _scene.render_data[0].main_view_surface_data.resize(_scene.surfaces.size());
   _scene.render_data[1].main_view_surface_data.resize(_scene.surfaces.size());

   for (int i = 0; i < _scene.surfaces.size(); ++i)
   {
      if (_scene.surfaces[i].vertex_source_for_material == nullptr)
         _scene.surfaces[i].vertex_source_for_material = createVertexSource(*_scene.surfaces[i].mesh, _scene.surfaces[i].material->requiredMeshFields());
   }

   for (int i = 0; i < _scene.surfaces.size(); ++i)
   {    
      _scene.surfaces[i].normal_matrix_world_local = mat3(transpose(inverse(_toMat4(_scene.surfaces[i].matrix_world_local))));
   }
}

void RenderEngine::updateScene(RenderData& render_data)
{
   const float znear = 0.05f;
   const float zfar = 1000.0f;
   
   auto mat = lookAt(_scene.camera.point_of_view.from, _scene.camera.point_of_view.to, _scene.camera.point_of_view.up);
   
   auto  matrix_projection = perspective(3.14f / 2.0f, render_resources->framebuffer_size.ratio(), znear, zfar);
   render_data.matrix_view_world = matrix_projection * mat;
   /** frustum(camera.frustum.left, camera.frustum.right,
   camera.frustum.bottom, camera.frustum.top,
   camera.frustum.near, camera.frustum.far);*/

   for (int i = 0; i < render_data.main_view_surface_data.size(); ++i)
   {
      render_data.main_view_surface_data[i].matrix_view_local = render_data.matrix_view_world * _toMat4(_scene.surfaces[i].matrix_world_local);
   }

   char* buffer = (char*)_surface_dynamic_uniforms->getCurrentWindowPtr(); // yeah I don't care if that buffer is still used by OpenGL
   for (int i = 0; i < render_data.main_view_surface_data.size(); ++i)
   {
      ((SurfaceDynamicUniforms*)buffer)->matrix_view_local = render_data.main_view_surface_data[i].matrix_view_local;
      ((SurfaceDynamicUniforms*)buffer)->normal_matrix_world_local = _scene.surfaces[i].normal_matrix_world_local;
      ((SurfaceDynamicUniforms*)buffer)->matrix_world_local = _scene.surfaces[i].matrix_world_local;
      buffer += _surface_dynamic_uniforms_size;
   }

   static float last_update_time = 0.0;
   static auto start = std::chrono::steady_clock::now();
   auto now = std::chrono::steady_clock::now();
   float time_lapse = std::chrono::duration<float>(now - start).count();


   SceneUniforms* scene_uniforms = (SceneUniforms*)_scene_uniforms->getCurrentWindowPtr();
   scene_uniforms->eye_position = _scene.camera.point_of_view.from;
   scene_uniforms->matrix_view_world = render_data.matrix_view_world;
   scene_uniforms->time = time_lapse;
   scene_uniforms->delta_time = time_lapse - last_update_time;
   scene_uniforms->znear = znear;
   scene_uniforms->zfar = zfar;
   scene_uniforms->proj_coeff_11 = matrix_projection[1][1];
   float tessellation_pixels_per_edge = 30.0;
   scene_uniforms->tessellation_edges_per_screen_height = render_resources->framebuffer_size.height/ tessellation_pixels_per_edge;

   last_update_time = time_lapse;

}

void RenderEngine::renderScene(const RenderData& render_data)
{
   GLDevice::bindDefaultDepthStencilState();   
   GLDevice::bindDefaultColorBlendState();
   GLDevice::bindDefaultRasterizationState();
   glPatchParameteri(GL_PATCH_VERTICES, 3);

   GLDevice::bindFramebuffer(render_resources->main_framebuffer.get(), 0);  
   {
      glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glViewport(0, 0, render_resources->main_framebuffer->width(), render_resources->main_framebuffer->height());
      //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      _renderSurfaces(render_data);
      background_sky->render();
      //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
   }

   film_processor->developFilm();
 
   GLPersistentlyMappedBuffer::moveWindow();    
   GLGPUTimer::swapCounters();
}

void RenderEngine::_renderSurfaces(const RenderData& render_data)
{
   static int counter = 0; 
   
   const auto & diffuse_cubemap =  counter < 120 ? *_scene.sky_diffuse_cubemap : *_scene.sky_diffuse_cubemap_sh;
   GLDevice::bindTexture(BI_SKY_CUBEMAP, *_scene.sky_cubemap, *render_resources->sampler_mipmap_clampToEdge);
   GLDevice::bindTexture(BI_SKY_DIFFUSE_CUBEMAP, diffuse_cubemap, *render_resources->sampler_mipmap_clampToEdge);
   counter++;
   if (counter == 240)
      counter = 0;
   glBindBufferRange(GL_UNIFORM_BUFFER, BI_SCENE_UNIFORMS, _scene_uniforms->id(),
      _scene_uniforms->getCurrentWindowOffset(), _scene_uniforms->windowSize());

   for (int i = 0; i < render_data.main_view_surface_data.size(); ++i)
   {
      glBindBufferRange(GL_UNIFORM_BUFFER, BI_SURFACE_DYNAMIC_UNIFORMS,
         _surface_dynamic_uniforms->id(),
         _surface_dynamic_uniforms->getCurrentWindowOffset() + _surface_dynamic_uniforms_size*i,
         _surface_dynamic_uniforms_size);

      const auto& surface_data = render_data.main_view_surface_data[i];
      const auto& surface = _scene.surfaces[i];

      surface.material->render(*render_resources, *surface.vertex_source_for_material);
   }
}

void RenderEngine::_createSceneLightsBuffer()
{  
   _lights_ssbo = createBuffer(sizeof(LightSSBO)*_scene.lights.size()+sizeof(glm::vec4));

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
            buffer_light->color = light.color*light.strength;
            buffer_light->spot.position = light.world_to_local_matrix[3];
            buffer_light->spot.direction = light.world_to_local_matrix[2];
            buffer_light->spot.cos_half_angle = cos(light.spot.angle/2.0);
            buffer_light->spot.angle_smooth = light.spot.angle_blend;
            break;
      }

      data += sizeof(LightSSBO);
   }
   _lights_ssbo->unmap();
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BI_LIGHTS_SSBO, _lights_ssbo->id());
}



} // namespace yare