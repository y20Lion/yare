~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ComputeShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include "glsl_volumetric_fog_defines.h"
#include "glsl_global_defines.h"
#include "scene_uniforms.glsl"
#include "lighting_uniforms.glsl"
#include "common.glsl"
#include "lighting.glsl"

layout(binding = BI_FOG_VOLUME_IMAGE, rgba16f) uniform restrict writeonly image3D fog_image;
layout(binding = BI_INSCATTERING_EXTINCTION_VOLUME) uniform sampler3D inscattering_extinction;

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
   ivec3 volume_size = imageSize(fog_image);
   float previous_depth = znear;

   for (int z = 0; z < volume_size.z; ++z)
   {
      float current_depth = convertFroxelZtoDepth(float(z) / volume_size.z, znear, zfar);
      float froxel_length = current_depth - previous_depth;
      
      vec4 fog_info = texture(inscattering_extinction, (vec3(gl_GlobalInvocationID.xy, z)+0.5)/volume_size);
      vec3 in_scattered_radiance = fog_info.rgb;
      float extinction = fog_info.a;

      vec3 inscatt_integral_along_froxel = (in_scattered_radiance - in_scattered_radiance * exp(-extinction * froxel_length)) / extinction;
      accum_scattering += accum_transmittance* inscatt_integral_along_froxel;
      accum_transmittance *= exp(-extinction*froxel_length);
      
      imageStore(fog_image, ivec3(gl_GlobalInvocationID.xy, z), vec4(accum_scattering, accum_transmittance));
      previous_depth = current_depth;
   }  
}
