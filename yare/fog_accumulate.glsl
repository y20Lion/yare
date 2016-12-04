~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ComputeShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include "glsl_volumetric_fog_defines.h"
#include "glsl_global_defines.h"
#include "scene_uniforms.glsl"
#include "lighting_uniforms.glsl"
#include "common.glsl"
#include "lighting.glsl"

layout(binding = BI_FOG_VOLUME_IMAGE, rgba16f) uniform restrict writeonly image3D fog_image;
layout(location = BI_MATRIX_WORLD_VIEW) uniform mat4 matrix_world_view;
layout(location = BI_FRUSTUM) uniform vec4 frustum;


layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

float convertFruxelZtoDepth(float fruxel_z, float znear, float zfar)
{
   float z = pow(fruxel_z, 2.0);

   return mix(znear, zfar, z);
}

vec3 convertFruxelToCameraCoords(vec3 fruxel_coords, vec3 fruxels_dims)
{
   float z = convertFruxelZtoDepth(fruxel_coords.z / fruxels_dims.z, znear, zfar);

   float ratio = z / znear;
   float x = mix(frustum.x, frustum.y, fruxel_coords.x / fruxels_dims.x) * ratio;
   float y = mix(frustum.z, frustum.w, fruxel_coords.y / fruxels_dims.y) * ratio;

   return vec3(x, y, -z);
}

void main()
{
   float accum_transmittance = 1.0;
   vec3 accum_scattering = vec3(0.0);
   ivec3 volume_size = imageSize(fog_image);
   float previous_depth = znear;

   for (int z = 0; z < volume_size.z; ++z)
   {
      float current_depth = convertFruxelZtoDepth(float(z) / volume_size.z, znear, zfar);
      float fruxel_length = current_depth - previous_depth;
      float extinction = 0.1;
      float scattering = 1000.0f;

      vec3 fruxel_in_view = convertFruxelToCameraCoords(vec3(gl_GlobalInvocationID.xy, z), volume_size);
      vec3 fruxel_center_in_world = mat4x3(matrix_world_view) * vec4(fruxel_in_view, 1.0);

      vec3 fruxel_coords01 = vec3(gl_GlobalInvocationID.xy, z)/volume_size;
      ivec3 current_cluster_coords = ivec3(light_clusters_dims * fruxel_coords01);
      uvec2 cluster_data = texelFetch(light_list_head, current_cluster_coords, 0).xy;
      unsigned int start_offset = cluster_data.x;
      cluster_light_lists.sphere_light_count = cluster_data.y & 0x3FF;
      cluster_light_lists.spot_light_count = (cluster_data.y >> 10) & 0x3FF;
      cluster_light_lists.rectangle_light_count = (cluster_data.y >> 20) & 0x3FF;

      vec3 in_scattered_radiance = vec3(0.0);
      for (unsigned int i = 0; i < cluster_light_lists.sphere_light_count; ++i)
      {
         int light_index = light_list_data[start_offset + i];
         SphereLight light = sphere_lights[light_index];

         in_scattered_radiance += 1/(4*PI)* scattering *pointLightIncidentRadiance(light.color, light.position, light.radius, fruxel_center_in_world);
      }
      start_offset += cluster_light_lists.sphere_light_count;

      float fruxel_transmittance = exp(-extinction*fruxel_length);

      accum_scattering += in_scattered_radiance * accum_transmittance;
      accum_transmittance *= fruxel_transmittance;
      
      imageStore(fog_image, ivec3(gl_GlobalInvocationID.xy, z), vec4(accum_scattering, accum_transmittance));

      previous_depth = current_depth;
   }  
}
