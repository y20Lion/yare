#pragma once

#include "tools.h"

namespace yare {

struct RenderResources;
struct RenderData;
class GLProgram;
class GLTexture2D;

class SSAORenderer
{
public:
   SSAORenderer(const RenderResources& render_resources);

   void render(const RenderData& render_data);

private:
   DISALLOW_COPY_AND_ASSIGN(SSAORenderer)
   Uptr<GLProgram> _ssao_render;
   Uptr<GLProgram> _ssao_blur_horizontal;
   Uptr<GLProgram> _ssao_blur_vertical;
   const RenderResources& _rr;
};

}
