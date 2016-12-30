~~~~~~~~~~~~~~~~~~~ VertexShader ~~~~~~~~~~~~~~~~~~~~~
#include "glsl_global_defines.h"
#include "glsl_voxelizer_defines.h"
#include "scene_uniforms.glsl"

layout(location = BI_VOXELS_TEXTURE) uniform sampler3D voxels;
layout(location = BI_VOXELS_AABB_PMIN) uniform vec3 aabb_pmin;
layout(location = BI_VOXELS_AABB_PMAX) uniform vec3 aabb_pmax;

layout(location = 0) in vec3 position;

void main()
{
   ivec3 size = textureSize(voxels, 0);

   int voxel_id = gl_VertexID / 24;

   ivec3 voxel_coords;
   voxel_coords.z = voxel_id / (size.y * size.x );
   int slice_flat = voxel_id % (size.y * size.x );
   voxel_coords.y = slice_flat / (size.x);
   voxel_coords.x = slice_flat % (size.x);

   float opacity = texelFetch(voxels, voxel_coords, 0).a;
   if (opacity > 0.0)
   {
      vec3 position_in_world = position * (aabb_pmax-aabb_pmin) + aabb_pmin;
      
      gl_Position = matrix_proj_world * vec4(position_in_world, 1.0);
   
   }
   else
      gl_Position = vec4(-10.0f, -10.0f, -10.0f, 1.0f);
}

~~~~~~~~~~~~~~~~~~ FragmentShader ~~~~~~~~~~~~~~~~~~~~~~

layout(location = 0) out vec4 shading_result;

void main()
{
   shading_result = vec4(0.0, 1.0, 0.0, 1.0);
}
