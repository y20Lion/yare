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

class FilmPostProcessor
{
public:
   FilmPostProcessor(const RenderResources& render_resources);
   
   void developFilm();

private:
   void _downscaleSceneTexture();
   void _computeLuminanceHistogram();
   void _computeBloom();
   void _presentFinalImage();

private:
   DISALLOW_COPY_AND_ASSIGN(FilmPostProcessor)
   Uptr<GLProgram> _luminance_histogram;
   Uptr<GLProgram> _luminance_histogram_reduce;
   Uptr<GLProgram> _tone_mapping;
   Uptr<GLProgram> _downscale_program;
   Uptr<GLProgram> _kawase_blur_program;
   Uptr<GLProgram> _kawase_blur_render_program;
   Uptr<GLProgram> _extract_bloom_pixels_program;
   Uptr<GLTexture2D> _histograms_texture;
   Uptr<GLBuffer> _exposure_values_ssbo;
   const RenderResources& _rr; // Bad naming but lisibility win for vars referencing
   GLGPUTimer _timer;
};

}