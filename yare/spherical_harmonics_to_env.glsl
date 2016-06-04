~~~~~~~~~~~~~~~~~~VertexShader ~~~~~~~~~~~~~~~~~~~~~~~~
#version 450

layout(location = 0) in vec2 position;

out vec2 uv;

void main()
{
   uv = position;
   gl_Position = vec4(position, 0.5, 1.0);
}

~~~~~~~~~~~~~~~~~FragmentShader ~~~~~~~~~~~~~~~~~~~~~~
#version 450
#include "glsl_cubemap_filtering_defines.h"
#include "common_cubemap_filtering.glsl"

layout(location = BI_FACE) uniform int face;
layout(std430, binding = BI_SPHERICAL_HARMONICS_SSBO) buffer SphericalHarmonics
{
   vec3 spherical_harmonics[9];
};

in vec2 uv;
out vec4 out_face_color;

void main()
{
   vec3 normal_dir = normalize(faceToDirection(face, uv));

   float legendre_input[10];
   legendre_input[0] = 1.0;
   legendre_input[1] = normal_dir.x;
   legendre_input[2] = normal_dir.y;
   legendre_input[3] = normal_dir.z;
   legendre_input[4] = normal_dir.x*normal_dir.x;
   legendre_input[5] = normal_dir.y*normal_dir.y;
   legendre_input[6] = normal_dir.z*normal_dir.z;
   legendre_input[7] = normal_dir.y*normal_dir.z;
   legendre_input[8] = normal_dir.x*normal_dir.z;
   legendre_input[9] = normal_dir.x*normal_dir.y;
   
   vec3 color = vec3(0.0);
   for (int i = 0; i < 9; ++i)
   {    
      float legendre_evaluation = 0.0;
      for (int j = 0; j < 10; ++j)
         legendre_evaluation += legendre_coeff[i][j] * legendre_input[j];

      color += spherical_harmonics[i] * Klm[i] * legendre_evaluation;
   }

   out_face_color.rgb = color / PI;
}
