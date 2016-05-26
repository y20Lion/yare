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
#include "LatlongToCubemapConverter.h"
#include "GLTexture.h"
#include "glsl_binding_defines.h"
#include "OceanMaterial.h"
#include "BackgroundSky.h"
#include "GLFramebuffer.h"
#include "FilmProcessor.h"
#include "GLGPUTimer.h"

namespace yare {

struct SurfaceDynamicUniforms
{
   glm::mat4 matrix_view_local;
   glm::mat4 normal_matrix_world_local;
   glm::mat4 matrix_world_local;
};

struct SurfaceConstantUniforms
{
   glm::mat4 normal_matrix_world_local;
   glm::mat4 matrix_world_local;
};

struct SceneUniforms
{
   glm::mat4 matrix_view_world; 
   glm::vec3 eye_position;
   float time;
   float delta_time;
};

RenderEngine::RenderEngine(const ImageSize& framebuffer_size)
   : _scene()
   , render_resources(new RenderResources(framebuffer_size))
   , latlong_to_cubemap_converter(new LatlongToCubemapConverter(*render_resources))
   , background_sky(new BackgroundSky(*render_resources))
   , film_processor(new FilmProcessor(*render_resources))
{    
    
}

RenderEngine::~RenderEngine()
{   
}


static size_t _alignSize(size_t real_size, int aligned_size)
{
    return (real_size + aligned_size - 1) & -aligned_size;
}

static glm::mat4 _toMat4(const glm::mat4x3& matrix)
{
   glm::mat4 result;
   result[0] = glm::vec4(matrix[0], 0.0);
   result[1] = glm::vec4(matrix[1], 0.0);
   result[2] = glm::vec4(matrix[2], 0.0);
   result[3] = glm::vec4(matrix[3], 1.0);
   return result;
}

static glm::mat4x3 _toMat4x3(const glm::mat4& matrix)
{
   glm::mat4x3 result;
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
   //_surface_constant_uniforms_size = _alignSize(sizeof(SurfaceConstantUniforms), uniform_buffer_align_size);

   int surface_count = int(_scene.surfaces.size());
   _surface_dynamic_uniforms = createPersistentBuffer(_surface_dynamic_uniforms_size * surface_count);
   _scene_uniforms = createPersistentBuffer(_scene_uniforms_size);


   _scene.render_data[0].main_view_surface_data.resize(_scene.surfaces.size());
   _scene.render_data[1].main_view_surface_data.resize(_scene.surfaces.size());

   for (int i = 0; i < _scene.surfaces.size(); ++i)
   {
      if (_scene.surfaces[i].vertex_source_for_material == nullptr)
         _scene.surfaces[i].vertex_source_for_material = createVertexSource(*_scene.surfaces[i].mesh, _scene.surfaces[i].material->requiredMeshFields());
   }

   for (int i = 0; i < _scene.surfaces.size(); ++i)
   {    
      _scene.surfaces[i].normal_matrix_world_local = glm::mat3(transpose(inverse(_toMat4(_scene.surfaces[i].matrix_world_local))));
   }
}

void RenderEngine::updateScene(RenderData& render_data)
{
   auto mat = glm::lookAt(_scene.camera.point_of_view.from, _scene.camera.point_of_view.to, _scene.camera.point_of_view.up);
   render_data.matrix_view_world = glm::perspective(3.14f / 2.0f, 1.0f, 0.05f, 1000.0f) * mat;
   /** glm::frustum(camera.frustum.left, camera.frustum.right,
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

   last_update_time = time_lapse;

}

void RenderEngine::renderScene(const RenderData& render_data)
{
   GLDevice::bindFramebuffer(render_resources->main_framebuffer.get(), 0);
   {
      glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      _renderSurfaces(render_data);
      background_sky->render();
   }
   
   /*glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);*/

   GLDevice::bindFramebuffer(default_framebuffer, 0);
   {
      film_processor->developFilm();
   }   
   
   GLPersistentlyMappedBuffer::moveWindow();    
   GLGPUTimer::swapCounters();
}

void RenderEngine::_renderSurfaces(const RenderData& render_data)
{
   glEnable(GL_DEPTH_TEST);
   glDepthFunc(GL_LEQUAL);

   glViewport(0, 0, 1024, 768);

   //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
   GLDevice::bindTexture(BI_SKY_CUBEMAP, *_scene.sky_cubemap, *render_resources->sampler_cubemap);
   glBindBufferRange(GL_UNIFORM_BUFFER, BI_SCENE_UNIFORMS, _scene_uniforms->id(),
      _scene_uniforms->getCurrentWindowOffset(), _scene_uniforms->windowSize());
   glPatchParameteri(GL_PATCH_VERTICES, 3);

   for (int i = 0; i < render_data.main_view_surface_data.size(); ++i)
   {
      glBindBufferRange(GL_UNIFORM_BUFFER, BI_SURFACE_DYNAMIC_UNIFORMS,
         _surface_dynamic_uniforms->id(),
         _surface_dynamic_uniforms->getCurrentWindowOffset() + _surface_dynamic_uniforms_size*i,
         _surface_dynamic_uniforms_size);

      const auto& surface_data = render_data.main_view_surface_data[i];
      const auto& surface = _scene.surfaces[i];

      surface.material->render(*surface.vertex_source_for_material);
   }
}



} // namespace yare