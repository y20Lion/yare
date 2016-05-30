#pragma once

#include "tools.h"

namespace yare {

struct RenderResources;
class GLTextureCubemap;
class GLTexture2D;
class GLProgram;
class GLSampler;

class CubemapConverter
{
public:
   CubemapConverter(const RenderResources& render_resources);
   
   Uptr<GLTextureCubemap> createCubemapFromLatlong(const GLTexture2D& latlong_texture) const;
   Uptr<GLTextureCubemap> createDiffuseCubemap(const GLTextureCubemap& cubemap_texture) const;

private:
   DISALLOW_COPY_AND_ASSIGN(CubemapConverter)
   Uptr<GLProgram> _render_cubemap_face_prog;
   Uptr<GLProgram> _render_diffuse_cubemap_face_prog;
   Uptr<GLSampler> _latlong_sampler;   
   const RenderResources& _render_resources;
};

}