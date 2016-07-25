~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ VertexShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#version 450
#include "glsl_global_defines.h"

layout(location = 0) in vec3 position;

#include "surface_uniforms.glsl"

void main()
{
   gl_Position = matrix_proj_local * vec4(position, 1.0); 
}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ FragmentShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#version 450
void main()
{
}

