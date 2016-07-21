~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ VertexShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#version 450

layout(location = 0) in vec2 position;

void main()
{
   gl_Position = vec4(position, -1.0, 1.0);   
}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ FragmentShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#version 450
#include "glsl_global_defines.h"
#include "glsl_film_postprocessing_defines.h"

layout(binding = BI_SCENE_TEXTURE) uniform sampler2D scene_texture;
layout(binding = BI_BLOOM_TEXTURE) uniform sampler2D bloom_texture;
layout(location = BI_BLOOM_THRESHOLD) uniform float bloom_threshold;

layout(std430, binding = BI_EXPOSURE_VALUES_SSBO) buffer ExposureValuesSSBO
{
   float current_exposure;
   float previous_exposure;
} exposures;

layout(location = 0) out vec4 out_color;

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
   float exposure = exposures.current_exposure;

   //exposure = 0.05;
   vec3 scene_color = texelFetch(scene_texture, ivec2(gl_FragCoord.xy), 0).rgb;
   vec3 bloom_color = texture(bloom_texture, gl_FragCoord.xy / textureSize(scene_texture, 0)).rgb;
   vec3 color = (scene_color*exposure /*+ bloom_color*/);

   //color = color / (1.0 + color);
   out_color.rgb = linearToSrgb(scene_color*exposure);
}