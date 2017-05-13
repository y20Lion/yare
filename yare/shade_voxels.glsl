~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ComputeShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include "glsl_voxelizer_defines.h"
#include "glsl_global_defines.h"
#include "scene_uniforms.glsl"
#include "lighting_uniforms.glsl"
#include "common.glsl"
#include "lighting.glsl"

layout(local_size_x = 4, local_size_y = 4, local_size_z = 4) in;

layout(binding = BI_VOXELS_GBUFFER_IMAGE, rgba16f) uniform readonly image3D gbuffer_voxels;
layout(binding = BI_VOXELS_ILLUMINATION_IMAGE, rgba16f) uniform writeonly image3D illumination_voxels;
layout(location = BI_VOXELS_AABB_PMIN) uniform vec3 aabb_pmin;
layout(location = BI_VOXELS_AABB_PMAX) uniform vec3 aabb_pmax;

void main()
{
   
   
   ivec3 voxel_coord = ivec3(gl_GlobalInvocationID.xyz);
   vec3 voxel_grid_dim = imageSize(illumination_voxels);
   vec4 data = imageLoad(gbuffer_voxels, voxel_coord);
   vec3 normal = data.xyz;
   if (data.a == 0)
      return;

   vec3 voxel_center_in_world = (vec3(voxel_coord))/voxel_grid_dim * (aabb_pmax-aabb_pmin) + aabb_pmin;
   float voxel_length = (aabb_pmax.x-aabb_pmin.x)/voxel_grid_dim.x;

   vec3 irradiance = vec3(0.0);   
   for (unsigned int i = 0; i < sphere_lights.length(); ++i)
   {
      vec3 light_dir = (sphere_lights[i].position - voxel_center_in_world);
      irradiance += sphereLightIncidentRadiance(sphere_lights[i], voxel_center_in_world, voxel_length) * saturate(dot(light_dir, normal));
   }

   for (unsigned int i = 0; i < spot_lights.length(); ++i)
   {   
      vec3 light_dir = (spot_lights[i].position - voxel_center_in_world);   
      irradiance += spotLightIncidentRadiance(spot_lights[i], voxel_center_in_world, voxel_length) * saturate(dot(light_dir, normal));
   }
   

   imageStore(illumination_voxels, voxel_coord, vec4(irradiance, 1.0));
   //imageStore(illumination_voxels, voxel_coord, vec4( 1.0));
}
