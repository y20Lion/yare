#include "ClusteredLightCuller.h"

#include <glm/common.hpp>

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

namespace yare {

static int cMaxLightsPerCluster = 32;

ClusteredLightCuller::ClusteredLightCuller(const RenderResources& render_resources)
: _rr(render_resources)
{   
   _light_clusters_dims = ivec3(32, 32, 10);
   _light_clusters.resize(_light_clusters_dims.x * _light_clusters_dims.y * _light_clusters_dims.z);

   _light_list_head = createTexture3D(_light_clusters_dims.x, _light_clusters_dims.y, _light_clusters_dims.z, GL_RG32UI);
   _light_list_data = createDynamicBuffer(cMaxLightsPerCluster*sizeof(int) * _light_clusters_dims.x * _light_clusters_dims.y * _light_clusters_dims.z);
   
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
   vec2 c(dot(axis, sphere_center), sphere_center.z);

   float tSquared = dot(c, c) - sphere_radius*sphere_radius;
   bool eye_inside_of_sphere = (tSquared <= 0);

   vec2 v = eye_inside_of_sphere ?   vec2(0.0f) : vec2(sqrt(tSquared), sphere_radius) / vec2(length(c));
   // Does the near plane intersect the sphere?
   const bool sphere_clipping_with_znear  = (c.y + sphere_radius >= znear);
   // Square root of the discriminant; NaN (and unused)
   // if the camera is in the sphere
   float k = sqrt(sphere_radius*sphere_radius - SQUARE(znear - c.y));
   
   vec2 bounds[2];
   for (int i = 0; i < 2; ++i)
   {
      if (!eye_inside_of_sphere)
         bounds[i] = v.x* mat2(v.x, -v.y, v.y, v.x) * c;

      const bool clip_bound = eye_inside_of_sphere || (bounds[i].y > znear);
      /*if (sphere_clipping_with_znear && clip_bound)
         bounds[i] = vec2(projCenter.x + k, znear);*/

      // Set up for the lower bound
      v.y = -v.y; 
      k = -k;
   }

   // Transform back to camera space
   bounds_cs[1] = bounds[1].x* axis;
   bounds_cs[1].z = bounds[1].y;

   bounds_cs[0] = bounds[0].x* axis;
   bounds_cs[0].z = bounds[0].y;
}


static Aabb3 _computeLightClipSpaceBoundingBox(const Scene& scene, const RenderData& render_data, const Light& light)
{
   vec3 sphere_center_in_vs = project(render_data.matrix_view_world, light.world_to_local_matrix[3]);
   float znear_in_vs = -scene.camera.frustum.near;

   vec3 left_right[2];
   vec3 bottom_top[2];
   sphereBoundsForAxis(vec3(1, 0, 0), sphere_center_in_vs, light.radius, znear_in_vs, left_right);
   sphereBoundsForAxis(vec3(0, 1, 0), sphere_center_in_vs, light.radius, znear_in_vs, bottom_top);

   const mat4& matrix_proj_view = render_data.matrix_proj_view;

   Aabb3 res;
   res.pmin = vec3(project(matrix_proj_view, left_right[0]).x, project(matrix_proj_view, bottom_top[0]).y, 0.0f);
   res.pmax = vec3(project(matrix_proj_view, left_right[1]).x, project(matrix_proj_view, bottom_top[1]).y, 1.0f);
   
   res.pmin.xy = (res.pmin.xy + vec2(1.0f)) * 0.5f;
   res.pmax.xy = (res.pmax.xy + vec2(1.0f)) * 0.5f;

   return res;
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
   
   short light_index = -1;
   for (const auto& light : scene.lights)
   {
      light_index++;

      if (light.type == LightType::Sun)
         continue;

      Aabb3 clip_space_aabb = _computeLightClipSpaceBoundingBox(scene, render_data, light);

      intAabb3 voxels_overlapping_light;
      voxels_overlapping_light.pmin.x = clamp(int(clip_space_aabb.pmin.x * _light_clusters_dims.x), 0, _light_clusters_dims.x - 1);
      voxels_overlapping_light.pmin.y = clamp(int(clip_space_aabb.pmin.y * _light_clusters_dims.y), 0, _light_clusters_dims.y - 1);
      voxels_overlapping_light.pmin.z = clamp(int(clip_space_aabb.pmin.z * _light_clusters_dims.z), 0, _light_clusters_dims.z - 1);
      voxels_overlapping_light.pmax.x = clamp(int(clip_space_aabb.pmax.x * _light_clusters_dims.x), 0, _light_clusters_dims.x - 1);
      voxels_overlapping_light.pmax.y = clamp(int(clip_space_aabb.pmax.y * _light_clusters_dims.y), 0, _light_clusters_dims.y - 1);
      voxels_overlapping_light.pmax.z = clamp(int(clip_space_aabb.pmax.z * _light_clusters_dims.z), 0, _light_clusters_dims.z - 1);

      for (int z = voxels_overlapping_light.pmin.z; z <= voxels_overlapping_light.pmax.z; z++)
      {
         for (int y = voxels_overlapping_light.pmin.x; y <= voxels_overlapping_light.pmax.y ; y++)
         {
            for (int x = voxels_overlapping_light.pmin.x; x <= voxels_overlapping_light.pmax.x; x++)
            {
               _light_clusters[_toFlatClusterIndex(x, y, z)].point_lights.push_back(light_index);
            }
         }
      }
     
   }

   _updateClustersGLData();
}

void ClusteredLightCuller::drawClusterGrid()
{
   GLDevice::bindProgram(*_debug_draw_cluster_grid);
   GLDevice::bindUniformMatrix4(0, glm::inverse(_debug_matrix_proj_world));
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _debug_enabled_clusters->id());
   GLDevice::draw(*_debug_cluster_grid_vertex_source);
}

void ClusteredLightCuller::debugUpdateClusteredGrid(RenderData& render_data)
{
   _debug_matrix_proj_world = render_data.matrix_proj_world;
   int* enabled_clusters = (int*)_debug_enabled_clusters->map(GL_MAP_WRITE_BIT);
   for (int i = 0; i <_light_clusters.size(); ++i)
   {
      enabled_clusters[i] = !_light_clusters[i].point_lights.empty();
   }
   _debug_enabled_clusters->unmap();
}

const GLTexture3D& ClusteredLightCuller::lightListHead() const
{ 
   return *_light_list_head; 
}
const GLBuffer& ClusteredLightCuller::lightListData() const
{
   return *_light_list_data; 
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
   _light_list_data->unmap();
}

void ClusteredLightCuller::_initDebugData()
{
   _debug_draw_cluster_grid = createProgramFromFile("debug_clustered_shading.glsl");
   _debug_cluster_grid = createBuffer(_light_clusters_dims.x * _light_clusters_dims.y * _light_clusters_dims.z * sizeof(vec3) * 24, GL_MAP_WRITE_BIT);
   _debug_enabled_clusters = createBuffer(_light_clusters_dims.x * _light_clusters_dims.y * _light_clusters_dims.z * sizeof(int), GL_MAP_WRITE_BIT);

   vec3* vertices = (vec3*)_debug_cluster_grid->map(GL_MAP_WRITE_BIT);

   _debug_matrix_proj_world = mat4(1);
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
}

void ClusteredLightCuller::bindLightLists()
{
   glBindBufferRange(GL_SHADER_STORAGE_BUFFER, BI_LIGHT_LIST_DATA_SSBO, _light_list_data->id(),
                     _light_list_data->getRenderSegmentOffset(), _light_list_data->segmentSize());

   GLDevice::bindTexture(BI_LIGHT_LIST_HEAD, *_light_list_head, *_rr.samplers.mipmap_clampToEdge);
}

}