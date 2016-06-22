~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ComputeShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#version 450
#include "glsl_cubemap_filtering_defines.h"
#include "common_cubemap_filtering.glsl"

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(binding = BI_INPUT_CUBEMAP) uniform samplerCube input_cubemap;
layout(location = BI_INPUT_CUBEMAP_LEVEL) uniform int cubemap_level;
layout(std430, binding = BI_SPHERICAL_HARMONICS_SSBO) buffer lol
{
   vec3 spherical_harmonics[9];
};

void main()
{
   int input_face_size = textureSize(input_cubemap, cubemap_level).x;
   
   vec3 sum = vec3(0.0);
   float sum_weight = 0.0;
   for (int input_face = 0; input_face < 6; ++input_face)
   {
      for (int y = 0; y < input_face_size; ++y)
      {
         for (int x = 0; x < input_face_size; ++x)
         {                        
            vec3 radiance_dir = normalize(faceToDirection(input_face, (vec2(x, y) + 0.5) / float(input_face_size)));
            vec3 radiance = min(textureLod(input_cubemap, radiance_dir, cubemap_level).rgb, vec3(DIR_LIGHT_THRESHOLD));
            
            float legendre_input[10];
            legendre_input[0] = 1.0;
            legendre_input[1] = radiance_dir.x;
            legendre_input[2] = radiance_dir.y;
            legendre_input[3] = radiance_dir.z;
            legendre_input[4] = radiance_dir.x*radiance_dir.x;
            legendre_input[5] = radiance_dir.y*radiance_dir.y;
            legendre_input[6] = radiance_dir.z*radiance_dir.z;
            legendre_input[7] = radiance_dir.y*radiance_dir.z;
            legendre_input[8] = radiance_dir.x*radiance_dir.z;
            legendre_input[9] = radiance_dir.x*radiance_dir.y;

            float legendre_evaluation = 0.0;   
            for (int j = 0; j < 10; j++)
               legendre_evaluation += legendre_coeff[gl_GlobalInvocationID.x][j] * legendre_input[j];
            
            float weight = Klm[gl_GlobalInvocationID.x] * legendre_evaluation;
            sum_weight += weight;
            sum += radiance * weight * texelSolidAngle(x, y, input_face_size);
         }
      }
   }

   spherical_harmonics[gl_GlobalInvocationID.x] = sum * cos_sph_harmonics_coeffs[gl_GlobalInvocationID.x];
}
