#pragma once

#include "tools.h"
#include "Scene.h"
#include "GLProgram.h"

namespace yare {

class LatlongToCubemapConverter;
struct RenderResources;
class GLPersistentlyMappedBuffer;
class GLBuffer;
class BackgroundSky;

class RenderEngine
{
public:
   RenderEngine();
   ~RenderEngine();

   void offlinePrepareScene();
   void updateScene(RenderData& render_data);
   void renderScene(const RenderData& render_data);
   Scene* scene() { return &_scene; }

   glm::vec3 pickPosition(int x, int y);

   Uptr<RenderResources> render_resources;
   Uptr<LatlongToCubemapConverter> latlong_to_cubemap_converter;   
   Uptr<BackgroundSky> background_sky;
   
private:
   void _renderSurfaces(const RenderData& render_data);

private:
   DISALLOW_COPY_AND_ASSIGN(RenderEngine)
   Scene _scene;

   Uptr<GLPersistentlyMappedBuffer> _surface_constant_uniforms;
   Uptr<GLPersistentlyMappedBuffer> _surface_dynamic_uniforms;
   char* _surface_dynamic_uniforms_ptr;
   Uptr<GLPersistentlyMappedBuffer> _scene_uniforms;
   size_t _scene_uniforms_size;
   size_t _surface_dynamic_uniforms_size;
   size_t _surface_constant_uniforms_size;
};

}