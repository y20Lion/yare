#include "SSAORenderer.h"

#include "RenderResources.h"
#include "glsl_ssao_defines.h"
#include "GLDevice.h"
#include "GLFramebuffer.h"
#include "GLSampler.h"
#include "GLProgram.h"
#include "GLGPUTimer.h"
#include "Scene.h"


namespace yare {

SSAORenderer::SSAORenderer(const RenderResources& render_resources)
: _rr(render_resources)
{
   _ssao_render = createProgramFromFile("ssao_render.glsl");
   _ssao_blur_horizontal = createProgramFromFile("ssao_blur.glsl", "BLUR_HORIZONTAL CHOCAPIC");
   _ssao_blur_vertical = createProgramFromFile("ssao_blur.glsl", "BLUR_VERTICAL");
}

void SSAORenderer::render(const RenderData& render_data)
{
   GLDevice::bindTexture(BI_DEPTHS_TEXTURE, _rr.main_framebuffer->attachedTexture(GL_DEPTH_ATTACHMENT), *_rr.samplers.nearest_clampToEdge);   
   //_rr.ssao_timer->start();
   GLDevice::bindFramebuffer(_rr.ssao_framebuffer.get(), 0);

   GLDevice::bindProgram(*_ssao_render);
   GLDevice::bindUniformMatrix4(BI_MATRIX_VIEW_PROJ, render_data.matrix_view_proj);
   GLDevice::bindUniformMatrix4(BI_MATRIX_VIEW_WORLD, render_data.matrix_view_world);   
   GLDevice::bindTexture(BI_NORMALS_TEXTURE, _rr.main_framebuffer->attachedTexture(GL_COLOR_ATTACHMENT1), *_rr.samplers.nearest_clampToEdge);
   GLDevice::bindTexture(BI_RANDOM_TEXTURE, *_rr.random_texture, *_rr.samplers.nearest_clampToEdge);
   GLDevice::draw(*_rr.fullscreen_triangle_source);

   
   GLDevice::bindFramebuffer(_rr.ssao_framebuffer.get(), 1);
   
   GLDevice::bindProgram(*_ssao_blur_horizontal);
   GLDevice::bindTexture(BI_SSAO_TEXTURE, _rr.ssao_framebuffer->attachedTexture(GL_COLOR_ATTACHMENT0), *_rr.samplers.nearest_clampToEdge);
   GLDevice::draw(*_rr.fullscreen_triangle_source);

   GLDevice::bindFramebuffer(_rr.ssao_framebuffer.get(), 0);

   GLDevice::bindProgram(*_ssao_blur_vertical);
   GLDevice::bindTexture(BI_SSAO_TEXTURE, _rr.ssao_framebuffer->attachedTexture(GL_COLOR_ATTACHMENT1), *_rr.samplers.nearest_clampToEdge);
   GLDevice::draw(*_rr.fullscreen_triangle_source);
   GLDevice::bindFramebuffer(nullptr, 0);
   //_rr.ssao_timer->stop();

   //_rr.ssao_timer->printElapsedTimeInMs();
}


}