#pragma once

#include "tools.h"

#include <vector>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include "Scene.h"

namespace yare {

struct RenderResources;
class GLBuffer;
class GLTexture3D;
class GLProgram;
class GLVertexSource;
class Scene;
class GLDynamicBuffer;
class Aabb3;
struct RenderData;
struct RenderSettings;
struct simdvec3;
struct simdfloat;


using namespace glm;

struct LightCoverage
{
   LightCoverage() {}
   LightCoverage(unsigned short mask, unsigned short light_index) : mask(mask), light_index(light_index) {}

   unsigned short mask : 4;
   unsigned short light_index : 12;   
};

struct MacroCluster
{
   std::vector<LightCoverage> lights[3];
};

_declspec(align(16))
struct ClusterInfo
{
   vec3 corner_coord;
   int padding;
   vec3 center_coord;
   int padding2;
};

struct MacroClusterInfo
{   
   struct
   {
      vec4* x;
      vec4* y;
      vec4* z;
   } center_cs;

   struct
   {
      vec4* x;
      vec4* y;
      vec4* z;
   } extent_cs;
};

class ClusteredLightCuller
{
public:
   ClusteredLightCuller(const RenderResources& render_resources, const RenderSettings& settings);
   ~ClusteredLightCuller();

   void buildLightLists(const Scene& scene, RenderData& render_data);

   void drawClusterGrid(const RenderData& render_data, int index);
   void debugUpdateClusteredGrid(RenderData& render_data);

   ivec3 clustersDimensions() const { return _light_clusters_dims; }

   void bindLightLists();
   void updateLightListHeadTexture();

private:
   int _toFlatClusterIndex(int x, int y, int z);
   int _toFlatMacroClusterIndex(int x, int y, int z);
   void _updateClustersGLData();   
   int _sphereOverlapsVoxelOptim(int x, int y, int z, float sphere_radius, const vec3& sphere_center, const ClusterInfo* cluster_infos);

   void _injectSphereLightsIntoClusters(const Scene& scene, const RenderData& render_data);
   void _injectSpotLightsIntoClusters(const Scene& scene, const RenderData& render_data);
   void _injectRectangleLightsIntoClusters(const Scene& scene, const RenderData& render_data);   
   void _injectLightIntoClusters(const Aabb3& clip_space_aabb, unsigned short light_index, LightType light_type,
                                 const simdvec3* light_clip_planes_xyz, const simdfloat* light_clip_planes_w, int num_light_clip_planes);

   void _initDebugData();
   float _convertClusterZtoCameraZ(float cluster_z, float znear, float zfar);
   float _convertCameraZtoClusterZ(float z_in_camera_space, float znear, float zfar);
   Aabb3 _computeConvexMeshClusterBounds(const RenderData& render_data, const mat4& matrix_light_proj_local, vec3* vertices_in_local, int num_vertices);
   Aabb3 _computeSphereClusterBounds(const Scene& scene, const RenderData& render_data, const Light& light);
   vec3 _clusterCorner(const vec3 ndc_coords, const vec3& cluster_dims, const RenderData& render_data);
   void _clusterCenterAndExtent(const RenderData& render_data, const ivec3& light_clusters_dims, int x, int y, int z, vec3* center, vec3* extent);

public:
   RenderData _debug_render_data;

private:
   DISALLOW_COPY_AND_ASSIGN(ClusteredLightCuller)
   ivec3 _light_clusters_dims;
   
   std::vector<MacroCluster> _macro_clusters;
   std::vector<ClusterInfo> _cluster_info;
   MacroClusterInfo _macro_cluster_info;

   Uptr<GLTexture3D> _light_list_head;
   Uptr<GLDynamicBuffer> _light_list_head_pbo;
   Uptr<GLDynamicBuffer> _light_list_data;

   
   Uptr<GLProgram> _debug_draw_cluster_grid;
   
   Uptr<GLBuffer> _debug_cluster_grid;
   Uptr<GLBuffer> _debug_enabled_clusters;
   Uptr<GLVertexSource> _debug_cluster_grid_vertex_source;

   Uptr<GLProgram> _debug_draw;
   Uptr<GLBuffer> _debug_lines_buffer;
   Uptr<GLVertexSource> _debug_lines_source;
   
   const RenderResources& _rr;
   const RenderSettings& _settings;
};

}
