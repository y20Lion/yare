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


layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

float convertFroxelZtoDepth(float froxel_z, float znear, float zfar)
{
   float z = pow(froxel_z, 2.0);

   return mix(znear, zfar, z);
}

vec3 convertFroxelToCameraCoords(vec3 froxel_coords, vec3 froxels_dims)
{
   float z = convertFroxelZtoDepth(froxel_coords.z / froxels_dims.z, znear, zfar);

   float ratio = z / znear;
   float x = mix(frustum.x, frustum.y, froxel_coords.x / froxels_dims.x) * ratio;
   float y = mix(frustum.z, frustum.w, froxel_coords.y / froxels_dims.y) * ratio;

   return vec3(x, y, -z);
}

void main()
{
   float accum_transmittance = 1.0;
   vec3 accum_scattering = vec3(0.0);
   ivec3 volume_size = imageSize(scattering_extinction_image);

   float current_depth = convertFroxelZtoDepth(float(gl_GlobalInvocationID.z) / volume_size.z, znear, zfar);
   float next_depth = convertFroxelZtoDepth(float(gl_GlobalInvocationID.z +1) / volume_size.z, znear, zfar);
   float froxel_length = next_depth - current_depth;
   float absorption = 0.0;
   float scattering = 0.1f;

   float extinction = max(0.000000001, absorption + scattering);

   vec3 froxel_in_view = convertFroxelToCameraCoords(vec3(gl_GlobalInvocationID.xyz), volume_size);
   vec3 froxel_center_in_world = mat4x3(matrix_world_view) * vec4(froxel_in_view, 1.0);

   vec3 froxel_coords01 = vec3(gl_GlobalInvocationID.xyz) / volume_size;
   ivec3 current_froxel_coords = ivec3(light_froxels_dims * froxel_coords01);
   uvec2 froxel_data = texelFetch(light_list_head, current_froxel_coords, 0).xy;
   unsigned int start_offset = froxel_data.x;
   froxel_light_lists.sphere_light_count = froxel_data.y & 0x3FF;
   froxel_light_lists.spot_light_count = (froxel_data.y >> 10) & 0x3FF;
   froxel_light_lists.rectangle_light_count = (froxel_data.y >> 20) & 0x3FF;

   vec3 in_scattered_radiance = vec3(0.0);
   for (unsigned int i = 0; i < froxel_light_lists.sphere_light_count; ++i)
   {
      int light_index = light_list_data[start_offset + i];
      SphereLight light = sphere_lights[light_index];

      in_scattered_radiance += 1 / (4 * PI)* scattering *pointLightIncidentRadiance(light.color, light.position, light.radius, froxel_center_in_world);
   }
   start_offset += froxel_light_lists.sphere_light_count;
   //vec3 in_scattered_radiance = 1 / (4 * PI)* scattering *pointLightIncidentRadiance(vec3(1.0,0.5,0.0), vec3(0.0), 5.0, froxel_center_in_world);

   vec3 inscatt_integral_along_froxel = (in_scattered_radiance - in_scattered_radiance * exp(-extinction * froxel_length)) / extinction;
   accum_scattering += accum_transmittance* inscatt_integral_along_froxel;
   accum_transmittance *= exp(-extinction*froxel_length);

   imageStore(scattering_extinction_image, ivec3(gl_GlobalInvocationID.xyz), vec4(in_scattered_radiance, extinction));

}