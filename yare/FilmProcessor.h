#pragma once

#include "tools.h"
#include "GLGPUTimer.h"

namespace yare {

struct RenderResources;
class GLTextureCubemap;
class GLTexture2D;
class GLProgram;
class GLSampler;
class GLTexture;
class GLBuffer;

class FilmProcessor
{
public:
   FilmProcessor(const RenderResources& render_resources);
   
   void developFilm();

private:
   DISALLOW_COPY_AND_ASSIGN(FilmProcessor)
   Uptr<GLProgram> _luminance_histogram;
   Uptr<GLProgram> _luminance_histogram_reduce;
   Uptr<GLProgram> _tone_mapping;
   Uptr<GLTexture2D> _histograms_texture;
   Uptr<GLBuffer> _exposure_values_ssbo;
   const RenderResources& _render_resources;
   GLGPUTimer _timer;
};

}