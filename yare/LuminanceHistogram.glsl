~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ComputeShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#version 450
#include "glsl_binding_defines.h"
#include "glsl_histogram_defines.h"

layout(binding = BI_INPUT_IMAGE, rgba32f) uniform readonly image2D input_image;
layout(binding = BI_HISTOGRAMS_IMAGE, r32f) uniform image2D histograms_image;

layout(local_size_x = THREADS_SIZE_X, local_size_y = THREADS_SIZE_Y, local_size_z = 1) in;

shared float shared_histogram[HISTOGRAM_SIZE][THREADS_SIZE_X][THREADS_SIZE_Y];

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

   for (int i = 0; i < HISTOGRAM_SIZE; ++i)
   {
      shared_histogram[i][localID.x][localID.y] = 0.0f;
   } 

   barrier();

   for (int y = 0; y < GATHER_SIZE_Y; ++y)
   {
      for (int x = 0; x < GATHER_SIZE_X; ++x)
      {
         ivec2 pixel_pos = ivec2(groupID.x*TILE_SIZE_X, groupID.y*TILE_SIZE_Y) + ivec2(x,y);
         vec3 color = imageLoad(input_image, pixel_pos).rgb;

         float log_luminance = log2(luminance(color));
         float abscissa = (log_luminance - HISTOGRAM_MIN)/(HISTOGRAM_MAX - HISTOGRAM_MIN);        
         
         float bucket = saturate(abscissa) * (HISTOGRAM_SIZE - 1);
         
         int bucket0 = int(bucket);
         int bucket1 = bucket0 + 1;

         float weight1 = fract(bucket);
         float weight0 = 1.0f - weight1;

         shared_histogram[bucket0][localID.x][localID.y] += weight0;
         shared_histogram[bucket1][localID.x][localID.y] += weight1;
      }
   }

   barrier();

   if (gl_LocalInvocationIndex < HISTOGRAM_SIZE)
   {      
      float sum = 0.0;
      for (int y = 0; y < THREADS_SIZE_Y; ++y)
      {
         for (int x = 0; x < THREADS_SIZE_X; ++x)
         {
            sum += shared_histogram[gl_LocalInvocationIndex][localID.x][localID.y];
         }
      }
      sum /= (TILE_SIZE_Y*TILE_SIZE_X);
      
      uint histogram_pos = groupID.y*gl_NumWorkGroups.x + groupID.x; // all histograms are stored lineary in output texture
      imageStore(histograms_image, ivec2(gl_LocalInvocationIndex, histogram_pos), vec4(sum));
   }
}