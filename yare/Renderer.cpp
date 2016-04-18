#include "Renderer.h"

#include <glm/gtc/type_ptr.hpp>

#include "GLDevice.h"
#include "GLBuffer.h"
#include "GLProgram.h"

const char* vertex_source = 
        " #version 450 \n"
        " layout(location=1) in vec3 position; \n"
        " layout(location=2) uniform mat4 t_view_local; \n"
        " void main() \n"
        " { \n"
        "   gl_Position =  t_view_local * vec4(position, 1.0); \n" 
        " }\n";
const char* fragment_source = 
        " #version 450 \n"
        " void main() \n"
        " { \n"
        "   gl_FragColor =  vec4(1.0, 1.0, 0.0, 1.0); \n"
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

    for (int i = 0; i < _scene.surfaces_render_data.size(); ++i)
    {
        const auto& render_data = _scene.surfaces_render_data[i];
        const auto& surface = _scene.surfaces[i];

        GLDevice::setCurrentProgram(*_draw_mesh);
        //GLDevice::setUniformMatrix4(loc.t_view_local, render_data.matrix_view_local);
        glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(render_data.matrix_view_local));
        GLDevice::setCurrentVertexSource(*surface.mesh);        
        GLDevice::draw(0, 6);
    }
}
