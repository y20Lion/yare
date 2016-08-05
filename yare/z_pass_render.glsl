~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ VertexShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include "glsl_global_defines.h"

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

#include "surface_uniforms.glsl"

out vec3 attr_normal;

void main()
{
   gl_Position = matrix_proj_local * vec4(position, 1.0); 
   attr_normal = mat3(normal_matrix_world_local)*normal;
}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ FragmentShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

in vec3 attr_normal;

layout(location = 0) out vec3 out_normal;

void main()
{
   out_normal = attr_normal;
}

