~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ VertexShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#version 450
#include "glsl_binding_defines.h"

layout(location = 0) in vec2 position;

void main()
{
   gl_Position = vec4(position, -1.0, 1.0);   
}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ FragmentShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#version 450
#include "glsl_binding_defines.h"

layout(binding = BI_INPUT_IMAGE, rgba32f) uniform readonly image2D input_image;
layout(binding = BI_HISTOGRAMS_IMAGE, r32f) uniform image2D histograms_image;

layout(location = BI_NEW_EXPOSURE_INDEX) uniform int new_exposure_index;
layout(std430, binding = BI_EXPOSURE_VALUES_SSBO) buffer ExposureValuesSSBO
{
   float exposure[2];
} exposures;

layout(location = 0) out vec4 color;

vec3 linearToSrgb(vec3 linear_color)
{
   vec3 result_lo = linear_color * 12.92;
   vec3 result_hi = (pow(abs(linear_color), vec3(1.0 / 2.4)) * 1.055) - 0.055;
   vec3 result;
   result.x = linear_color.x <= 0.0031308 ? result_lo.x : result_hi.x;
   result.y = linear_color.y <= 0.0031308 ? result_lo.y : result_hi.y;
   result.z = linear_color.z <= 0.0031308 ? result_lo.z : result_hi.z;
   return result;
}

void main()
{
   float exposure = exposures.exposure[new_exposure_index];

   //exposure = 1.0;
   vec3 linear_color = imageLoad(input_image, ivec2(gl_FragCoord.xy)).rgb * exposure;
   color.rgb = linearToSrgb(linear_color);
}