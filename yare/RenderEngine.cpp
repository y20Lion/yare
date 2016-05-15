#include "RenderEngine.h"

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

namespace yare {

    const char* vertex_source =
        " #version 450 \n"
        " layout(location=1) in vec3 position; \n"
        " layout(location=2) in vec3 normal; \n"
        " layout(std140, binding=3) uniform MatUniform \n"
        " { \n"
        "   mat4 t_view_local; \n"
        " }; \n"
        " out vec3 attr_normal; \n"
        " void main() \n"
        " { \n"
        "   gl_Position =  t_view_local * vec4(position, 1.0); \n" 
        "   attr_normal =  normal; \n"
        " }\n";
const char* fragment_source = 
        " #version 450 \n"
       " in vec3 attr_normal; \n"
        " void main() \n"
        " { \n"
        "   gl_FragColor =  vec4(attr_normal, 1.0); \n"
        " }\n";

RenderEngine::RenderEngine()
   : _scene()
   , render_resources(new RenderResources())
   , latlong_to_cubemap_converter(new LatlongToCubemapConverter(*render_resources))
{
    _draw_mesh = createProgram(vertex_source, fragment_source);  
    _uniforms_buffer = createBuffer(256* 1500);
}

RenderEngine::~RenderEngine()
{

}

struct UniformsData
{
    glm::mat4 matrix_view_local;
    glm::mat4 normal_matrix_world_local;
    glm::mat4 matrix_world_local;
};

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

void RenderEngine::update()
{
   auto mat = glm::lookAt(_scene.camera.point_of_view.from, _scene.camera.point_of_view.to, _scene.camera.point_of_view.up);
   _scene._matrix_view_world = glm::perspective(3.14f / 2.0f, 1.0f, 0.05f, 100.0f) * mat;
   /** glm::frustum(camera.frustum.left, camera.frustum.right,
   camera.frustum.bottom, camera.frustum.top,
   camera.frustum.near, camera.frustum.far);*/

   _scene.main_view_surface_data.resize(_scene.surfaces.size());
   for (int i = 0; i < _scene.main_view_surface_data.size(); ++i)
   {
      if (_scene.main_view_surface_data[i].vertex_source == nullptr)
         _scene.main_view_surface_data[i].vertex_source = createVertexSource(*_scene.surfaces[i].mesh);

      _scene.main_view_surface_data[i].matrix_view_local = _scene._matrix_view_world * _toMat4(_scene.surfaces[i].matrix_world_local);
      _scene.main_view_surface_data[i].normal_matrix_world_local = glm::mat3(transpose(inverse(_toMat4(_scene.surfaces[i].matrix_world_local))));    
   }
}

void RenderEngine::render()
{
    int uniform_buffer_align_size;
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uniform_buffer_align_size);
    size_t element_size = _alignSize(sizeof(UniformsData), uniform_buffer_align_size);

    char* buffer = (char*)_uniforms_buffer->mapRange(0, element_size * 1500, GL_MAP_WRITE_BIT);
    for (int i = 0; i < _scene.main_view_surface_data.size(); ++i)
    {
        ((UniformsData*)buffer)->matrix_view_local = _scene.main_view_surface_data[i].matrix_view_local;
        ((UniformsData*)buffer)->normal_matrix_world_local =  _scene.main_view_surface_data[i].normal_matrix_world_local;
        ((UniformsData*)buffer)->matrix_world_local = _scene.surfaces[i].matrix_world_local;
        buffer += element_size;
    }
    _uniforms_buffer->unmap();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, 1024, 768);
    GLDevice::bindTexture(BI_SKY_CUBEMAP, *_scene.sky_cubemap, *render_resources->sampler_cubemap);
    
    for (int i = 0; i < _scene.main_view_surface_data.size(); ++i)
    {
        const auto& render_data = _scene.main_view_surface_data[i];
        const auto& surface = _scene.surfaces[i];

        surface.material->bindTextures();

           
        GLDevice::bindProgram(surface.material->program());
        glBindBufferRange(GL_UNIFORM_BUFFER, 3, _uniforms_buffer->id(), element_size * i, element_size);
        glUniform3fv(BI_EYE_POSITION,1, glm::value_ptr(_scene.camera.point_of_view.from));

        
        glUniformMatrix4x3fv(12, 1, false, glm::value_ptr(_scene.surfaces[i].matrix_world_local));
        GLDevice::bindVertexSource(*render_data.vertex_source);
        
        if (surface.material->isTransparent())
        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_COLOR, GL_SRC1_COLOR);
            glBlendEquation(GL_FUNC_ADD);
        }
        
        GLDevice::draw(0, render_data.vertex_source->vertexCount());

        if (surface.material->isTransparent())
        {
            glDisable(GL_BLEND);
        }
    }
}


} // namespace yare