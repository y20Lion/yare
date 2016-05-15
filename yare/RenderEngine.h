#pragma once

#include "tools.h"
#include "Scene.h"
#include "GLProgram.h"

namespace yare {

class LatlongToCubemapConverter;
struct RenderResources;

class RenderEngine
{
public:
   RenderEngine();
   ~RenderEngine();
   void update();
   void render();
   Scene* scene() { return &_scene; }

   glm::vec3 pickPosition(int x, int y);

   Uptr<RenderResources> render_resources;
   Uptr<LatlongToCubemapConverter> latlong_to_cubemap_converter;   
   
private:
   DISALLOW_COPY_AND_ASSIGN(RenderEngine)
   Scene _scene;
   Uptr<GLProgram> _draw_mesh;
   Uptr<GLBuffer> _uniforms_buffer;
};

}