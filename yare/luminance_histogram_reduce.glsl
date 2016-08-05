~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ComputeShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include "glsl_global_defines.h"
#include "glsl_histogram_defines.h"
#include "glsl_film_postprocessing_defines.h"

#include "scene_uniforms.glsl"

layout(binding = BI_HISTOGRAMS_IMAGE, r32f) uniform image2D histograms_image;

layout(std430, binding = BI_EXPOSURE_VALUES_SSBO) buffer ExposureValuesSSBO
{
   float current_exposure;
   float previous_exposure;
} exposures;

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

float averageLogLuminanceFromHistogram()
{
   
   float average = 0.0;
   float accumulated_percentage = 0.0;
   for (int i = 0; i < HISTOGRAM_SIZE; ++i)
   {
      float percentage = imageLoad(histograms_image, ivec2(i, 0)).x;
      accumulated_percentage += percentage;
      if (accumulated_percentage >= 0.4 && accumulated_percentage <= 0.95)
         average += percentage*(float(i+0.5) / HISTOGRAM_SIZE*float(HISTOGRAM_MAX - HISTOGRAM_MIN) + HISTOGRAM_MIN);
   }
   return average/accumulated_percentage;
}

const float adaptation_speed = 3.0;

void main()
{
   int histogram_count = imageSize(histograms_image).y;
   
   float sum = 0.0;
   for (int i = 0; i < histogram_count; ++i)
   {
      sum += imageLoad(histograms_image, ivec2(gl_GlobalInvocationID.x, i)).x;
   }
   sum /= histogram_count;
   imageStore(histograms_image, ivec2(gl_GlobalInvocationID.x, 0), vec4(sum));

   barrier();

   if (gl_GlobalInvocationID.x == 0)
   {
      float log_average_lum = averageLogLuminanceFromHistogram();
      float average_lum = exp2(log_average_lum) / 0.18; // map average luminance to 18% of max intensity (see reinhard paper on tone mapping)

      float target_exposure = 1.0 / average_lum;
      float old_exposure = exposures.previous_exposure;

      float delta_exposure = target_exposure - old_exposure;
      float Factor = 1.0f - exp2(-delta_time * adaptation_speed);
      //exposures.exposure[new_exposure_index] = old_exposure + sign(delta_exposure)*clamp(delta_time*adaptation_speed, 0.0, abs(delta_exposure));
      exposures.previous_exposure = exposures.current_exposure;
      exposures.current_exposure =  clamp(old_exposure + delta_exposure * Factor, 0.1, 5.0);
   }
}
