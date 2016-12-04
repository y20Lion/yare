~~~~~~~~~~~~~~~~~~~VertexShader ~~~~~~~~~~~~~~~~~~~~~
#include "glsl_global_defines.h"
#include "scene_uniforms.glsl"

layout(location = 0) in vec2 position;

out vec3 pos;

void main()
{
   gl_Position = vec4(position, 1.0, 1.0); 
   vec4 pos_in_world = inverse(matrix_proj_world)*gl_Position;
   pos = pos_in_world.xyz / pos_in_world.w;
}

~~~~~~~~~~~~~~~~~~FragmentShader ~~~~~~~~~~~~~~~~~~~~~~
#include "glsl_global_defines.h"
#include "scene_uniforms.glsl"

in vec3 pos;

layout(location = 0) out vec4 color;

void main()
{   
   vec3 eye_vector = normalize(pos - eye_position);
   color = vec4(vec3(0.0), 1.0);//vec4(texture(sky_cubemap, eye_vector).rgb, 1.0);

   vec2 current_ndc01_pos = (gl_FragCoord.xy - viewport.xy) / (viewport.zw);
   vec4 fog_info = texture(fog_volume, vec3(current_ndc01_pos, 1.0));
   float fog_transmittance = fog_info.a;
   vec3 fog_in_scattering = fog_info.rgb;

   color.rgb = color.rgb * fog_transmittance + fog_in_scattering;
}
/*

#include "glsl_global_defines.h"
#include "scene_uniforms.glsl"

layout(location = 0) in vec2 position;

//out vec3 pos;

void main()
{
   gl_Position = vec4(position, 1.0, 1.0);
}

#include "glsl_global_defines.h"
#include "scene_uniforms.glsl"

layout(location = 1) uniform mat4 matrix_world_proj;

//in vec3 pos;

layout(location = 0) out vec4 color;

void main()
{
   vec4 ndcPos;
   ndcPos.xy = ((2.0 * gl_FragCoord.xy)) / (viewport.zw) - 1.0;
   ndcPos.z = (2.0 * gl_FragCoord.z - 1.0);
   ndcPos.w = 1.0;

   vec4 pos_in_world = matrix_world_proj*ndcPos;
   vec3 pos = pos_in_world.xyz / pos_in_world.w;

   vec3 eye_vector = normalize(pos - eye_position);
   color = vec4(texture(sky_cubemap, eye_vector).rgb, 1.0);
}*/
