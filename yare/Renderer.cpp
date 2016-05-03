#include "Renderer.h"

#include <glm/gtc/type_ptr.hpp>

#include "GLDevice.h"
#include "GLBuffer.h"
#include "GLProgram.h"
#include "ShadeTreeMaterial.h"

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

Renderer::Renderer()
    : _scene(createBasicScene())
{
    _draw_mesh = createProgram(vertex_source, fragment_source);  
    _uniforms_buffer = createBuffer(256* 1500);
}

struct UniformsData
{
    glm::mat4 matrix_view_local;
    glm::mat4 normal_matrix_view_local;
};

static size_t _alignSize(size_t real_size, int aligned_size)
{
    return (real_size + aligned_size - 1) & -aligned_size;
}

void Renderer::render()
{
    _scene.update();

    int uniform_buffer_align_size;
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uniform_buffer_align_size);
    size_t element_size = _alignSize(sizeof(UniformsData), uniform_buffer_align_size);

    char* buffer = (char*)_uniforms_buffer->mapRange(0, element_size * 1500, GL_MAP_WRITE_BIT);
    for (int i = 0; i < _scene.main_view_surface_data.size(); ++i)
    {
        ((UniformsData*)buffer)->matrix_view_local = _scene.main_view_surface_data[i].matrix_view_local;
        ((UniformsData*)buffer)->normal_matrix_view_local =  _scene.main_view_surface_data[i].normal_matrix_world_local;
        buffer += element_size;
    }
    _uniforms_buffer->unmap();

    /*for (int i = 0; i < _scene.surfaces_render_data.size(); ++i)
    {
        const auto& render_data = _scene.surfaces_render_data[i];
        const auto& surface = _scene.surfaces[i];

        GLDevice::setCurrentProgram(*_draw_mesh);
        glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(render_data.matrix_view_local));
        GLDevice::setCurrentVertexSource(*surface.mesh);        
        GLDevice::draw(0, surface.mesh->vertexCount());
    }*/

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    for (int i = 0; i < _scene.main_view_surface_data.size(); ++i)
    {
        const auto& render_data = _scene.main_view_surface_data[i];
        const auto& surface = _scene.surfaces[i];

        surface.material->bindTextures();

        GLDevice::setCurrentProgram(surface.material->program());
        glBindBufferRange(GL_UNIFORM_BUFFER, 3, _uniforms_buffer->id(), element_size * i, 16); //TODO yvain fixme
        
        //glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(render_data.matrix_view_local));
        GLDevice::setCurrentVertexSource(*render_data.vertex_source);
        GLDevice::draw(0, render_data.vertex_source->vertexCount());
    }
}

} // namespace yare