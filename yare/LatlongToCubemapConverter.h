#pragma once

#include "tools.h"

namespace yare {

struct RenderResources;
class GLTextureCubemap;
class GLTexture2D;
class GLProgram;
class GLSampler;

class LatlongToCubemapConverter
{
public:
   LatlongToCubemapConverter(const RenderResources& render_resources);
   
   Uptr<GLTextureCubemap> convert(const GLTexture2D& latlong_texture) const;
private:
   DISALLOW_COPY_AND_ASSIGN(LatlongToCubemapConverter)
   Uptr<GLProgram> _render_cubemap_face_prog;
   Uptr<GLSampler> _latlong_sampler;
   const RenderResources& _render_resources;
};

}