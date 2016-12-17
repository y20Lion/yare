~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ComputeShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include "glsl_volumetric_fog_defines.h"
#include "glsl_global_defines.h"
#include "scene_uniforms.glsl"
#include "lighting_uniforms.glsl"
#include "common.glsl"
#include "lighting.glsl"

layout(binding = BI_INSCATTERING_EXTINCTION_VOLUME_IMAGE, rgba16f) uniform restrict writeonly image3D scattering_extinction_image;
layout(location = BI_MATRIX_WORLD_VIEW) uniform mat4 matrix_world_view;
layout(location = BI_FRUSTUM) uniform vec4 frustum;
layout(location = BI_SCATTERING_ABSORPTION) uniform vec4 scattering_absorption;

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

void main()
{
   vec3 inv_fog_vol_size = 1.0 / vec3(imageSize(scattering_extinction_image));
   float absorption = scattering_absorption.w;
   vec3 scattering = scattering_absorption.rgb;

   float current_depth = convertFroxelZtoDepth(float(gl_GlobalInvocationID.z) * inv_fog_vol_size.z, znear, zfar);
   float next_depth = convertFroxelZtoDepth(float(gl_GlobalInvocationID.z + 1) * inv_fog_vol_size.z, znear, zfar);
   float froxel_length = next_depth - current_depth;   

   float extinction = max(0.000000001, absorption + (scattering.r+ scattering.g+ scattering.b)*0.33333);
   
   vec3 froxel_coords01 = (vec3(gl_GlobalInvocationID.xyz) + 0.5) * inv_fog_vol_size;
   vec3 froxel_in_view = convertFroxelToCameraCoords(froxel_coords01, frustum);
   vec3 froxel_center_in_world = mat4x3(matrix_world_view) * vec4(froxel_in_view, 1.0);

   FroxelLightLists froxel_light_lists = fetchCurrentFroxelLightLists(froxel_coords01);  

   unsigned int start_offset = froxel_light_lists.start_offset;
   vec3 in_scattered_radiance = vec3(0.0);
   
   for (unsigned int i = 0; i < froxel_light_lists.sphere_light_count; ++i)
   {
      int light_index = light_list_data[start_offset + i];
      in_scattered_radiance += sphereLightIncidentRadiance(sphere_lights[light_index], froxel_center_in_world, froxel_length);
   }
   start_offset += froxel_light_lists.sphere_light_count;

   for (unsigned int i = 0; i < froxel_light_lists.spot_light_count; ++i)
   {
      int light_index = light_list_data[start_offset + i];
      in_scattered_radiance += spotLightIncidentRadiance(spot_lights[light_index], froxel_center_in_world, froxel_length);
   }
   start_offset += froxel_light_lists.spot_light_count;

   in_scattered_radiance += textureLod(sky_diffuse_cubemap, vec3(0, 0, 1), 0).rgb;
   in_scattered_radiance *= scattering * 1 / (4 * PI);


   imageStore(scattering_extinction_image, ivec3(gl_GlobalInvocationID.xyz), vec4(in_scattered_radiance, extinction));
}