#pragma once

#include "tools.h"
#include "Scene.h"
#include "GLProgram.h"

namespace yare {

class CubemapFiltering;
struct RenderResources;
class GLDynamicBuffer;
class GLBuffer;
class BackgroundSky;
struct ImageSize;
class FilmPostProcessor;
class SSAORenderer;
class ClusteredLightCuller;
class VolumetricFog;

struct RenderSettings
{
   float froxel_z_distribution_factor = 2.0f;
   float light_contribution_threshold = 0.05f;
   float bias = 0.0f;
   int x = 16;
   int y = 16;
   int z = 16;
};


class RenderEngine
{
public:
   RenderEngine(const ImageSize& framebuffer_size);
   ~RenderEngine();

   void offlinePrepareScene();
   void updateScene(RenderData& render_data);
   void renderScene(const RenderData& render_data);
   void presentDebugTexture();
   Scene* scene() { return &_scene; }

   Uptr<RenderResources> render_resources;
   Uptr<CubemapFiltering> cubemap_converter;
   Uptr<BackgroundSky> background_sky;
   Uptr<FilmPostProcessor> film_processor;
   Uptr<SSAORenderer> ssao_renderer;
   Uptr<ClusteredLightCuller> froxeled_light_culler;
   Uptr<VolumetricFog> volumetric_fog;

   RenderSettings _settings;
   
private:
   void _bindSceneUniforms();
   void _bindSurfaceUniforms(int suface_index, const SurfaceInstance& surface);
   void _renderSurfaces(const RenderData& render_data);
   void _renderSurfacesMaterial(SurfaceRange surfaces);
   void _createSceneLightsBuffer();

   void _sortSurfacesByDistanceToCamera(RenderData& render_data);
   void _sortSurfacesByMaterial();
   void _updateUniformBuffers(const RenderData& render_data, float time, float delta_time);
   void _updateRenderMatrices(RenderData& render_data);
   void _computeLightsRadius();

private:
   DISALLOW_COPY_AND_ASSIGN(RenderEngine)
   Scene _scene;
   
   Uptr<GLDynamicBuffer> _surface_uniforms;
   Uptr<GLDynamicBuffer> _scene_uniforms;

   Uptr<GLBuffer> _sphere_lights_ssbo;
   Uptr<GLBuffer> _spot_lights_ssbo;
   Uptr<GLBuffer> _rectangle_lights_ssbo;
   Uptr<GLBuffer> _sun_lights_ssbo;

   Uptr<GLProgram> _z_pass_render_program;

   size_t _surface_uniforms_size;

   
};

}