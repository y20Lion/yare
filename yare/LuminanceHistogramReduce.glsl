~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ComputeShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#version 450
#include "glsl_binding_defines.h"

#include "scene_uniforms.glsl"

layout(binding = BI_HISTOGRAMS_IMAGE, r32f) uniform image2D histograms_image;

layout(location = BI_NEW_EXPOSURE_INDEX) uniform int new_exposure_index;
layout(std430, binding = BI_EXPOSURE_VALUES_SSBO) buffer ExposureValuesSSBO
{
   float exposure[2];
} exposures;

const int COUNT = 768;

const float HISTOGRAM_MAX = 6;
const float HISTOGRAM_MIN = -8;
const int HISTOGRAM_SIZE = 8;

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

float averageLogLuminanceFromHistogram()
{
   int histogram_size = imageSize(histograms_image).x;
   float average = 0.0;

   for (int i = 0; i < HISTOGRAM_SIZE; ++i)
   {
      average += imageLoad(histograms_image, ivec2(i, 0)).x*(float(i) / HISTOGRAM_SIZE*float(HISTOGRAM_MAX - HISTOGRAM_MIN) + HISTOGRAM_MIN);
   }
   return average;
}

const float adaptation_speed = 1.0;

void main()
{
   float sum = 0.0;
   for (int i = 0; i < COUNT; ++i)
   {
      sum += imageLoad(histograms_image, ivec2(gl_GlobalInvocationID.x, i)).x;
   }
   sum /= COUNT;
   imageStore(histograms_image, ivec2(gl_GlobalInvocationID.x, 0), vec4(sum));

   barrier();

   if (gl_GlobalInvocationID.x == 0)
   {
      float log_average_lum = averageLogLuminanceFromHistogram();
      float average_lum = exp2(log_average_lum) / 0.018; // map average luminance to 18% of max intensity (see reinhard paper on tone mapping)

      float target_exposure = 1.0 / average_lum;
      float old_exposure = exposures.exposure[1 - new_exposure_index];

      float delta_exposure = target_exposure - old_exposure;
      float Factor = 1.0f - exp2(-delta_time * adaptation_speed);
      //exposures.exposure[new_exposure_index] = old_exposure + sign(delta_exposure)*clamp(delta_time*adaptation_speed, 0.0, abs(delta_exposure));
      exposures.exposure[new_exposure_index] = clamp(old_exposure + delta_exposure * Factor, 0.25, 2.0);
   }
}
