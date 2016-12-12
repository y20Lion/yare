~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ VertexShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include "glsl_global_defines.h"
#include "glsl_volumetric_fog_defines.h"
#include "scene_uniforms.glsl"
#include "lighting_uniforms.glsl"

layout(location = 0) in vec2 position;

layout(location = BI_FRUSTUM) uniform vec4 frustum;

out int light_index;
out float sprite_size;

void main()
{
   vec3 light_position = sphere_lights[gl_VertexID].position;

   gl_Position = matrix_proj_world * vec4(light_position, 1.0);
   float depth = gl_Position.w;
   float viewport_width = viewport.z;

   sprite_size = 0.2;      
   gl_PointSize = sprite_size* znear/(depth*(frustum.y - frustum.x))* viewport_width;
   light_index = gl_VertexID;
}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ FragmentShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include "glsl_volumetric_fog_defines.h"
#include "glsl_global_defines.h"
#include "common.glsl"
#include "scene_uniforms.glsl"
#include "lighting_uniforms.glsl"

layout(location = BI_SCATTERING_ABSORPTION) uniform vec4 scattering_absorption;

flat in int light_index;
in float sprite_size;

layout(location = 0) out vec4 shading_result;

float convertCameraZtoFroxelZ(float z_in_camera_space, float znear, float zfar)
{
   float z = (z_in_camera_space - znear) / (zfar - znear);
   float froxel_z = pow(z, 1.0f / froxel_z_distribution_factor);

   return clamp(froxel_z, 0.0f, 1.0f);
}

vec3 positionInFrustumAlignedVolumeTextures()
{
   vec2 current_ndc01_pos = (gl_FragCoord.xy - viewport.xy) / (viewport.zw);
   float z_eye_space = 2.0 * znear * zfar / (znear + zfar - (2.0*gl_FragCoord.z - 1.0) * (zfar - znear));
   float froxel_z = convertCameraZtoFroxelZ(z_eye_space, znear, zfar);

   return vec3(current_ndc01_pos, froxel_z);
}

void main()
{
   SphereLight light = sphere_lights[light_index];
  
   float norm_distance = distance(gl_PointCoord.xy - vec2(0.5), vec2(0.0));   
   float light_distance =sprite_size*norm_distance;
   light_distance = max(light_distance, light.size);
   
   float light_distance_sqr = light_distance*light_distance;
   float restrict_influence_factor = pow(saturate(1.0 - pow(light_distance_sqr / (light.radius*light.radius), 2.0)), 2.0);
   
   vec3 light_color = light.color*scattering_absorption.rgb /(4*PI) / ((4 * PI) * light_distance_sqr)*restrict_influence_factor;
   light_color *= (1.0-smoothstep(0.0, 1.0, 2.0*norm_distance));
   shading_result = vec4(light_color, 1.0);

   vec2 current_ndc01_pos = (gl_FragCoord.xy - viewport.xy) / (viewport.zw);
   vec4 fog_info = texture(fog_volume, positionInFrustumAlignedVolumeTextures());
   float fog_transmittance = fog_info.a;
   vec3 fog_in_scattering = fog_info.rgb;

   shading_result = shading_result * (fog_transmittance);
}
