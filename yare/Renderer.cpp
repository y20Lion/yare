#include "Renderer.h"

#include <glm/gtc/type_ptr.hpp>

#include "GLDevice.h"
#include "GLBuffer.h"
#include "GLProgram.h"
#include "ShadeTreeMaterial.h"

const char* vertex_source = 
        " #version 450 \n"
        " layout(location=1) in vec3 position; \n"
        " layout(location=1) in vec3 normal; \n"
        " layout(location=3) uniform mat4 t_view_local; \n"
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
}
struct Locations
{
    int position;
    int t_view_local;
};

void Renderer::render()
{
    _scene.update();

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
    for (int i = 0; i < _scene.surfaces_render_data.size(); ++i)
    {
        const auto& render_data = _scene.surfaces_render_data[i];
        const auto& surface = _scene.surfaces[i];

        GLDevice::setCurrentProgram(surface.material->program());        
        glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(render_data.matrix_view_local));
        GLDevice::setCurrentVertexSource(*surface.mesh);
        GLDevice::draw(0, surface.mesh->vertexCount());
    }
}
