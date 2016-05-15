#pragma once

#include "tools.h"

#include <string>

namespace yare {

class GLBuffer;
class GLVertexSource;
class GLSampler;

struct RenderResources
{
   RenderResources();
   ~RenderResources();

   Uptr<GLSampler> sampler_cubemap;
   Uptr<GLBuffer> fullscreen_quad_vbo;
   Uptr<GLVertexSource> fullscreen_quad_source;
   std::string shade_tree_material_fragment;
   std::string shade_tree_material_vertex;
private:
   DISALLOW_COPY_AND_ASSIGN(RenderResources)
};

}