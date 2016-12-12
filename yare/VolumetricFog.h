#pragma once

#include "tools.h"

namespace yare {

class GLProgram;
class GLTexture3D;
struct RenderResources;
struct RenderData;
struct RenderSettings;
class Scene;
class GLBuffer;
using namespace glm;


class VolumetricFog
{
public:
   VolumetricFog(const RenderResources& _render_resources, const RenderSettings& settings);
   ~VolumetricFog();

   void render(const RenderData& render_data);
   void renderLightSprites(const RenderData& render_data, const Scene& scene);
   void bindFogVolume();
   void computeMemBarrier();

private:
   DISALLOW_COPY_AND_ASSIGN(VolumetricFog)
   Uptr<GLProgram> _fog_accumulate_program;
   Uptr<GLProgram> _fog_lighting_program; 
   Uptr<GLProgram> _draw_light_sprite;
   Uptr<GLTexture3D> _fog_volume;
   Uptr<GLBuffer> _debug_buffer;
   Uptr<GLTexture3D> _inscattering_extinction_volume;
   ivec3 _fog_volume_size;
   const RenderResources& _rr;
   const RenderSettings& _settings;
};

}