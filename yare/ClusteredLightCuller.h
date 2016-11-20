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
struct RenderData;
struct RenderSettings;


using namespace glm;

struct Cluster
{
   std::vector<short> point_lights;
   std::vector<short> spot_lights;
   std::vector<short> rectangle_lights;
};

_declspec(align(16))
struct ClusterInfo
{
   vec3 corner_coord;
   int padding;
   vec3 center_coord;
   int padding2;
   vec3 center_cs;
   int padding3;
   vec3 extent_cs;
   int padding4;
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
   void _updateClustersGLData();
   void _initDebugData();
   int _sphereOverlapsVoxelOptim(int x, int y, int z, float sphere_radius, const vec3& sphere_center, const ClusterInfo* cluster_infos);

public:
   RenderData _debug_render_data;

private:
   DISALLOW_COPY_AND_ASSIGN(ClusteredLightCuller)
   ivec3 _light_clusters_dims;
   
   std::vector<Cluster> _light_clusters;
   std::vector<ClusterInfo> _cluster_info;

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
