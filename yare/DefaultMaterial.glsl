~~~~~~~~~~~~~~~~~~~VertexShader ~~~~~~~~~~~~~~~~~~~~~
#version 450
#include "glsl_binding_defines.h"

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

out vec3 attr_position;
out vec3 attr_normal;

layout(std140, binding = BI_SURFACE_DYNAMIC_UNIFORMS) uniform SurfaceDynamicUniforms
{
   mat4 matrix_view_local;
   mat4 normal_matrix_world_local;
   mat4x3 matrix_world_local;
};

void main()
{
   gl_Position = matrix_view_local * vec4(position, 1.0);
   attr_normal = mat3(normal_matrix_world_local)*normal;
   attr_position = matrix_world_local*vec4(position, 1.0);
}

~~~~~~~~~~~~~~~~~~FragmentShader ~~~~~~~~~~~~~~~~~~~~~~
#version 450
#include "glsl_binding_defines.h"

vec3 color = vec3(1.0, 0.0, 1.0);

layout(std140, binding = BI_SCENE_UNIFORMS) uniform SceneUniforms
{
   mat4 matrix_view_world;
   vec3 eye_position;
   float time;
};

in vec3 attr_position;
in vec3 attr_normal;

layout(location = 0) out vec4 shading_result;

void main()
{
   vec3 normal = normalize(attr_normal);
   vec3 light_direction = normalize(eye_position - attr_position);

   shading_result.rgb = max(dot(normal, light_direction), 0.02) * color;
}