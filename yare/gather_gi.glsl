~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ VertexShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

layout(location = 0) in vec2 position;

out vec2 pixel_uv;

void main()
{
   gl_Position = vec4(position, 0.0, 1.0);
   pixel_uv = 0.5*(position + 1.0);
}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ FragmentShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include "glsl_voxelizer_defines.h"
#include "common.glsl"

layout(binding = BI_GATHER_GI_TEXTURE) uniform sampler2D input_texture;

in vec2 pixel_uv;
out vec3 out_global_illum;

const int blur_radius = 5;

float gaussianWeight(float x)
{
   const float sigma = 3.0*blur_radius;
   return 1.0 / (sigma*sqrt(2.0*PI)) * exp(- x*x/(2.0*sigma*sigma));
}

void main()
{
   vec2 pixel_size = 1.0 / textureSize(input_texture, 0);
   #ifdef BLUR_HORIZONTAL
      vec2 pixel_step = vec2(pixel_size.x, 0);
   #else
      vec2 pixel_step = vec2(0, pixel_size.y);
   #endif
   vec4 sum = vec4(0.0); 

   for (int i = -blur_radius; i <= blur_radius; ++i)
   {
      vec2 sample_pos = pixel_uv + (i) * pixel_step;
      vec3 sample_gi = texture(input_texture, sample_pos).rgb;            
      float weight = gaussianWeight(i);

      sum += vec4(weight * sample_gi, weight);      
   }
   out_global_illum = sum.rgb / sum.a;
}
