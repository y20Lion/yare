/// Ray descriptor
typedef struct _Ray
{
   /// xyz - origin, w - max range
   float4 origin;
   /// xyz - direction, w - time
   float4 direction;
   /// x - ray mask, y - activity flag
   int2 extra;
   int2 padding;
} Ray;


__kernel void updateRaysDirection(float3 ray_direction, int ray_count, __global Ray* rays)
{
   int thread_id = get_global_id(0);
   if (thread_id < ray_count)
   {
      rays[thread_id].direction.xyz = ray_direction;
   }
}

static float3 getVoxelPosition(int3 position, float3 volume_size_in_voxels, float3 volume_size, float3 volume_position)
{
   float3 corner = volume_position - 0.5f*volume_size;
   return corner + volume_size * (convert_float3(position) + (float3)(0.5f)) / volume_size_in_voxels;
}

__kernel void initRays(float3 volume_size_in_voxels_, float3 volume_size, float3 volume_position, int ray_count, __global Ray* rays)
{
   int3 volume_size_in_voxels = convert_int3(volume_size_in_voxels_);
   
   int thread_id = get_global_id(0);
   if (thread_id < ray_count)
   {
      
      int z = thread_id / (volume_size_in_voxels.x*volume_size_in_voxels.y);
      int y = (thread_id - z*(volume_size_in_voxels.x*volume_size_in_voxels.y)) / volume_size_in_voxels.x;
      int x = (thread_id - z*(volume_size_in_voxels.x*volume_size_in_voxels.y) - y*volume_size_in_voxels.x);

      float3 voxel_position = getVoxelPosition((int3)(x, y, z), volume_size_in_voxels_, volume_size, volume_position);

      __global Ray* ray = &rays[thread_id];
      ray->origin = (float4)(voxel_position, 1000000.0f);
      ray->direction = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
      ray->extra = (int2)(0xFFFFFFFF, 1);
   }
}

__kernel void accumulateAO(int ray_count, __global int* intersections, __global float* ao_texture)
{
   int thread_id = get_global_id(0);
   if (thread_id < ray_count)
   {
      int shape_id = intersections[thread_id];
      ao_texture[thread_id] += (shape_id != -1 ? 1.0 : 0.0);
   }
}
