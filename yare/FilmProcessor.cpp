#include "FilmProcessor.h"

#include <iostream>

#include "RenderResources.h"
#include "GLFramebuffer.h"
#include "GLTexture.h"
#include "GLDevice.h"
#include "glsl_binding_defines.h"
#include "GLProgram.h"
#include "GLBuffer.h"

#define TILE_SIZE_X 32
#define TILE_SIZE_Y 32
#define HISTOGRAM_SIZE 8
namespace yare {

FilmProcessor::FilmProcessor(const RenderResources& render_resources)
 : _render_resources(render_resources)
{
   _luminance_histogram = createProgramFromFile("LuminanceHistogram.glsl");
   _luminance_histogram_reduce = createProgramFromFile("LuminanceHistogramReduce.glsl");
   _tone_mapping = createProgramFromFile("ToneMapping.glsl");
   
   float initial_exposures[] = { 1.0f , 1.0f };
   _exposure_values_ssbo = createBuffer(2*sizeof(float), initial_exposures);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BI_EXPOSURE_VALUES_SSBO, _exposure_values_ssbo->id());

   int histogram_count = _render_resources.framebuffer_size.width*_render_resources.framebuffer_size.height / (32 * 32);
   _histograms_texture = createTexture2D(HISTOGRAM_SIZE, histogram_count, GL_R32F);
}

void FilmProcessor::developFilm()
{
   static int new_exposure_index = 0;
   
   
   int num_group_x = _render_resources.framebuffer_size.width / 32;
   int num_group_y = _render_resources.framebuffer_size.height / 32;


   const GLTexture2D& texture = (GLTexture2D&)_render_resources.main_framebuffer->attachedTexture(GL_COLOR_ATTACHMENT0);
   GLDevice::bindImage(BI_INPUT_IMAGE, texture, GL_READ_ONLY);
   GLDevice::bindImage(BI_HISTOGRAMS_IMAGE, *_histograms_texture, GL_READ_WRITE);
   
   //_timer.start();

   GLDevice::bindProgram(*_luminance_histogram);     
   glDispatchCompute(num_group_x, num_group_y, 1);
   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
   //_timer.stop();
   GLDevice::bindProgram(*_luminance_histogram_reduce);
   glUniform1i(BI_NEW_EXPOSURE_INDEX, new_exposure_index);
   glDispatchCompute(HISTOGRAM_SIZE, 1, 1);
   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

   

   //std::cout << _timer.elapsedTimeInMs() << std::endl;

   /*float* histogram = new float[HISTOGRAM_SIZE * 768];
   glGetTextureImage(_histograms_texture->id(), 0, GL_RED, GL_FLOAT, HISTOGRAM_SIZE * 768*4, histogram);
   static int j = 0;*/
   /*for (int i = 0; i < HISTOGRAM_SIZE; ++i)
      std::cout << histogram[i] << " ";
   std::cout << std::endl;*/
   /*float average = 0;
   for (int i = 0; i < HISTOGRAM_SIZE; ++i)
   {
      average += histogram[i] *((float(i)+0.5) / HISTOGRAM_SIZE*float(6 + 8) + -8);
   }
   std::cout << average<<std::endl;*/
   /*float* values =  (float*)_exposure_values_ssbo->map(GL_READ_ONLY);
   std::cout << values[new_exposure_index] << std::endl;
   _exposure_values_ssbo->unmap();*/

   GLDevice::bindProgram(*_tone_mapping); 
   GLDevice::draw(*_render_resources.fullscreen_quad_source);

   //delete[] histogram;

   new_exposure_index = 1 - new_exposure_index;
}

}