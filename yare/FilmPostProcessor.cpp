#include "FilmPostProcessor.h"

#include <iostream>

#include "RenderResources.h"
#include "GLFramebuffer.h"
#include "GLTexture.h"
#include "GLDevice.h"
#include "glsl_binding_defines.h"
#include "glsl_histogram_defines.h"
#include "GLProgram.h"
#include "GLBuffer.h"

namespace yare {

FilmPostProcessor::FilmPostProcessor(const RenderResources& render_resources)
 : _rr(render_resources)
{
   _luminance_histogram = createProgramFromFile("LuminanceHistogram.glsl");
   _luminance_histogram_reduce = createProgramFromFile("LuminanceHistogramReduce.glsl");
   _tone_mapping = createProgramFromFile("ToneMapping.glsl");
   _downscale_program = createProgramFromFile("Downscale.glsl");
   _kawase_blur_program = createProgramFromFile("KawaseBlur.glsl");
   _kawase_blur_render_program = createProgramFromFile("KawaseBlurRender.glsl");
   _extract_bloom_pixels_program = createProgramFromFile("ExtractBloomPixels.glsl");
   
   float initial_exposures[] = { 1.0f , 1.0f };
   _exposure_values_ssbo = createBuffer(2*sizeof(float), initial_exposures);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BI_EXPOSURE_VALUES_SSBO, _exposure_values_ssbo->id());

   int histogram_count = _rr.framebuffer_size.width/2*_rr.framebuffer_size.height/2 / (TILE_SIZE_X*TILE_SIZE_Y);
   _histograms_texture = createTexture2D(HISTOGRAM_SIZE, histogram_count, GL_R32F);
}

void FilmPostProcessor::developFilm()
{     
   GLDevice::bindDepthStencilState({ false, GL_LEQUAL });

   _downscaleSceneTexture();
   
   _computeLuminanceHistogram();
   _computeBloom();
   _presentFinalImage();

   GLDevice::bindDefaultDepthStencilState();

   /*float* histogram = new float[HISTOGRAM_SIZE * 768];
   glGetTextureImage(_histograms_texture->id(), 0, GL_RED, GL_FLOAT, HISTOGRAM_SIZE * 768*4, histogram);
   static int j = 0;
   for (int i = 0; i < HISTOGRAM_SIZE; ++i)
      std::cout << histogram[i] << " ";
   std::cout << std::endl;
   float average = 0;
   for (int i = 0; i < HISTOGRAM_SIZE; ++i)
   {
      average += histogram[i] *((float(i)+0.5) / HISTOGRAM_SIZE*float(6 + 8) + -8);
   }
   std::cout << average<<std::endl;*/
   /*float* values =  (float*)_exposure_values_ssbo->map(GL_READ_ONLY);
   std::cout << values[0] << std::endl;
   _exposure_values_ssbo->unmap();*/

   //delete[] histogram;
}

void FilmPostProcessor::_downscaleSceneTexture()
{
   const GLTexture2D& scene_texture = (GLTexture2D&)_rr.main_framebuffer->attachedTexture(GL_COLOR_ATTACHMENT0);
   auto& halfsize_texture0 = (GLTexture2D&)_rr.halfsize_postprocess_fbo->attachedTexture(GL_COLOR_ATTACHMENT0);
   glMemoryBarrier(GL_ALL_BARRIER_BITS);
   GLDevice::bindProgram(*_downscale_program);
   GLDevice::bindTexture(BI_INPUT_TEXTURE, scene_texture, *_rr.sampler_bilinear_clampToEdge);
   GLDevice::bindImage(BI_OUTPUT_IMAGE, halfsize_texture0, GL_WRITE_ONLY);
   glDispatchCompute(_rr.framebuffer_size.width / 2, _rr.framebuffer_size.height / 2, 1);
   glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

void FilmPostProcessor::_computeLuminanceHistogram()
{
   auto& halfsize_texture0 = (GLTexture2D&)_rr.halfsize_postprocess_fbo->attachedTexture(GL_COLOR_ATTACHMENT0);
   
   int num_group_x = _rr.framebuffer_size.width / 2 / (TILE_SIZE_X);
   int num_group_y = _rr.framebuffer_size.height / 2 / (TILE_SIZE_Y);

   GLDevice::bindImage(BI_INPUT_IMAGE, halfsize_texture0, GL_READ_ONLY);
   GLDevice::bindImage(BI_HISTOGRAMS_IMAGE, *_histograms_texture, GL_READ_WRITE);

   //_timer.start();

   GLDevice::bindProgram(*_luminance_histogram);
   glDispatchCompute(num_group_x, num_group_y, 1);
   glMemoryBarrier(GL_ALL_BARRIER_BITS);

   GLDevice::bindProgram(*_luminance_histogram_reduce);
   glDispatchCompute(HISTOGRAM_SIZE, 1, 1);
   glMemoryBarrier(GL_ALL_BARRIER_BITS);
   //_timer.stop();

   //std::cout << _timer.elapsedTimeInMs() << std::endl;
}

void FilmPostProcessor::_computeBloom()
{
#if 1
   auto& halfsize_texture0 = (GLTexture2D&)_rr.halfsize_postprocess_fbo->attachedTexture(GL_COLOR_ATTACHMENT0);
   auto& halfsize_texture1 = (GLTexture2D&)_rr.halfsize_postprocess_fbo->attachedTexture(GL_COLOR_ATTACHMENT1);
   _timer.start();

   glViewport(0, 0, halfsize_texture0.width(), halfsize_texture0.height());
   GLDevice::bindFramebuffer(_rr.halfsize_postprocess_fbo.get(), 1);

   GLDevice::bindProgram(*_extract_bloom_pixels_program);

   glUniform1f(BI_BLOOM_THRESHOLD, 10.0f);
   _rr.halfsize_postprocess_fbo->setDrawColorBuffer(1);
   GLDevice::bindTexture(BI_INPUT_TEXTURE, halfsize_texture0, *_rr.sampler_bilinear_clampToEdge);
   GLDevice::draw(*_rr.fullscreen_triangle_source);
   
   
   GLDevice::bindProgram(*_kawase_blur_render_program);  

   glUniform1i(BI_KERNEL_SIZE, 0);
   _rr.halfsize_postprocess_fbo->setDrawColorBuffer(0);
   GLDevice::bindTexture(BI_INPUT_TEXTURE, halfsize_texture1, *_rr.sampler_bilinear_clampToEdge);
   GLDevice::draw(*_rr.fullscreen_triangle_source);

   glUniform1i(BI_KERNEL_SIZE, 1);
   _rr.halfsize_postprocess_fbo->setDrawColorBuffer(1);
   GLDevice::bindTexture(BI_INPUT_TEXTURE, halfsize_texture0, *_rr.sampler_bilinear_clampToEdge);
   GLDevice::draw(*_rr.fullscreen_triangle_source);

   glUniform1i(BI_KERNEL_SIZE, 2);
   _rr.halfsize_postprocess_fbo->setDrawColorBuffer(0);
   GLDevice::bindTexture(BI_INPUT_TEXTURE, halfsize_texture1, *_rr.sampler_bilinear_clampToEdge);
   GLDevice::draw(*_rr.fullscreen_triangle_source);

   glUniform1i(BI_KERNEL_SIZE, 2);
   _rr.halfsize_postprocess_fbo->setDrawColorBuffer(1);
   GLDevice::bindTexture(BI_INPUT_TEXTURE, halfsize_texture0, *_rr.sampler_bilinear_clampToEdge);
   GLDevice::draw(*_rr.fullscreen_triangle_source);

   glUniform1i(BI_KERNEL_SIZE, 3);
   _rr.halfsize_postprocess_fbo->setDrawColorBuffer(0);
   GLDevice::bindTexture(BI_INPUT_TEXTURE, halfsize_texture1, *_rr.sampler_bilinear_clampToEdge);
   GLDevice::draw(*_rr.fullscreen_triangle_source);
   _timer.stop();

#else
   auto& halfsize_texture0 = (GLTexture2D&)_rr.halfsize_postprocess_fbo->attachedTexture(GL_COLOR_ATTACHMENT0);
   auto& halfsize_texture1 = (GLTexture2D&)_rr.halfsize_postprocess_fbo->attachedTexture(GL_COLOR_ATTACHMENT1);
   
   GLDevice::bindTexture(BI_INPUT_TEXTURE, halfsize_texture0, *_rr.sampler_bilinear_clp_to_edge);
   GLDevice::bindImage(BI_OUTPUT_IMAGE, halfsize_texture1, GL_WRITE_ONLY);
   
   int num_group_x = _rr.framebuffer_size.width / 2;
   int num_group_y = _rr.framebuffer_size.height / 2;
   _timer.start();
   GLDevice::bindProgram(*_kawase_blur_program);

   glUniform1i(BI_KERNEL_SIZE, 0);
   GLDevice::bindTexture(BI_INPUT_TEXTURE, halfsize_texture0, *_rr.sampler_bilinear_clp_to_edge);
   GLDevice::bindImage(BI_OUTPUT_IMAGE, halfsize_texture1, GL_WRITE_ONLY);
   glDispatchCompute(num_group_x, num_group_y, 1);
   glMemoryBa_rrier(GL_ALL_BA_rrIER_BITS);

   glUniform1i(BI_KERNEL_SIZE, 1);
   GLDevice::bindTexture(BI_INPUT_TEXTURE, halfsize_texture1, *_rr.sampler_bilinear_clp_to_edge);
   GLDevice::bindImage(BI_OUTPUT_IMAGE, halfsize_texture0, GL_WRITE_ONLY);
   glDispatchCompute(num_group_x, num_group_y, 1);
   glMemoryBa_rrier(GL_ALL_BA_rrIER_BITS);

   glUniform1i(BI_KERNEL_SIZE, 2);
   GLDevice::bindTexture(BI_INPUT_TEXTURE, halfsize_texture0, *_rr.sampler_bilinear_clp_to_edge);
   GLDevice::bindImage(BI_OUTPUT_IMAGE, halfsize_texture1, GL_WRITE_ONLY);
   glDispatchCompute(num_group_x, num_group_y, 1);
   glMemoryBa_rrier(GL_ALL_BA_rrIER_BITS);

   glUniform1i(BI_KERNEL_SIZE, 2);
   GLDevice::bindTexture(BI_INPUT_TEXTURE, halfsize_texture1, *_rr.sampler_bilinear_clp_to_edge);
   GLDevice::bindImage(BI_OUTPUT_IMAGE, halfsize_texture0, GL_WRITE_ONLY);
   glDispatchCompute(num_group_x, num_group_y, 1);
   glMemoryBa_rrier(GL_ALL_BA_rrIER_BITS);

   glUniform1i(BI_KERNEL_SIZE, 3);
   GLDevice::bindTexture(BI_INPUT_TEXTURE, halfsize_texture0, *_rr.sampler_bilinear_clp_to_edge);
   GLDevice::bindImage(BI_OUTPUT_IMAGE, halfsize_texture1, GL_WRITE_ONLY);
   glDispatchCompute(num_group_x, num_group_y, 1);
   glMemoryBa_rrier(GL_ALL_BA_rrIER_BITS);

    _timer.stop();

   
#endif
    //std::cout << _timer.elapsedTimeInMs() << std::endl;
}

void FilmPostProcessor::_presentFinalImage()
{
   GLDevice::bindFramebuffer(default_framebuffer, 0);   
   glViewport(0, 0, _rr.framebuffer_size.width, _rr.framebuffer_size.height);
   
   const GLTexture2D& scene_texture = (GLTexture2D&)_rr.main_framebuffer->attachedTexture(GL_COLOR_ATTACHMENT0);
   auto& halfsize_texture = (GLTexture2D&)_rr.halfsize_postprocess_fbo->attachedTexture(GL_COLOR_ATTACHMENT0);
   
   GLDevice::bindProgram(*_tone_mapping);
   glUniform1f(BI_BLOOM_THRESHOLD, 10.0f);
   GLDevice::bindTexture(BI_SCENE_TEXTURE, scene_texture, *_rr.sampler_bilinear_clampToEdge);
   GLDevice::bindTexture(BI_BLOOM_TEXTURE, halfsize_texture, *_rr.sampler_bilinear_clampToEdge);
   GLDevice::draw(*_rr.fullscreen_triangle_source);   
}

}