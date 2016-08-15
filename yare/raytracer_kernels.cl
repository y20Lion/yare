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

typedef struct _Intersection
{
   // UV parametrization
   float4 uvwt;
   // Shape ID
   int shapeid;
   // Primitve ID
   int primid;

   int padding0;
   int padding1;
} Intersection;

static float3 getVoxelPosition(int3 position, float3 volume_size_in_voxels, float3 volume_size, float3 volume_position)
{
   float3 corner = volume_position - 0.5f*volume_size;
   return corner + volume_size * (convert_float3(position) + (float3)(0.5f)) / volume_size_in_voxels;
}

__kernel void initRays(float3 volume_size_in_voxels_, float3 volume_size, float3 volume_position, int ray_count, __global Ray* rays)
{
   int3 volume_size_in_voxels = convert_int3(volume_size_in_voxels_);

   int3 voxel_id = (int3)(get_global_id(0), get_global_id(1), get_global_id(2));
   float3 voxel_position = getVoxelPosition(voxel_id, volume_size_in_voxels_, volume_size, volume_position);

   int ray_offset = (voxel_id.x + voxel_id.y*volume_size_in_voxels.x + voxel_id.z*(volume_size_in_voxels.x*volume_size_in_voxels.y));
   __global Ray* ray = rays + ray_offset;
   ray->origin = (float4)(voxel_position, 1000000.0f);
   ray->direction = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
   ray->extra = (int2)(0xFFFFFFFF, 1);
}

__kernel void updateRaysDirection(float3 ray_direction, int ray_count, __global Ray* rays)
{
   int thread_id = get_global_id(0);
   rays[thread_id].direction.xyz = ray_direction;
}

__kernel void accumulateAO(int ray_count, __global int* intersections, __global float* ao_texture)
{
   int thread_id = get_global_id(0);
   int shape_id = intersections[thread_id];
   ao_texture[thread_id] += (shape_id != -1 ? 1.0 : 0.0);
}

__kernel void accumulateSDF(int ray_count,
                            float3 ray_direction,
                            __global Intersection* intersections,
                            __global float3* face_normals,
                            __global float3* sdf_accumulation)
{
   int thread_id = get_global_id(0);
   int shape_offset = intersections[thread_id].shapeid;
   int face = intersections[thread_id].primid;
   float hit_distance = intersections[thread_id].uvwt.w;

   if (shape_offset != -1)
   {
      float3 normal = face_normals[shape_offset + face];
      float hit_side = dot(normal, -ray_direction) > 0 ? 1.0f : -1.0f;

      sdf_accumulation[thread_id].x = min(sdf_accumulation[thread_id].x, hit_distance);
      sdf_accumulation[thread_id].y += hit_side;
      sdf_accumulation[thread_id].z += 1.0f;
   }   
}

