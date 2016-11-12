#include "ClusteredLightCuller.h"

#include <glm/common.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <chrono>
#include <iostream>

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

namespace yare {

static int cMaxLightsPerCluster = 32;

ClusteredLightCuller::ClusteredLightCuller(const RenderResources& render_resources, const RenderSettings& settings)
: _rr(render_resources)
, _settings(settings)
{   
   _light_clusters_dims = ivec3(24, 24, 32);
   _light_clusters.resize(_light_clusters_dims.x * _light_clusters_dims.y * _light_clusters_dims.z);

   _light_list_head = createTexture3D(_light_clusters_dims.x, _light_clusters_dims.y, _light_clusters_dims.z, GL_RG32UI);
   _light_list_data = createDynamicBuffer(cMaxLightsPerCluster*sizeof(int) * _light_clusters_dims.x * _light_clusters_dims.y * _light_clusters_dims.z);
   //_light_list_data = createBuffer(cMaxLightsPerCluster * sizeof(int) * _light_clusters_dims.x * _light_clusters_dims.y * _light_clusters_dims.z, GL_MAP_WRITE_BIT);
   _initDebugData();
}

ClusteredLightCuller::~ClusteredLightCuller()
{

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
         
         vec2 rotated =(rotateTheta * projectedCenter);
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


static Aabb3 _computeLightClipSpaceBoundingBox(const Scene& scene, const RenderData& render_data, const Light& light)
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
   res.pmin = vec3(project(matrix_proj_view, left_right[0]).x, project(matrix_proj_view, bottom_top[0]).y, clamp((z_light_min - znear) / (zfar - znear), 0.0f, 1.0f));
   res.pmax = vec3(project(matrix_proj_view, left_right[1]).x, project(matrix_proj_view, bottom_top[1]).y, clamp((z_light_max - znear) / (zfar - znear), 0.0f, 1.0f));

   res.pmin.xy = (res.pmin.xy + vec2(1.0f)) * 0.5f;
   res.pmax.xy = (res.pmax.xy + vec2(1.0f)) * 0.5f;

   /*const_cast<vec3&>(render_data.points[0]) = left_right[0];
   const_cast<vec3&>(render_data.points[1]) = left_right[1];
   const_cast<vec3&>(render_data.points[2]) = bottom_top[0];
   const_cast<vec3&>(render_data.points[3]) = bottom_top[1];*/
   return res;
}

vec3 _clusterCorner(const vec3 ndc_coords, const vec3& cluster_dims, RenderData& render_data)
{
   float z = mix(-render_data.frustum.near, -render_data.frustum.far, ndc_coords.z / cluster_dims.z);

   float ratio = -z / render_data.frustum.near;
   float x = mix(render_data.frustum.left, render_data.frustum.right, ndc_coords.x / cluster_dims.x) * ratio;
   float y = mix(render_data.frustum.bottom, render_data.frustum.top, ndc_coords.y / cluster_dims.y) * ratio;

   return vec3(x, y, z);
}

bool _sphereOverlapsVoxel(const RenderSettings& settings, RenderData& render_data, float sphere_radius, const vec3& sphere_center, const ivec3& cluster_coords, const ivec3& cluster_dims, const Frustum& frustum)
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

   if (cluster_coords.x == settings.x && cluster_coords.y == settings.y && cluster_coords.z == settings.z)
   {
      render_data.points[0] = cluster_center;
      render_data.points[1] = _clusterCorner(vec3(cluster_coords), cluster_dims, render_data);
      render_data.points[2] = _clusterCorner(vec3(cluster_coords) + vec3(0, 1, 0), cluster_dims, render_data);
   }

   return intersection;
}


void ClusteredLightCuller::buildLightLists(const Scene& scene, RenderData& render_data)
{
   for (int z = 0; z <= _light_clusters_dims.z - 1; z++)
   {
      for (int y = 0; y <= _light_clusters_dims.y - 1; y++)
      {
         for (int x = 0; x <= _light_clusters_dims.x - 1; x++)
         {
            _light_clusters[_toFlatClusterIndex(x, y, z)].point_lights.resize(0);
         }
      }
   }
   auto start = std::chrono::steady_clock::now();
   short light_index = -1;
   
   {
      for (const auto& light : scene.lights)
      {
         light_index++;

         if (light.type == LightType::Sun)
            continue;

         Aabb3 clip_space_aabb = _computeLightClipSpaceBoundingBox(scene, _debug_render_data, light);
         vec3 sphere_center_in_vs = project(_debug_render_data.matrix_view_world, light.world_to_local_matrix[3]);

         intAabb3 voxels_overlapping_light;
         voxels_overlapping_light.pmin.x = (int)clamp(floor(clip_space_aabb.pmin.x * _light_clusters_dims.x), 0.0f, _light_clusters_dims.x - 1.0f);
         voxels_overlapping_light.pmin.y = (int)clamp(floor(clip_space_aabb.pmin.y * _light_clusters_dims.y), 0.0f, _light_clusters_dims.y - 1.0f);
         voxels_overlapping_light.pmin.z = (int)clamp(floor(clip_space_aabb.pmin.z * _light_clusters_dims.z), 0.0f, _light_clusters_dims.z - 1.0f);
         voxels_overlapping_light.pmax.x = (int)clamp(ceil(clip_space_aabb.pmax.x * _light_clusters_dims.x), 0.0f, _light_clusters_dims.x - 1.0f);
         voxels_overlapping_light.pmax.y = (int)clamp(ceil(clip_space_aabb.pmax.y * _light_clusters_dims.y), 0.0f, _light_clusters_dims.y - 1.0f);
         voxels_overlapping_light.pmax.z = (int)clamp(ceil(clip_space_aabb.pmax.z * _light_clusters_dims.z), 0.0f, _light_clusters_dims.z - 1.0f);

         for (int z = voxels_overlapping_light.pmin.z; z <= voxels_overlapping_light.pmax.z; z++)
         {
            for (int y = voxels_overlapping_light.pmin.y; y <= voxels_overlapping_light.pmax.y; y++)
            {
               for (int x = voxels_overlapping_light.pmin.x; x <= voxels_overlapping_light.pmax.x; x++)
               {
          
                  if (!_sphereOverlapsVoxel(_settings, _debug_render_data, light.radius, vec3(sphere_center_in_vs), ivec3(x, y, z), _light_clusters_dims, _debug_render_data.frustum))
                     continue;

                  /*vec3 cluster_center = _clusterCorner(vec3(x, y, z) + vec3(0.5f), _light_clusters_dims, render_data);
                  
                  float bounding_radius = 0.5*distance(_clusterCorner(vec3(x, y, z), _light_clusters_dims, render_data), _clusterCorner(vec3(x+1, y+1, z+1), _light_clusters_dims, render_data));
                  if (distance(sphere_center_in_vs, cluster_center) < (bounding_radius + light.radius))*/

                  _light_clusters[_toFlatClusterIndex(x, y, z)].point_lights.push_back(light_index);
               }
            }
         }

      }
   }

   float duration = std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count();
   std::cout << duration << std::endl;
   //_updateClustersGLData();
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
   line_data = _drawCross(project(render_data.matrix_proj_world*debug_matrix_world_view, _debug_render_data.points[0]), line_data);
   line_data = _drawCross(project(render_data.matrix_proj_world*debug_matrix_world_view, _debug_render_data.points[1]), line_data);
   line_data = _drawCross(project(render_data.matrix_proj_world*debug_matrix_world_view, _debug_render_data.points[2]), line_data);
   line_data = _drawCross(project(render_data.matrix_proj_world*debug_matrix_world_view, _debug_render_data.points[3]), line_data);
   _debug_lines_buffer->unmap();

   _debug_lines_source->setVertexCount(3*4);
   GLDevice::bindProgram(*_debug_draw);
   GLDevice::bindUniformMatrix4(0, mat4(1.0));
   GLDevice::draw(*_debug_lines_source);
   //glEnable(GL_DEPTH_TEST);
}

void ClusteredLightCuller::debugUpdateClusteredGrid(RenderData& render_data)
{
   _debug_render_data = render_data;
  

   _updateClustersGLData();

   int* enabled_clusters = (int*)_debug_enabled_clusters->map(GL_MAP_WRITE_BIT);
   for (int i = 0; i <_light_clusters.size(); ++i)
   {
      enabled_clusters[i] = !_light_clusters[i].point_lights.empty();
   }
   _debug_enabled_clusters->unmap();

   
}


int ClusteredLightCuller::_toFlatClusterIndex(int x, int y, int z)
{
   return x + y*_light_clusters_dims.x + z*(_light_clusters_dims.x*_light_clusters_dims.y);
}

struct ClusterListHead
{
   unsigned int list_start_offset;
   unsigned int point_light_count;
};

void ClusteredLightCuller::_updateClustersGLData()
{
   int cluster_count = _light_clusters_dims.x * _light_clusters_dims.y * _light_clusters_dims.z;
   
   auto gpu_lists_head = std::make_unique<ClusterListHead[]>(cluster_count);
   int* gpu_lists_data = (int*)_light_list_data->getUpdateSegmentPtr();

   unsigned int offset_into_data_buffer = 0;
   for (int i = 0; i < cluster_count; ++i)
   {
      const auto& cpu_cluster = _light_clusters[i];

      gpu_lists_head[i].list_start_offset = offset_into_data_buffer;
      gpu_lists_head[i].point_light_count = (unsigned int)cpu_cluster.point_lights.size();

      for (int j = 0; j < cpu_cluster.point_lights.size(); ++j)
      {
         gpu_lists_data[offset_into_data_buffer + j] = cpu_cluster.point_lights[j];
      }
      
      offset_into_data_buffer += (unsigned int)cpu_cluster.point_lights.size();
   }

   _light_list_head->update(gpu_lists_head.get());
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

   _debug_lines_buffer = createBuffer(sizeof(vec3) * 100, GL_MAP_WRITE_BIT);

   _debug_lines_source = std::make_unique<GLVertexSource>();
   _debug_lines_source->setVertexBuffer(*_debug_lines_buffer);
   _debug_lines_source->setPrimitiveType(GL_LINES);
   _debug_lines_source->setVertexCount(100);
   _debug_lines_source->setVertexAttribute(0, 3, GL_FLOAT, GLSLVecType::vec);
}

void ClusteredLightCuller::bindLightLists()
{
   glBindBufferRange(GL_SHADER_STORAGE_BUFFER, BI_LIGHT_LIST_DATA_SSBO, _light_list_data->id(),
                     _light_list_data->getRenderSegmentOffset(), _light_list_data->segmentSize());

   GLDevice::bindTexture(BI_LIGHT_LIST_HEAD, *_light_list_head, *_rr.samplers.mipmap_clampToEdge);
}

}