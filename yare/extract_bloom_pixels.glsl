~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~VertexShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

layout(location = 0) in vec2 position;

void main()
{
   gl_Position = vec4(position, -1.0, 1.0);
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~FragmentShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include "glsl_global_defines.h"
#include "glsl_film_postprocessing_defines.h"

layout(binding = BI_INPUT_TEXTURE) uniform sampler2D input_texture;
layout(location = BI_BLOOM_THRESHOLD) uniform float bloom_threshold;

layout(location = 0) out vec4 out_color;

layout(std430, binding = BI_EXPOSURE_VALUES_SSBO) buffer ExposureValuesSSBO
{
   float current_exposure;
   float previous_exposure;
} exposures;

#include "common.glsl"

void main()
{
   vec2 texcoord = gl_FragCoord.xy / textureSize(input_texture, 0);

   vec3 color = texture(input_texture, texcoord, 0).rgb;
   float full_luminance = luminance(color)*exposures.current_exposure;   
   if (full_luminance > 1.0)
      color = color / full_luminance *1.0;

   float bloom_luminance = max(full_luminance - bloom_threshold, 0.0);
   float bloom_amount = saturate(bloom_luminance/2.0);

   out_color = vec4(color*bloom_amount, 1.0);
}
