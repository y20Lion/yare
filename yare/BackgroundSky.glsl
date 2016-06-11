~~~~~~~~~~~~~~~~~~~VertexShader ~~~~~~~~~~~~~~~~~~~~~
#version 450
#include "glsl_binding_defines.h"
#include "scene_uniforms.glsl"

layout(location = 0) in vec2 position;

out vec3 pos;

void main()
{
   gl_Position = vec4(position, 1.0, 1.0); 
   vec4 pos_in_world = inverse(matrix_view_world)*gl_Position;
   pos = pos_in_world.xyz / pos_in_world.w;
}

~~~~~~~~~~~~~~~~~~FragmentShader ~~~~~~~~~~~~~~~~~~~~~~
#version 450
#include "glsl_binding_defines.h"
#include "scene_uniforms.glsl"

in vec3 pos;

layout(location = 0) out vec4 color;

void main()
{   
   vec3 eye_vector = normalize(pos - eye_position);
   color = vec4(0.0);// vec4(texture(sky_cubemap, eye_vector).rgb, 1.0);
}