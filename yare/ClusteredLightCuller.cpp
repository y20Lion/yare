#include "ClusteredLightCuller.h"

#include <glm/common.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <chrono>
#include <iostream>
#include <immintrin.h>

#include "glsl_global_defines.h"
#include "GLDevice.h"
#include "GLTexture.h"
#include "GLBuffer.h"
#include "GLProgram.h"
#include "GLVertexSource.h"
#include "Scene.h"
#include "Aabb3.h"
#include "RenderResources.h"
#include "matrix_math.h"
#include "RenderEngine.h"
#include "simd.h"

#define USE_CHEAP_SPHERE_INJECTION

namespace yare {

static int cMaxLightsPerCluster = 100;

ClusteredLightCuller::ClusteredLightCuller(const RenderResources& render_resources, const RenderSettings& settings)
   : _rr(render_resources)
   , _settings(settings)
{
   _light_clusters_dims = ivec3(24, 24, 32);
   _cluster_info.resize((_light_clusters_dims.x + 1) * (_light_clusters_dims.y + 1) * (_light_clusters_dims.z + 1));
   int macro_cluster_count = _light_clusters_dims.x * _light_clusters_dims.y * _light_clusters_dims.z / 2 / 2;
   _macro_clusters.resize(macro_cluster_count);
   _macro_cluster_info.center_cs.x = (vec4*)_aligned_malloc(sizeof(vec4)*macro_cluster_count, 16);
   _macro_cluster_info.center_cs.y = (vec4*)_aligned_malloc(sizeof(vec4)*macro_cluster_count, 16);
   _macro_cluster_info.center_cs.z = (vec4*)_aligned_malloc(sizeof(vec4)*macro_cluster_count, 16);

   _macro_cluster_info.extent_cs.x = (vec4*)_aligned_malloc(sizeof(vec4)*macro_cluster_count, 16);
   _macro_cluster_info.extent_cs.y = (vec4*)_aligned_malloc(sizeof(vec4)*macro_cluster_count, 16);
   _macro_cluster_info.extent_cs.z = (vec4*)_aligned_malloc(sizeof(vec4)*macro_cluster_count, 16);

   _light_list_head_pbo = createDynamicBuffer(_light_clusters_dims.x* _light_clusters_dims.y* _light_clusters_dims.z * sizeof(uvec2));
   _light_list_head = createTexture3D(_light_clusters_dims.x, _light_clusters_dims.y, _light_clusters_dims.z, GL_RG32UI);
   _light_list_data = createDynamicBuffer(cMaxLightsPerCluster * sizeof(int) * _light_clusters_dims.x * _light_clusters_dims.y * _light_clusters_dims.z);
 
   _initDebugData();
}

ClusteredLightCuller::~ClusteredLightCuller()
{
   _aligned_free(_macro_cluster_info.center_cs.x);
   _aligned_free(_macro_cluster_info.center_cs.y);
   _aligned_free(_macro_cluster_info.center_cs.z);

   _aligned_free(_macro_cluster_info.extent_cs.x);
   _aligned_free(_macro_cluster_info.extent_cs.y);
   _aligned_free(_macro_cluster_info.extent_cs.z);
}

struct intAabb3
{
   ivec3 pmin;
   ivec3 pmax;
};

#define SQUARE(X) (X)*(X)

// From 2D Polyhedral Bounds of a Clipped, Perspective - Projected 3D Sphere
void sphereBoundsForAxis(const vec3& axis, const vec3& sphere_center, float sphere_radius, float znear, vec3 bounds_cs[2])
{
   bool sphere_clipping_with_znear = !((sphere_center.z + sphere_radius) < znear);

   // given in coordinates (a,z), where a is in the direction of the vector a, and z is in the standard z direction
   const vec2& projectedCenter = vec2(dot(axis, sphere_center), sphere_center.z);
   vec2 bounds_az[2];
   float tSquared = dot(projectedCenter, projectedCenter) - SQUARE(sphere_radius);
   float t, cLength, costheta, sintheta;

   bool camera_outside_sphere = (tSquared > 0);
   if (camera_outside_sphere)
   { // Camera is outside sphere
      // Distance to the tangent points of the sphere (points where a vector from the camera are tangent to the sphere) (calculated a-z space)
      t = sqrt(tSquared);
      cLength = length(projectedCenter);

      // Theta is the angle between the vector from the camera to the center of the sphere and the vectors from the camera to the tangent points
      costheta = t / cLength;
      sintheta = sphere_radius / cLength;
   }
   float sqrtPart;
   if (sphere_clipping_with_znear)
      sqrtPart = sqrt(SQUARE(sphere_radius) - SQUARE(znear - projectedCenter.y));

   sqrtPart *= -1;
   for (int i = 0; i < 2; ++i)
   {
      if (tSquared >  0)
      {
         const mat2& rotateTheta = mat2(costheta, -sintheta,
                                          sintheta, costheta);

         vec2 rotated = (rotateTheta * projectedCenter);
         vec2 norm = rotated / length(rotated);
         vec2 point = t * norm;

         bounds_az[i] = costheta * (rotateTheta * projectedCenter);

         int a = 0;
      }

      if (sphere_clipping_with_znear && (!camera_outside_sphere || bounds_az[i].y > znear))
      {
         bounds_az[i].x = projectedCenter.x + sqrtPart;
         bounds_az[i].y = znear;
      }
      sintheta *= -1; // negate theta for B
      sqrtPart *= -1; // negate sqrtPart for B
   }
   bounds_cs[0] = bounds_az[0].x * axis;
   bounds_cs[0].z = bounds_az[0].y;
   bounds_cs[1] = bounds_az[1].x * axis;
   bounds_cs[1].z = bounds_az[1].y;
}



/*bool _sphereOverlapsVoxel(const RenderSettings& settings, RenderData& render_data, float sphere_radius, const vec3& sphere_center, const ivec3& cluster_coords, const ivec3& cluster_dims, const Frustum& frustum)
{
   vec3 cluster_center = _clusterCorner(vec3(cluster_coords) + vec3(0.5f), cluster_dims, render_data);

   vec3 plane_normal = normalize(cluster_center - sphere_center);
   vec3 plane_origin = sphere_center + sphere_radius*plane_normal;


   bool intersection = false;
   intersection |= (dot(_clusterCorner(vec3(cluster_coords) + vec3(0, 0, 0), cluster_dims, render_data) - plane_origin, plane_normal) < 0);
   intersection |= (dot(_clusterCorner(vec3(cluster_coords) + vec3(0, 1, 0), cluster_dims, render_data) - plane_origin, plane_normal) < 0);
   intersection |= (dot(_clusterCorner(vec3(cluster_coords) + vec3(1, 1, 0), cluster_dims, render_data) - plane_origin, plane_normal) < 0);
   intersection |= (dot(_clusterCorner(vec3(cluster_coords) + vec3(1, 0, 0), cluster_dims, render_data) - plane_origin, plane_normal) < 0);

   intersection |= (dot(_clusterCorner(vec3(cluster_coords) + vec3(0, 0, 1), cluster_dims, render_data) - plane_origin, plane_normal) < 0);
   intersection |= (dot(_clusterCorner(vec3(cluster_coords) + vec3(0, 1, 1), cluster_dims, render_data) - plane_origin, plane_normal) < 0);
   intersection |= (dot(_clusterCorner(vec3(cluster_coords) + vec3(1, 1, 1), cluster_dims, render_data) - plane_origin, plane_normal) < 0);
   intersection |= (dot(_clusterCorner(vec3(cluster_coords) + vec3(1, 0, 1), cluster_dims, render_data) - plane_origin, plane_normal) < 0);

   return intersection;
}*/

__forceinline int _overlap(__m128 sse_plane_normal, __m128 sse_dot_plane, __m128* corner_a, __m128* corner_b, __m128* corner_c, __m128* corner_d)
{
   __m128 dota = _mm_dp_ps(*corner_a, sse_plane_normal, 0x70 | 0x1);
   __m128 dotb = _mm_dp_ps(*corner_b, sse_plane_normal, 0x70 | 0x2);
   __m128 dotc = _mm_dp_ps(*corner_c, sse_plane_normal, 0x70 | 0x4);
   __m128 dotd = _mm_dp_ps(*corner_d, sse_plane_normal, 0x70 | 0x8);

   __m128 all_dots = _mm_add_ps(dota, dotb);
   all_dots = _mm_add_ps(all_dots, dotc);
   all_dots = _mm_add_ps(all_dots, dotd);

   __m128 intersection_test = _mm_sub_ps(all_dots, sse_dot_plane);

   __m128 zero = _mm_setzero_ps();
   intersection_test = _mm_cmplt_ps(intersection_test, zero);

   return _mm_movemask_ps(intersection_test);
}

__forceinline __m128 _normalize(__m128 vec)
{
   return _mm_mul_ps(vec, _mm_rsqrt_ps(_mm_dp_ps(vec, vec, 0x7F)));
}


__forceinline int ClusteredLightCuller::_sphereOverlapsVoxelOptim(int x, int y, int z, float sphere_radius, const vec3& sphere_center, const ClusterInfo* cluster_infos)
{

   /*vec3 plane_normal = normalize(cluster_infos[_toFlatClusterIndex(x, y, z)].center_coord - sphere_center);
   vec3 plane_origin = sphere_center + sphere_radius*plane_normal;

   float dot_plane = dot(plane_origin, plane_normal);

   bool intersection = false;
   intersection |= (dot(cluster_infos[_toFlatClusterIndex(x + 0, y + 0, z + 1)].corner_coord , plane_normal) - dot_plane < 0);
   intersection |= (dot(cluster_infos[_toFlatClusterIndex(x + 1, y + 0, z + 1)].corner_coord , plane_normal) - dot_plane < 0);
   intersection |= (dot(cluster_infos[_toFlatClusterIndex(x + 1, y + 1, z + 1)].corner_coord , plane_normal) - dot_plane < 0);
   intersection |= (dot(cluster_infos[_toFlatClusterIndex(x + 0, y + 1, z + 1)].corner_coord , plane_normal) - dot_plane < 0);*/
   /*vec3 plane_normal = normalize(cluster_infos[_toFlatClusterIndex(x, y, z)].center_coord - sphere_center);
   vec3 plane_origin = sphere_center + sphere_radius*plane_normal;*/

   __m128* center_coord = (__m128*)&cluster_infos[_toFlatClusterIndex(x, y, z)].center_coord;
   __m128 sse_sphere_center = _mm_set_ps(1.0, sphere_center.z, sphere_center.y, sphere_center.x);
   __m128 sse_plane_normal = _normalize(_mm_sub_ps(*center_coord, sse_sphere_center));

   __m128 plane_origin = _mm_add_ps(sse_sphere_center, _mm_mul_ps(_mm_set1_ps(sphere_radius), sse_plane_normal));
   __m128 sse_dot_plane = _mm_dp_ps(plane_origin, sse_plane_normal, 0x70 | 0xF);

   __m128* corner_a = (__m128*)&cluster_infos[_toFlatClusterIndex(x + 0, y + 0, z + 0)].corner_coord;
   __m128* corner_b = (__m128*)&cluster_infos[_toFlatClusterIndex(x + 1, y + 0, z + 0)].corner_coord;
   __m128* corner_c = (__m128*)&cluster_infos[_toFlatClusterIndex(x + 1, y + 1, z + 0)].corner_coord;
   __m128* corner_d = (__m128*)&cluster_infos[_toFlatClusterIndex(x + 0, y + 1, z + 0)].corner_coord;

   if (_overlap(sse_plane_normal, sse_dot_plane, corner_a, corner_b, corner_c, corner_d))
      return 1;

   corner_a = (__m128*)&cluster_infos[_toFlatClusterIndex(x + 0, y + 0, z + 1)].corner_coord;
   corner_b = (__m128*)&cluster_infos[_toFlatClusterIndex(x + 1, y + 0, z + 1)].corner_coord;
   corner_c = (__m128*)& cluster_infos[_toFlatClusterIndex(x + 1, y + 1, z + 1)].corner_coord;
   corner_d = (__m128*)& cluster_infos[_toFlatClusterIndex(x + 0, y + 1, z + 1)].corner_coord;

   return (_overlap(sse_plane_normal, sse_dot_plane, corner_a, corner_b, corner_c, corner_d));
}

__forceinline int _aabbOverlapsFrustumOptim(simdvec3 aabb_center, simdvec3 aabb_extent, const simdvec3* frustum_planes_xyz, const simdfloat* frustum_planes_w, int num_planes)
{
   simdbool test = true;
   for (int i_plane = 0; i_plane < num_planes; ++i_plane)
   {
      simdvec3 plane_normal = frustum_planes_xyz[i_plane];

      simdfloat d = dot(aabb_center, plane_normal);
      simdfloat r = dot(aabb_extent, abs(plane_normal));

      simdbool is_inside = simdbool((d + r) >= -frustum_planes_w[i_plane]);
      test &= is_inside;
   }

   return _mm_movemask_ps(test.val);
}


intAabb3 _convertClipSpaceAABBToVoxel(const Aabb3& clip_space_aabb, const ivec3& light_clusters_dims)
{
   intAabb3 voxels_overlapping_light;
   voxels_overlapping_light.pmin.x = (int)clamp(floor(clip_space_aabb.pmin.x * light_clusters_dims.x), 0.0f, light_clusters_dims.x - 1.0f);
   voxels_overlapping_light.pmin.y = (int)clamp(floor(clip_space_aabb.pmin.y * light_clusters_dims.y), 0.0f, light_clusters_dims.y - 1.0f);
   voxels_overlapping_light.pmin.z = (int)clamp(floor(clip_space_aabb.pmin.z * light_clusters_dims.z), 0.0f, light_clusters_dims.z - 1.0f);
   voxels_overlapping_light.pmax.x = (int)clamp(ceil(clip_space_aabb.pmax.x * light_clusters_dims.x), 0.0f, light_clusters_dims.x - 1.0f);
   voxels_overlapping_light.pmax.y = (int)clamp(ceil(clip_space_aabb.pmax.y * light_clusters_dims.y), 0.0f, light_clusters_dims.y - 1.0f);
   voxels_overlapping_light.pmax.z = (int)clamp(ceil(clip_space_aabb.pmax.z * light_clusters_dims.z), 0.0f, light_clusters_dims.z - 1.0f);

   return voxels_overlapping_light;
}


static mat4 _matrix_light_proj_local;


__forceinline simdfloat _convertClusterZtoCameraZ(simdfloat cluster_z, simdfloat znear, simdfloat zfar, int cluster_z_distribution_factor)
{
   simdfloat z = cluster_z;
   for (int i = 0; i < cluster_z_distribution_factor - 1; ++i)
      z = z*cluster_z;//pow(cluster_z, simdfloat(3.0f));

   return -mix(znear, zfar, z);
}

__forceinline simdvec3 _clusterCorner(const simdvec3 ndc_coords, const simdvec3& cluster_dims, const simdfrustum& frustum, int cluster_z_distribution_factor)
{
   simdfloat z = _convertClusterZtoCameraZ(ndc_coords.z * cluster_dims.z, frustum.near, frustum.far, cluster_z_distribution_factor);

   simdfloat ratio = -z / frustum.near;
   simdfloat x = mix(frustum.left, frustum.right, ndc_coords.x * cluster_dims.x) * ratio;
   simdfloat y = mix(frustum.bottom, frustum.top, ndc_coords.y * cluster_dims.y) * ratio;

   return simdvec3(x, y, z);
}

__forceinline simdvec3 _project(const simdmat4& matrix, const simdvec3& point)
{
   simdvec4 vec_hs = matrix * simdvec4(point.x, point.y, point.z, simdfloat(1.0f));
   return simdvec3(vec_hs.x, vec_hs.y, vec_hs.z) / simdvec3(vec_hs.w);
}

__forceinline void _clusterCenterAndExtent2(const simdfrustum& frustum, const simdmat4& matrix_proj_view, const simdvec3& light_clusters_dims,
                                            simdfloat x, simdfloat y, simdfloat z, simdvec3* center, simdvec3* extent, int cluster_z_distribution_factor)
{
   simdvec3 aabb_min = _project(matrix_proj_view, _clusterCorner(simdvec3(x, y, z), light_clusters_dims, frustum, cluster_z_distribution_factor));
   simdvec3 aabb_max = _project(matrix_proj_view, _clusterCorner(simdvec3(x + simdfloat(1.0f), y + simdfloat(1.0f), z + simdfloat(1.0f)), light_clusters_dims, frustum, cluster_z_distribution_factor));

   *center = simdfloat(0.5f) * (aabb_min + aabb_max);
   *extent = simdfloat(0.5f) * (aabb_max - aabb_min);
}


void ClusteredLightCuller::buildLightLists(const Scene& scene, RenderData& render_data)
{
   

#ifndef USE_CHEAP_SPHERE_INJECTION
   for (int z = 0; z <= _light_clusters_dims.z; z++)
   {
      for (int y = 0; y <= _light_clusters_dims.y; y++)
      {
         for (int x = 0; x <= _light_clusters_dims.x; x++)
         {
            _cluster_info[_toFlatClusterIndex(x, y, z)].center_coord = _clusterCorner(vec3(x, y, z) + vec3(0.5), _light_clusters_dims, render_data);
            _cluster_info[_toFlatClusterIndex(x, y, z)].corner_coord = _clusterCorner(vec3(x, y, z), _light_clusters_dims, render_data);
         }
      }
   }
#endif

   for (int z = 0; z < _light_clusters_dims.z; z++)
   {
      for (int y = 0; y < (_light_clusters_dims.y / 2); y++)
      {
         for (int x = 0; x < (_light_clusters_dims.x / 2); x++)
         {
            _macro_clusters[_toFlatMacroClusterIndex(x, y, z)].lights[0].resize(0);
            _macro_clusters[_toFlatMacroClusterIndex(x, y, z)].lights[1].resize(0);
            _macro_clusters[_toFlatMacroClusterIndex(x, y, z)].lights[2].resize(0);
         }
      }
   }
   
   
   simdfrustum frustum;
   frustum.left   = simdfloat(render_data.frustum.left);
   frustum.right  = simdfloat(render_data.frustum.right);
   frustum.bottom = simdfloat(render_data.frustum.bottom);
   frustum.top    = simdfloat(render_data.frustum.top);
   frustum.near   = simdfloat(render_data.frustum.near);
   frustum.far    = simdfloat(render_data.frustum.far);

   simdmat4 matrix_proj_view(render_data.matrix_proj_view);
   vec3 temp = 1.0f/vec3(_light_clusters_dims);
   simdvec3 light_clusters_dims(temp);   

   for (int z = 0; z < _light_clusters_dims.z; z++)
   {
      for (int y = 0; y < (_light_clusters_dims.y / 2); y++)
      {
         for (int x = 0; x < (_light_clusters_dims.x / 2); x++)
         {
            simdfloat x_, y_, z_;
            x_.load(2.0f*x+1.0f, 2.0f*x, 2.0f*x+1.0f, 2.0f*x);
            y_.load(2.0f*y+1.0f, 2.0f*y+1.0f, 2.0f*y, 2.0f*y);
            z_.load(float(z), float(z), float(z), float(z));

            simdvec3 center, extent;
            _clusterCenterAndExtent2(frustum, matrix_proj_view, light_clusters_dims, x_, y_, z_, &center, &extent, (int)_settings.cluster_z_distribution_factor);

            int index = _toFlatMacroClusterIndex(x, y, z);
            _mm_store_ps((float*)&_macro_cluster_info.center_cs.x[index], center.x.val);
            _mm_store_ps((float*)&_macro_cluster_info.center_cs.y[index], center.y.val);
            _mm_store_ps((float*)&_macro_cluster_info.center_cs.z[index], center.z.val);

            _mm_store_ps((float*)&_macro_cluster_info.extent_cs.x[index], extent.x.val);
            _mm_store_ps((float*)&_macro_cluster_info.extent_cs.y[index], extent.y.val);
            _mm_store_ps((float*)&_macro_cluster_info.extent_cs.z[index], extent.z.val);
         }
      }
   }
   /*for (int z = 0; z < _light_clusters_dims.z; z++)
   {
      for (int y = 0; y < (_light_clusters_dims.y / 2); y++)
      {
         for (int x = 0; x < (_light_clusters_dims.x / 2); x++)
         {          
            
            vec3 cluster_center[4];
            vec3 cluster_extent[4];

            for (int i = 0; i < 4; ++i)
            {
               int sub_x = 2 * x + (i % 2);
               int sub_y = 2 * y + (i / 2);

               _clusterCenterAndExtent(render_data, _light_clusters_dims, sub_x, sub_y, z, &cluster_center[i], &cluster_extent[i]);
            }

            _macro_cluster_info.center_cs.x[_toFlatMacroClusterIndex(x, y, z)] = vec4(cluster_center[0].x, cluster_center[1].x, cluster_center[2].x, cluster_center[3].x);
            _macro_cluster_info.center_cs.y[_toFlatMacroClusterIndex(x, y, z)] = vec4(cluster_center[0].y, cluster_center[1].y, cluster_center[2].y, cluster_center[3].y);
            _macro_cluster_info.center_cs.z[_toFlatMacroClusterIndex(x, y, z)] = vec4(cluster_center[0].z, cluster_center[1].z, cluster_center[2].z, cluster_center[3].z);

            _macro_cluster_info.extent_cs.x[_toFlatMacroClusterIndex(x, y, z)] = vec4(cluster_extent[0].x, cluster_extent[1].x, cluster_extent[2].x, cluster_extent[3].x);
            _macro_cluster_info.extent_cs.y[_toFlatMacroClusterIndex(x, y, z)] = vec4(cluster_extent[0].y, cluster_extent[1].y, cluster_extent[2].y, cluster_extent[3].y);
            _macro_cluster_info.extent_cs.z[_toFlatMacroClusterIndex(x, y, z)] = vec4(cluster_extent[0].z, cluster_extent[1].z, cluster_extent[2].z, cluster_extent[3].z);
         }
      }
   }*/

   auto start = std::chrono::steady_clock::now();
      
   _injectSphereLightsIntoClusters(scene, render_data);
   _injectSpotLightsIntoClusters(scene, render_data);
   _injectRectangleLightsIntoClusters(scene, render_data);
   float duration = std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count();
   //std::cout << duration*1000.0f << std::endl;
   _updateClustersGLData();
   
}

vec3* _drawCross(const vec3& center, vec3* buffer)
{
   float size = 0.025f;
   buffer[0] = center + vec3(size, 0.0f, 0.0f);
   buffer[1] = center + vec3(-size, 0.0f, 0.0f);
   buffer[2] = center + vec3(0.0f, size, 0.0f);
   buffer[3] = center + vec3(0.0f, -size, 0.0f);
   buffer += 4;
   return buffer;
}

void ClusteredLightCuller::drawClusterGrid(const RenderData& render_data, int index)
{
   GLDevice::bindProgram(*_debug_draw_cluster_grid);
   GLDevice::bindUniformMatrix4(0, glm::inverse(_debug_render_data.matrix_proj_world));
   glUniform2f(1, _debug_render_data.frustum.near, _debug_render_data.frustum.far);
   glUniform1i(2, index);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _debug_enabled_clusters->id());
   GLDevice::draw(*_debug_cluster_grid_vertex_source);
   //glDisable(GL_DEPTH_TEST);

   mat4 debug_matrix_world_view = inverse(_debug_render_data.matrix_view_world);
   vec3* line_data = (vec3*)_debug_lines_buffer->map(GL_MAP_WRITE_BIT);


   for (int i = 0; i < 100; ++i)
   {
      line_data = _drawCross(render_data.points[i], line_data);
   }

   _debug_lines_buffer->unmap();

   _debug_lines_source->setVertexCount(100 * 4);
   GLDevice::bindProgram(*_debug_draw);
   GLDevice::bindUniformMatrix4(0, _matrix_light_proj_local);
   GLDevice::draw(*_debug_lines_source);

   glUseProgram(0);
   //glEnable(GL_DEPTH_TEST);
}

void ClusteredLightCuller::debugUpdateClusteredGrid(RenderData& render_data)
{
   _debug_render_data = render_data;


   _updateClustersGLData();

   int* enabled_clusters = (int*)_debug_enabled_clusters->map(GL_MAP_WRITE_BIT);
   for (int i = 0; i <_macro_clusters.size(); ++i)
   {
      const auto& cpu_cluster = _macro_clusters[i];
      for (int sub_cluster = 0; sub_cluster < 4; sub_cluster++)
      {
         int macro_z = i / (_light_clusters_dims.y * _light_clusters_dims.x / 4);
         int slice_flat = i % (_light_clusters_dims.y * _light_clusters_dims.x / 4);
         int macro_y = slice_flat / (_light_clusters_dims.x / 2);
         int macro_x = slice_flat % (_light_clusters_dims.x / 2);

         int sub_x = 2 * macro_x + (sub_cluster % 2);
         int sub_y = 2 * macro_y + (sub_cluster / 2);
         int sub_z = macro_z;

         unsigned int rectangle_light_count = 0;
         unsigned int spot_light_count = 0;
         unsigned int sphere_light_count = 0;

         enabled_clusters[_toFlatClusterIndex(sub_x, sub_y, sub_z)] = false;
         for (int j = 0; j < cpu_cluster.lights[int(LightType::Sphere)].size(); ++j)
         {
            if ((1 << sub_cluster) & cpu_cluster.lights[int(LightType::Sphere)][j].mask)
               enabled_clusters[_toFlatClusterIndex(sub_x, sub_y, sub_z)] = true;
         }
      }
   }
   _debug_enabled_clusters->unmap();


}

void ClusteredLightCuller::bindLightLists()
{
   glBindBufferRange(GL_SHADER_STORAGE_BUFFER, BI_LIGHT_LIST_DATA_SSBO, _light_list_data->id(),
                     _light_list_data->getRenderSegmentOffset(), _light_list_data->segmentSize());

   GLDevice::bindTexture(BI_LIGHT_LIST_HEAD, *_light_list_head, *_rr.samplers.mipmap_clampToEdge);
}

void ClusteredLightCuller::updateLightListHeadTexture()
{
   _light_list_head->updateFromBuffer(*_light_list_head_pbo, _light_list_head_pbo->getRenderSegmentOffset());
}


int ClusteredLightCuller::_toFlatClusterIndex(int x, int y, int z)
{
   return x + y*_light_clusters_dims.x + z*(_light_clusters_dims.x*_light_clusters_dims.y);
}

int ClusteredLightCuller::_toFlatMacroClusterIndex(int x, int y, int z)
{
   return x + y*(_light_clusters_dims.x / 2) + z*(_light_clusters_dims.x*_light_clusters_dims.y / 4);
}

struct ClusterListHead
{
   unsigned int list_start_offset;
   unsigned int light_counts;
};

void ClusteredLightCuller::_updateClustersGLData()
{
   auto start = std::chrono::steady_clock::now();

   ClusterListHead* gpu_lists_head = (ClusterListHead*)_light_list_head_pbo->getUpdateSegmentPtr();
   int* gpu_lists_data = (int*)_light_list_data->getUpdateSegmentPtr();

   unsigned int offset_into_data_buffer = 0;
   int macro_cluster_count = _light_clusters_dims.x * _light_clusters_dims.y * _light_clusters_dims.z / 4;
   for (int i = 0; i < macro_cluster_count; ++i)
   {
      const auto& cpu_cluster = _macro_clusters[i];

      for (int sub_cluster = 0; sub_cluster < 4; sub_cluster++)
      {
         int macro_z = i / (_light_clusters_dims.y * _light_clusters_dims.x / 4);
         int slice_flat = i % (_light_clusters_dims.y * _light_clusters_dims.x / 4);
         int macro_y = slice_flat / (_light_clusters_dims.x / 2);
         int macro_x = slice_flat % (_light_clusters_dims.x / 2);

         int sub_x = 2 * macro_x + (sub_cluster % 2);
         int sub_y = 2 * macro_y + (sub_cluster / 2);
         int sub_z = macro_z;

         gpu_lists_head[_toFlatClusterIndex(sub_x, sub_y, sub_z)].list_start_offset = offset_into_data_buffer;
         unsigned int rectangle_light_count = 0;
         unsigned int spot_light_count = 0;
         unsigned int sphere_light_count = 0;

         const auto& sphere_lights = cpu_cluster.lights[int(LightType::Sphere)];
         for (int j = 0; j < sphere_lights.size(); ++j)
         {
            if ((1 << sub_cluster) & sphere_lights[j].mask)
               gpu_lists_data[offset_into_data_buffer + sphere_light_count++] = sphere_lights[j].light_index;
         }
         offset_into_data_buffer += sphere_light_count;

         const auto& spot_lights = cpu_cluster.lights[int(LightType::Spot)];
         for (int j = 0; j < spot_lights.size(); ++j)
         {
            if ((1 << sub_cluster) & spot_lights[j].mask)
               gpu_lists_data[offset_into_data_buffer + spot_light_count++] = spot_lights[j].light_index;
         }
         offset_into_data_buffer += spot_light_count;

         const auto& rectangle_lights = cpu_cluster.lights[int(LightType::Rectangle)];
         for (int j = 0; j < rectangle_lights.size(); ++j)
         {
            if ((1 << sub_cluster) & rectangle_lights[j].mask)
               gpu_lists_data[offset_into_data_buffer + rectangle_light_count++] = rectangle_lights[j].light_index;
         }
         offset_into_data_buffer += rectangle_light_count;

         gpu_lists_head[_toFlatClusterIndex(sub_x, sub_y, sub_z)].light_counts =
            (unsigned int(sphere_light_count) & 0x3FF)
            | ((unsigned int(spot_light_count) & 0x3FF) << 10)
            | ((unsigned int(rectangle_light_count) & 0x3FF) << 20);

      }
   }

   float duration = std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count();
   //std::cout << "in " << duration*1000.0f << std::endl;
}

void ClusteredLightCuller::_injectSphereLightsIntoClusters(const Scene& scene, const RenderData& render_data)
{
#ifdef USE_CHEAP_SPHERE_INJECTION
   unsigned short light_index = -1;
   for (const auto& light : scene.lights)
   {
      if (light.type != LightType::Sphere)
         continue;

      light_index++;

      mat4 matrix_light_proj_local = render_data.matrix_proj_world*toMat4(light.world_to_local_matrix);
      mat4 matrix_plane_clip_to_local = transpose(inverse(matrix_light_proj_local));

      simdvec3 light_clip_planes_xyz[6];
      simdfloat light_clipplanes_w[6];
      for (int i = 0; i < 6; ++i)
      {
         vec4 oobb_planes_in_clip_space = matrix_plane_clip_to_local * light.frustum_planes_in_local[i];
         light_clip_planes_xyz[i] = simdvec3(oobb_planes_in_clip_space.xyz);
         light_clipplanes_w[i] = simdfloat(oobb_planes_in_clip_space.w);
      }

      Aabb3 clip_space_aabb = _computeSphereClusterBounds(scene, render_data, light);

      _injectLightIntoClusters(clip_space_aabb, light_index, LightType::Sphere, light_clip_planes_xyz, light_clipplanes_w, 6);
   }
#else
   unsigned short light_index = -1;
   for (const auto& light : scene.lights)
   {
      if (light.type != LightType::Sphere)
         continue;

      light_index++;
      Aabb3 clip_space_aabb = _computeSphereClusterBounds(scene, render_data, light);
      intAabb3 voxels_overlapping_light = _convertClipSpaceAABBToVoxel(clip_space_aabb, _light_clusters_dims);
      voxels_overlapping_light.pmin.x = voxels_overlapping_light.pmin.x / 2;
      voxels_overlapping_light.pmax.x = voxels_overlapping_light.pmax.x / 2;

      voxels_overlapping_light.pmin.y = voxels_overlapping_light.pmin.y / 2;
      voxels_overlapping_light.pmax.y = voxels_overlapping_light.pmax.y / 2;

      vec3 sphere_center_in_vs = project(render_data.matrix_view_world, light.world_to_local_matrix[3]);

      for (int z = voxels_overlapping_light.pmin.z; z <= voxels_overlapping_light.pmax.z; z++)
      {
         for (int y = voxels_overlapping_light.pmin.y; y <= voxels_overlapping_light.pmax.y; y++)
         {
            for (int x = voxels_overlapping_light.pmin.x; x <= voxels_overlapping_light.pmax.x; x++)
            {
               int mask = 0;
               for (int sub_cluster = 0; sub_cluster < 4; ++sub_cluster)
               {
                  int sub_x = 2 * x + (sub_cluster % 2);
                  int sub_y = 2 * y + (sub_cluster / 2);
                  int sub_z = z;

                  int res = bool(_sphereOverlapsVoxelOptim(sub_x, sub_y, sub_z, light.radius, vec3(sphere_center_in_vs), _cluster_info.data()));
                  mask |= res << sub_cluster;
               }

               if (mask != 0)
                  _macro_clusters[_toFlatMacroClusterIndex(x, y, z)].lights[int(LightType::Sphere)].emplace_back(LightCoverage(mask, light_index));
            }
         }
      }
   }
#endif
}

void ClusteredLightCuller::_injectSpotLightsIntoClusters(const Scene& scene, const RenderData& render_data)
{
   unsigned short light_index = -1;
   for (const auto& light : scene.lights)
   {
      if (light.type != LightType::Spot)
         continue;

      light_index++;

      mat4 matrix_light_proj_local = render_data.matrix_proj_world*toMat4(light.world_to_local_matrix);
      mat4 matrix_plane_clip_to_local = transpose(inverse(matrix_light_proj_local));

      simdvec3 frustum_planes_xyz[5];
      simdfloat frustum_planes_w[5];
      for (int i = 0; i < 5; ++i)
      {
         vec4 spot_planes_in_clip_space = matrix_plane_clip_to_local * light.frustum_planes_in_local[i];
         frustum_planes_xyz[i] = simdvec3(spot_planes_in_clip_space.xyz);
         frustum_planes_w[i] = simdfloat(spot_planes_in_clip_space.w);
      }

      float w = light.radius * tan(light.spot.angle*0.5f);
      float l = light.radius;

      vec3 pyramid_vertices[8];
      pyramid_vertices[0] = vec3(0.0f, 0.0f, 0.0f);
      pyramid_vertices[1] = vec3(w, w, -l);
      pyramid_vertices[2] = vec3(-w, w, -l);
      pyramid_vertices[3] = vec3(-w, -w, -l);
      pyramid_vertices[4] = vec3(w, -w, -l);

      Aabb3 clip_space_aabb = _computeConvexMeshClusterBounds(render_data, matrix_light_proj_local, pyramid_vertices, 5);

      _injectLightIntoClusters(clip_space_aabb, light_index, LightType::Spot, frustum_planes_xyz, frustum_planes_w, 5);
   }
}

void ClusteredLightCuller::_injectRectangleLightsIntoClusters(const Scene& scene, const RenderData& render_data)
{
   unsigned short light_index = -1;
   for (const auto& light : scene.lights)
   {
      if (light.type != LightType::Rectangle)
         continue;

      light_index++;

      mat4 matrix_light_proj_local = render_data.matrix_proj_world*toMat4(light.world_to_local_matrix);
      mat4 matrix_plane_clip_to_local = transpose(inverse(matrix_light_proj_local));

      simdvec3 light_clip_planes_xyz[6];
      simdfloat light_clipplanes_w[6];
      for (int i = 0; i < 6; ++i)
      {
         vec4 oobb_planes_in_clip_space = matrix_plane_clip_to_local * light.frustum_planes_in_local[i];
         light_clip_planes_xyz[i] = simdvec3(oobb_planes_in_clip_space.xyz);
         light_clipplanes_w[i] = simdfloat(oobb_planes_in_clip_space.w);
      }

      float w = light.rectangle.bounds_width;
      float h = light.rectangle.bounds_height;
      float d = light.rectangle.bounds_depth;

      vec3 oobb_vertices[8];
      oobb_vertices[0] = vec3(w, h, 0.0);
      oobb_vertices[1] = vec3(-w, h, 0.0);
      oobb_vertices[2] = vec3(-w, -h, 0.0);
      oobb_vertices[3] = vec3(w, -h, 0.0);
      oobb_vertices[4] = vec3(w, h, -d);
      oobb_vertices[5] = vec3(-w, h, -d);
      oobb_vertices[6] = vec3(-w, -h, -d);
      oobb_vertices[7] = vec3(w, -h, -d);

      Aabb3 clip_space_aabb = _computeConvexMeshClusterBounds(render_data, matrix_light_proj_local, oobb_vertices, 8);

      _injectLightIntoClusters(clip_space_aabb, light_index, LightType::Rectangle, light_clip_planes_xyz, light_clipplanes_w, 6);
   }
}

void ClusteredLightCuller::_injectLightIntoClusters(const Aabb3& clip_space_aabb, unsigned short light_index, LightType light_type,
                                                      const simdvec3* light_clip_planes_xyz, const simdfloat* light_clip_planes_w, int num_light_clip_planes)
{
   intAabb3 voxels_overlapping_light = _convertClipSpaceAABBToVoxel(clip_space_aabb, _light_clusters_dims);
   voxels_overlapping_light.pmin.x = voxels_overlapping_light.pmin.x / 2;
   voxels_overlapping_light.pmax.x = voxels_overlapping_light.pmax.x / 2;

   voxels_overlapping_light.pmin.y = voxels_overlapping_light.pmin.y / 2;
   voxels_overlapping_light.pmax.y = voxels_overlapping_light.pmax.y / 2;

#pragma omp parallel for num_threads(3)
   for (int z = voxels_overlapping_light.pmin.z; z <= voxels_overlapping_light.pmax.z; z++)
   {
      for (int y = voxels_overlapping_light.pmin.y; y <= voxels_overlapping_light.pmax.y; y++)
      {
         for (int x = voxels_overlapping_light.pmin.x; x <= voxels_overlapping_light.pmax.x; x++)
         {
            simdvec3 aabb_center;
            simdvec3 aabb_extent;
            aabb_center.load((float*)_macro_cluster_info.center_cs.x, (float*)_macro_cluster_info.center_cs.y, (float*)_macro_cluster_info.center_cs.z, 4 * _toFlatMacroClusterIndex(x, y, z));
            aabb_extent.load((float*)_macro_cluster_info.extent_cs.x, (float*)_macro_cluster_info.extent_cs.y, (float*)_macro_cluster_info.extent_cs.z, 4 * _toFlatMacroClusterIndex(x, y, z));

            int overlap_mask = _aabbOverlapsFrustumOptim(aabb_center, aabb_extent, light_clip_planes_xyz, light_clip_planes_w, num_light_clip_planes);

            if (overlap_mask != 0)
               _macro_clusters[_toFlatMacroClusterIndex(x, y, z)].lights[int(light_type)].emplace_back(LightCoverage(overlap_mask, light_index));
         }
      }
   }
}

void ClusteredLightCuller::_initDebugData()
{
   _debug_draw_cluster_grid = createProgramFromFile("debug_clustered_shading.glsl");
   _debug_draw = createProgramFromFile("debug_draw.glsl");
   _debug_cluster_grid = createBuffer(_light_clusters_dims.x * _light_clusters_dims.y * _light_clusters_dims.z * sizeof(vec3) * 24, GL_MAP_WRITE_BIT);
   _debug_enabled_clusters = createBuffer(_light_clusters_dims.x * _light_clusters_dims.y * _light_clusters_dims.z * sizeof(int), GL_MAP_WRITE_BIT);

   vec3* vertices = (vec3*)_debug_cluster_grid->map(GL_MAP_WRITE_BIT);

   vec3 scale;
   scale.x = 1.0f / float(_light_clusters_dims.x);
   scale.y = 1.0f / float(_light_clusters_dims.y);
   scale.z = 1.0f / float(_light_clusters_dims.z);
   for (int z = 0; z <= _light_clusters_dims.z - 1; z++)
   {
      for (int y = 0; y <= _light_clusters_dims.y - 1; y++)
      {
         for (int x = 0; x <= _light_clusters_dims.x - 1; x++)
         {
            vertices[0] = vec3(x, y, z) * scale;
            vertices[1] = vec3(x + 1, y, z) * scale;
            vertices[2] = vec3(x + 1, y, z) * scale;
            vertices[3] = vec3(x + 1, y + 1, z) * scale;
            vertices[4] = vec3(x + 1, y + 1, z) * scale;
            vertices[5] = vec3(x, y + 1, z) * scale;
            vertices[6] = vec3(x, y + 1, z) * scale;
            vertices[7] = vec3(x, y, z) * scale;
            vertices += 8;

            vertices[0] = vec3(x, y, z + 1) * scale;
            vertices[1] = vec3(x + 1, y, z + 1) * scale;
            vertices[2] = vec3(x + 1, y, z + 1) * scale;
            vertices[3] = vec3(x + 1, y + 1, z + 1) * scale;
            vertices[4] = vec3(x + 1, y + 1, z + 1) * scale;
            vertices[5] = vec3(x, y + 1, z + 1) * scale;
            vertices[6] = vec3(x, y + 1, z + 1) * scale;
            vertices[7] = vec3(x, y, z + 1) * scale;
            vertices += 8;

            vertices[0] = vec3(x, y, z) * scale;
            vertices[1] = vec3(x, y, z + 1) * scale;
            vertices[2] = vec3(x + 1, y, z) * scale;
            vertices[3] = vec3(x + 1, y, z + 1) * scale;
            vertices[4] = vec3(x + 1, y + 1, z) * scale;
            vertices[5] = vec3(x + 1, y + 1, z + 1) * scale;
            vertices[6] = vec3(x, y + 1, z) * scale;
            vertices[7] = vec3(x, y + 1, z + 1) * scale;
            vertices += 8;
         }
      }
   }
   _debug_cluster_grid->unmap();

   _debug_cluster_grid_vertex_source = std::make_unique<GLVertexSource>();
   _debug_cluster_grid_vertex_source->setVertexBuffer(*_debug_cluster_grid);
   _debug_cluster_grid_vertex_source->setPrimitiveType(GL_LINES);
   _debug_cluster_grid_vertex_source->setVertexCount(_light_clusters_dims.x * _light_clusters_dims.y * _light_clusters_dims.z * 24);
   _debug_cluster_grid_vertex_source->setVertexAttribute(0, 3, GL_FLOAT, GLSLVecType::vec);

   _debug_lines_buffer = createBuffer(sizeof(vec3) * 400, GL_MAP_WRITE_BIT);

   _debug_lines_source = std::make_unique<GLVertexSource>();
   _debug_lines_source->setVertexBuffer(*_debug_lines_buffer);
   _debug_lines_source->setPrimitiveType(GL_LINES);
   _debug_lines_source->setVertexCount(100);
   _debug_lines_source->setVertexAttribute(0, 3, GL_FLOAT, GLSLVecType::vec);
}


float ClusteredLightCuller::_convertClusterZtoCameraZ(float cluster_z, float znear, float zfar)
{
   float z = pow(cluster_z, _settings.cluster_z_distribution_factor);

   return -mix(znear, zfar, z);
}

float ClusteredLightCuller::_convertCameraZtoClusterZ(float z_in_camera_space, float znear, float zfar)
{
   float z = (z_in_camera_space - znear) / (zfar - znear);
   float cluster_z = pow(z, 1 / _settings.cluster_z_distribution_factor);

   return clamp(cluster_z, 0.0f, 1.0f);
}

Aabb3 ClusteredLightCuller::_computeConvexMeshClusterBounds(const RenderData& render_data, const mat4& matrix_light_proj_local, vec3* vertices_in_local, int num_vertices)
{
   float znear = render_data.frustum.near;
   float zfar = render_data.frustum.far;

   Aabb3 res;
   for (int i = 0; i < num_vertices; ++i)
   {
      vec4 vec_hs = matrix_light_proj_local * vec4(vertices_in_local[i], 1.0f);
      vec3 vertex_in_clip_space = vec_hs.xyz / vec3(vec_hs.w);
      if (vec_hs.w < 0.0f)
      {
         vertex_in_clip_space.x = -signOfFloat(vertex_in_clip_space.x);
         vertex_in_clip_space.y = -signOfFloat(vertex_in_clip_space.y);
      }

      float z_eye_space = 2.0f * znear * zfar / (znear + zfar - vertex_in_clip_space.z * (zfar - znear));
      float cluster_z = _convertCameraZtoClusterZ(z_eye_space, znear, zfar);

      res.extend(vec3(vertex_in_clip_space.xy, cluster_z));
   }

   res.pmin.xy = (res.pmin.xy + vec2(1.0f)) * 0.5f;
   res.pmax.xy = (res.pmax.xy + vec2(1.0f)) * 0.5f;

   return res;
}


Aabb3 ClusteredLightCuller::_computeSphereClusterBounds(const Scene& scene, const RenderData& render_data, const Light& light)
{
   vec3 sphere_center_in_vs = project(render_data.matrix_view_world, light.world_to_local_matrix[3]);
   float znear = render_data.frustum.near;
   float zfar = render_data.frustum.far;
   float znear_in_vs = -znear;

   float z_light_min = (-sphere_center_in_vs.z) - light.radius;
   float z_light_max = (-sphere_center_in_vs.z) + light.radius;

   vec3 left_right[2];
   vec3 bottom_top[2];
   sphereBoundsForAxis(vec3(1, 0, 0), sphere_center_in_vs, light.radius, znear_in_vs, left_right);
   sphereBoundsForAxis(vec3(0, 1, 0), sphere_center_in_vs, light.radius, znear_in_vs, bottom_top);

   const mat4& matrix_proj_view = render_data.matrix_proj_view;

   Aabb3 res;
   res.pmin = vec3(project(matrix_proj_view, left_right[0]).x, project(matrix_proj_view, bottom_top[0]).y, _convertCameraZtoClusterZ(z_light_min, znear, zfar));
   res.pmax = vec3(project(matrix_proj_view, left_right[1]).x, project(matrix_proj_view, bottom_top[1]).y, _convertCameraZtoClusterZ(z_light_max, znear, zfar));

   res.pmin.xy = (res.pmin.xy + vec2(1.0f)) * 0.5f;
   res.pmax.xy = (res.pmax.xy + vec2(1.0f)) * 0.5f;

   return res;
}

vec3 ClusteredLightCuller::_clusterCorner(const vec3 ndc_coords, const vec3& cluster_dims, const RenderData& render_data)
{
   float z = _convertClusterZtoCameraZ(ndc_coords.z / cluster_dims.z, render_data.frustum.near, render_data.frustum.far);

   float ratio = -z / render_data.frustum.near;
   float x = mix(render_data.frustum.left, render_data.frustum.right, ndc_coords.x / cluster_dims.x) * ratio;
   float y = mix(render_data.frustum.bottom, render_data.frustum.top, ndc_coords.y / cluster_dims.y) * ratio;

   return vec3(x, y, z);
}


void ClusteredLightCuller::_clusterCenterAndExtent(const RenderData& render_data, const ivec3& light_clusters_dims, int x, int y, int z, vec3* center, vec3* extent)
{
   vec3 aabb_min = project(render_data.matrix_proj_view, _clusterCorner(vec3(x, y, z), light_clusters_dims, render_data));
   vec3 aabb_max = project(render_data.matrix_proj_view, _clusterCorner(vec3(x + 1, y + 1, z + 1), light_clusters_dims, render_data));

   *center = 0.5f * (aabb_min + aabb_max);
   *extent = 0.5f * (aabb_max - aabb_min);
}

}