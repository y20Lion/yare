#pragma once

#include "tools.h"

namespace yare {

class GLProgram;
class GLTexture3D;
struct RenderResources;
struct RenderData;
using namespace glm;


class VolumetricFog
{
public:
   VolumetricFog(const RenderResources& _render_resources);
   ~VolumetricFog();

   void render(const RenderData& render_data);
   void bindFogVolume();
   void computeMemBarrier();

private:
   DISALLOW_COPY_AND_ASSIGN(VolumetricFog)
   Uptr<GLProgram> _fog_accumulate_program;
   Uptr<GLTexture3D> _fog_volume;
   ivec3 _fog_volume_size;
   const RenderResources& _rr;
};

}