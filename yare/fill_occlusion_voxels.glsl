~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ComputeShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include "glsl_voxelizer_defines.h"

layout(local_size_x = 4, local_size_y = 4, local_size_z = 4) in;

layout(binding = BI_VOXELS_GBUFFER_IMAGE, rgba16f) uniform readonly image3D gbuffer_voxels;
layout(binding = BI_VOXELS_OCCLUSION_IMAGE, r8) uniform writeonly image3D occlusion_voxels;

void main()
{
   ivec3 voxel_coord = ivec3(gl_GlobalInvocationID.xyz);
   float opacity = imageLoad(gbuffer_voxels, voxel_coord).a;
   imageStore(occlusion_voxels, voxel_coord, vec4(opacity));
}
