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

namespace yare {

using namespace glm;

struct SurfaceDynamicUniforms
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
{    
    
}

RenderEngine::~RenderEngine()
{   
}

void RenderEngine::offlinePrepareScene()
{
   bindAnimationCurvesToTargets(_scene, *_scene.animation_player);
   
   int uniform_buffer_align_size;
   glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uniform_buffer_align_size);
   _surface_uniforms_size = GLFormats::alignSize(sizeof(SurfaceDynamicUniforms), uniform_buffer_align_size);

   int surface_count = int(_scene.surfaces.size());
   _surface_uniforms = createDynamicBuffer(_surface_uniforms_size * surface_count);
   _scene_uniforms = createDynamicBuffer(sizeof(SceneUniforms));
   _createSceneLightsBuffer();
   
   _scene.render_data[0].main_view_surface_data.resize(_scene.surfaces.size());
   _scene.render_data[1].main_view_surface_data.resize(_scene.surfaces.size());

   for (int i = 0; i < _scene.surfaces.size(); ++i)
   {
      if (_scene.surfaces[i].vertex_source_for_material == nullptr)
         _scene.surfaces[i].vertex_source_for_material = createVertexSource(*_scene.surfaces[i].mesh, _scene.surfaces[i].material->requiredMeshFields(_scene.surfaces[i].material_variant));
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

   const float znear = 0.05f;
   const float zfar = 1000.0f;
   
   auto mat = lookAt(_scene.camera.point_of_view.from, _scene.camera.point_of_view.to, _scene.camera.point_of_view.up);
   
   auto  matrix_projection = perspective(3.14f / 2.0f, render_resources->framebuffer_size.ratio(), znear, zfar);
   render_data.matrix_proj_world = matrix_projection * mat;
   /** frustum(camera.frustum.left, camera.frustum.right,
   camera.frustum.bottom, camera.frustum.top,
   camera.frustum.near, camera.frustum.far);*/
   //_scene.surfaces[3].matrix_world_local = mat4x3(1.0);
   for (int i = 0; i < render_data.main_view_surface_data.size(); ++i)
   {
      mat4 matrix_world_local = _scene.transform_hierarchy->nodeWorldToLocalMatrix(_scene.surfaces[i].transform_node_index);//.world_local.toMatrix();
      render_data.main_view_surface_data[i].matrix_world_local        = matrix_world_local;
      render_data.main_view_surface_data[i].normal_matrix_world_local = mat3(transpose(inverse(matrix_world_local)));
      render_data.main_view_surface_data[i].matrix_proj_local         = render_data.matrix_proj_world * matrix_world_local;      
   }

   char* buffer = (char*)_surface_uniforms->getUpdateSegmentPtr(); // hopefully OpenGL will be done using that range at that time (I could use a fence to enforce it but meh I don't care)
   for (int i = 0; i < render_data.main_view_surface_data.size(); ++i)
   {
      ((SurfaceDynamicUniforms*)buffer)->matrix_proj_local         = render_data.main_view_surface_data[i].matrix_proj_local;
      ((SurfaceDynamicUniforms*)buffer)->normal_matrix_world_local = render_data.main_view_surface_data[i].normal_matrix_world_local;
      ((SurfaceDynamicUniforms*)buffer)->matrix_world_local        = render_data.main_view_surface_data[i].matrix_world_local;
      buffer += _surface_uniforms_size;
   }
      
   
   SceneUniforms* scene_uniforms = (SceneUniforms*)_scene_uniforms->getUpdateSegmentPtr();
   scene_uniforms->eye_position = _scene.camera.point_of_view.from;
   scene_uniforms->matrix_proj_world = render_data.matrix_proj_world;
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
      background_sky->render();
      _renderSurfaces(render_data);
      
      //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
   }

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
      _scene_uniforms->getRenderSegmentOffset(), _scene_uniforms->segmentSize());

   

   for (int i = 0; i < render_data.main_view_surface_data.size(); ++i)
   {
      const auto& surface_data = render_data.main_view_surface_data[i];
      const auto& surface = _scene.surfaces[i];
      
      if (surface.skeleton)
      {
         const GLDynamicBuffer& skinning_ssbo = surface.skeleton->skinningPalette();
         glBindBufferRange(GL_SHADER_STORAGE_BUFFER, BI_SKINNING_PALETTE_SSBO, skinning_ssbo.id(),
                           skinning_ssbo.getRenderSegmentOffset(), skinning_ssbo.segmentSize());
      }

      glBindBufferRange(GL_UNIFORM_BUFFER, BI_SURFACE_DYNAMIC_UNIFORMS,
         _surface_uniforms->id(),
         _surface_uniforms->getRenderSegmentOffset() + _surface_uniforms_size*i,
         _surface_uniforms_size);

      

      surface.material->render(*surface.vertex_source_for_material, *surface.material_program);
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



} // namespace yare