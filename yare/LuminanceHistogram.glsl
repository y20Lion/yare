~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ComputeShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#version 450
#include "glsl_binding_defines.h"

layout(binding = BI_INPUT_IMAGE, rgba32f) uniform readonly image2D input_image;
layout(binding = BI_HISTOGRAMS_IMAGE, r32f) uniform image2D histograms_image;

const float HISTOGRAM_MAX = 6;
const float HISTOGRAM_MIN = -8;
const int TILE_SIZE_X = 32;
const int TILE_SIZE_Y = 32;
const int HISTOGRAM_SIZE = 8;
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

shared float shared_histogram[HISTOGRAM_SIZE];

float luminance(vec3 color)
{
   return dot(vec3(0.3f, 0.59f, 0.11f), color);
}

float saturate(float v)
{
   return clamp(v, 0.0, 1.0);
}

void main()
{
   uvec3 localID = gl_LocalInvocationID;
   uvec3 groupID = gl_WorkGroupID;
   //if (gl_LocalInvocationIndex == 0)
   {
      for (int i = 0; i < HISTOGRAM_SIZE; ++i)
      {
         shared_histogram[i] = 0.0f;
      }
   }   

   //barrier();

   for (int y = 0; y < TILE_SIZE_Y; ++y)
   {
      for (int x = 0; x < TILE_SIZE_X; ++x)
      {
         ivec2 pixel_pos = ivec2(groupID.xy*TILE_SIZE_X) + ivec2(x,y);
         vec3 color = imageLoad(input_image, pixel_pos).rgb;

         float log_luminance = /*log2(max(max(color.r, color.g),color.b));*/log2(luminance(color));
         float abscissa = (log_luminance - HISTOGRAM_MIN)/(HISTOGRAM_MAX - HISTOGRAM_MIN);        
         
         float bucket = saturate(abscissa) * (HISTOGRAM_SIZE - 1);

         //shared_histogram[clamp(int(bucket),0, HISTOGRAM_SIZE - 1)] += 1.0;
         
         int bucket0 = int(bucket);
         int bucket1 = bucket0 + 1;

         float weight1 = fract(bucket);
         float weight0 = 1.0f - weight1;

         //if (bucket0 != 0)
            shared_histogram[bucket0] += weight0;
         shared_histogram[bucket1] += weight1;
      }
   }

   //barrier();

   //if (gl_LocalInvocationIndex == 0)
   {
      uint histogram_pos = groupID.y*gl_NumWorkGroups.x+ groupID.x;
      for (int i = 0; i < HISTOGRAM_SIZE; ++i)
      {         
         imageStore(histograms_image, ivec2(i, histogram_pos), vec4(shared_histogram[i]/(TILE_SIZE_Y*TILE_SIZE_X)));
      }
   }
}