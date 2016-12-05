#include "VolumetricFog.h"

#include "GLTexture.h"
#include "GLDevice.h"
#include "GLProgram.h"
#include "GLGPUTimer.h"
#include "GLSampler.h"
#include "glsl_volumetric_fog_defines.h"
#include "glsl_global_defines.h"
#include "stl_helpers.h"
#include "RenderResources.h"
#include "Scene.h"

namespace yare {

VolumetricFog::VolumetricFog(const RenderResources& render_resources)
   : _rr(render_resources)
{
   _fog_volume_size = ivec3(120, 80, 64);
   _fog_volume = createTexture3D(_fog_volume_size.x, _fog_volume_size.y, _fog_volume_size.z, GL_RGBA16F);
   _inscattering_extinction_volume = createTexture3D(_fog_volume_size.x, _fog_volume_size.y, _fog_volume_size.z, GL_RGBA16F);

   _fog_accumulate_program = createProgramFromFile("fog_accumulate.glsl");
   _fog_lighting_program = createProgramFromFile("fog_lighting.glsl");
}

VolumetricFog::~VolumetricFog(){}

void VolumetricFog::render(const RenderData& render_data)
{
   _rr.volumetric_fog_timer->start();

   GLDevice::bindProgram(*_fog_lighting_program);
   GLDevice::bindImage(BI_INSCATTERING_EXTINCTION_VOLUME_IMAGE, *_inscattering_extinction_volume, GL_WRITE_ONLY);
   GLDevice::bindUniformMatrix4(BI_MATRIX_WORLD_VIEW, inverse(render_data.matrix_view_world));
   glUniform4f(BI_FRUSTUM, render_data.frustum.left, render_data.frustum.right, render_data.frustum.bottom, render_data.frustum.top);
   glDispatchCompute(_fog_volume_size.x, _fog_volume_size.y, _fog_volume_size.z);
   
   glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

   GLDevice::bindProgram(*_fog_accumulate_program);
   GLDevice::bindImage(BI_FOG_VOLUME_IMAGE, *_fog_volume, GL_WRITE_ONLY);   
   GLDevice::bindTexture(BI_INSCATTERING_EXTINCTION_VOLUME, *_inscattering_extinction_volume, *_rr.samplers.linear_clampToEdge);
   GLDevice::bindUniformMatrix4(BI_MATRIX_WORLD_VIEW, inverse(render_data.matrix_view_world));
   glUniform4f(BI_FRUSTUM, render_data.frustum.left, render_data.frustum.right, render_data.frustum.bottom, render_data.frustum.top);
   glDispatchCompute(_fog_volume_size.x, _fog_volume_size.y, 1);

   _rr.volumetric_fog_timer->stop();
   /*float clear_val[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
   glClearTexImage(_fog_volume->id(), 0, GL_RGBA, GL_FLOAT, clear_val);*/
}

void VolumetricFog::bindFogVolume()
{
   GLDevice::bindTexture(BI_FOG_VOLUME, *_fog_volume, *_rr.samplers.linear_clampToEdge);
}

void VolumetricFog::computeMemBarrier()
{
   glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
}

}