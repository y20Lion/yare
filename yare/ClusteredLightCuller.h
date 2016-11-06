#pragma once

#include "tools.h"

#include <vector>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

namespace yare {

struct RenderResources;
class GLBuffer;
class GLTexture3D;
class GLProgram;
class GLVertexSource;
class Scene;
class GLDynamicBuffer;
struct RenderData;
using namespace glm;

struct Cluster
{
   std::vector<short> point_lights;
};

class ClusteredLightCuller
{
public:
   ClusteredLightCuller(const RenderResources& render_resources);
   ~ClusteredLightCuller();

   void buildLightLists(const Scene& scene, RenderData& render_data);

   void drawClusterGrid(const RenderData& render_data);
   void debugUpdateClusteredGrid(RenderData& render_data);

   const GLTexture3D& lightListHead() const;
   const GLBuffer& lightListData() const;

   ivec3 clustersDimensions() const { return _light_clusters_dims; }

   void bindLightLists();

private:
   int _toFlatClusterIndex(int x, int y, int z);
   void _updateClustersGLData();
   void _initDebugData();

private:
   DISALLOW_COPY_AND_ASSIGN(ClusteredLightCuller)
   ivec3 _light_clusters_dims;
   std::vector<Cluster> _light_clusters;

   Uptr<GLTexture3D> _light_list_head;
   Uptr<GLDynamicBuffer> _light_list_data;
   Uptr<GLProgram> _debug_draw_cluster_grid;
   Uptr<GLProgram> _debug_draw;
   Uptr<GLBuffer> _debug_cluster_grid;
   Uptr<GLBuffer> _debug_enabled_clusters;
   Uptr<GLVertexSource> _debug_cluster_grid_vertex_source;

   Uptr<GLBuffer> _debug_lines_buffer;
   Uptr<GLVertexSource> _debug_lines_source;

   mat4 _debug_matrix_proj_world;
   const RenderResources& _rr;
};

}
