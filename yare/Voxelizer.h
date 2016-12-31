#pragma once

#include "tools.h"

#include "Aabb3.h"

namespace yare {

class Scene;
struct RenderResources;
class GLFramebuffer;
class GLProgram;
class RenderEngine;
struct RenderData;
class GLTexture3D;
class GLTexture1D;
class GLVertexSource;
class GLBuffer;
class GLFramebuffer;
struct RenderSettings;

class Voxelizer
{
public:
   Voxelizer(const RenderResources& render_resources, const RenderSettings& render_settings);
   ~Voxelizer();

   void bakeVoxels(RenderEngine* render_engine, const RenderData& render_data);
   void debugDrawVoxels(const RenderData& render_data);

   void shadeVoxels(const RenderData& render_data);
   void traceGlobalIlluminationRays(const RenderData& render_data);
   void gatherGlobalIllumination();
   void bindGlobalIlluminationTexture();

private:
   Uptr<GLFramebuffer> _empty_framebuffer;
   Uptr<GLProgram> _rasterize_voxels;
   Uptr<GLProgram> _fill_occlusion_voxels;
   Uptr<GLProgram> _shade_voxels;   
   Uptr<GLTexture3D> _voxels_gbuffer;
   Uptr<GLTexture3D> _voxels_occlusion;
   Uptr<GLTexture3D> _voxels_illumination;
   Uptr<GLTexture1D> _hemisphere_samples;
   int _texture_size;
   Aabb3 _voxels_aabb;

   Uptr<GLVertexSource> _debug_voxel_wireframe_vertex_source;
   Uptr<GLProgram> _debug_draw_voxels;
   Uptr<GLBuffer> _debug_voxels_wireframe;

   Uptr<GLProgram> _trace_gi_rays;
   Uptr<GLProgram> _gather_gi_horizontal;
   Uptr<GLProgram> _gather_gi_vertical;
   Uptr<GLFramebuffer> _gi_framebuffer;

   const RenderResources& _rr;
   const RenderSettings& _settings;
};

}