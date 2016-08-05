~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ VertexShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

layout(location = 0) in vec2 position;

out vec2 pixel_uv;

void main()
{
   gl_Position = vec4(position, 0.0, 1.0);
   pixel_uv = 0.5*(position + 1.0);
}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ FragmentShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include "glsl_ssao_defines.h"
#include "common.glsl"

layout(binding = BI_SSAO_TEXTURE) uniform sampler2D ssao_texture;
layout(binding = BI_DEPTHS_TEXTURE) uniform sampler2D depths_texture;

in vec2 pixel_uv;
out float out_blurred_ambient_occlusion;

const int blur_radius = 5;

float gaussianWeight(float x)
{
   const float sigma = 3.0*blur_radius;
   return 1.0 / (sigma*sqrt(2.0*PI)) * exp(- x*x/(2.0*sigma*sigma));
}

float depthWeight(float depth_center, float sample_depth)
{   
   return 1.0 / (1.0 + 100*abs(depth_center-sample_depth));
}

vec2 valueOfNeighborX(vec2 value)
{
   return value + (1.0 - 2.0*mod(floor(gl_FragCoord.x), 2.0)) * dFdxFine(value);
}

vec2 valueOfNeighborY(vec2 value)
{
   return value + (1.0 - 2.0*mod(floor(gl_FragCoord.y), 2.0)) * dFdyFine(value);
}

void main()
{
   vec2 pixel_size = 1.0 / textureSize(ssao_texture, 0);
   #ifdef BLUR_HORIZONTAL
      vec2 pixel_step = vec2(pixel_size.x, 0);
   #else
      vec2 pixel_step = vec2(0, pixel_size.y);
   #endif
   vec2 sum = vec2(0.0); 

   float depth_center = texture(depths_texture, pixel_uv).x;

   for (int i = -blur_radius; i <= blur_radius; ++i)
   {
      vec2 sample_pos = pixel_uv + (2*i) * pixel_step; // strided blur, missing samples will be obtained by using dFdx and dFdy
      float sample_ao = texture(ssao_texture, sample_pos).r;
      float sample_depth = texture(depths_texture, sample_pos).x;
      
      float weight =  gaussianWeight(i) * depthWeight(depth_center, sample_depth);
      vec2 current_val = vec2(weight * sample_ao, weight);

      // use dFdx and dFdy to get value of pixel not explicitly sampled
      #ifdef BLUR_HORIZONTAL
         current_val = current_val + valueOfNeighborX(current_val);
      #else
         current_val = current_val + valueOfNeighborY(current_val);
      #endif
      sum += current_val;
   }
   out_blurred_ambient_occlusion = sum.x / sum.y;
}