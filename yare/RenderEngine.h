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
   
private:
   void _bindSceneUniforms();
   void _bindSurfaceUniforms(int suface_index, const SurfaceInstance& surface);
   void _renderSurfaces(const RenderData& render_data);
   void _createSceneLightsBuffer();

   void _sortSurfacesByDistanceToCamera(RenderData& render_data);

private:
   DISALLOW_COPY_AND_ASSIGN(RenderEngine)
   Scene _scene;
   
   Uptr<GLDynamicBuffer> _surface_uniforms;
   Uptr<GLDynamicBuffer> _scene_uniforms;
   Uptr<GLBuffer> _lights_ssbo;

   Uptr<GLProgram> _z_pass_render_program;

   size_t _surface_uniforms_size;
};

}