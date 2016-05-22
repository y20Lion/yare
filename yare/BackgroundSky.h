#pragma once

#include "tools.h"
#include "IMaterial.h"

namespace yare {

class GLProgram;
struct RenderResources;

class BackgroundSky
{
public:
   BackgroundSky(const RenderResources& _render_resources);
   ~BackgroundSky();

   void render();

private:
   DISALLOW_COPY_AND_ASSIGN(BackgroundSky)
   Uptr<GLProgram> _program;   
   const RenderResources& _render_resources;
};

}