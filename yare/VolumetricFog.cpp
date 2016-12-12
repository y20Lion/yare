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
#include "RenderEngine.h"
#include "GLBuffer.h"

namespace yare {

VolumetricFog::VolumetricFog(const RenderResources& render_resources, const RenderSettings& settings)
   : _rr(render_resources)
   , _settings(settings)
{
   _fog_volume_size = ivec3(128, 64, 128);
   _fog_volume = createTexture3D(_fog_volume_size.x, _fog_volume_size.y, _fog_volume_size.z, GL_RGBA16F);
   _inscattering_extinction_volume = createTexture3D(_fog_volume_size.x, _fog_volume_size.y, _fog_volume_size.z, GL_RGBA16F);

   _debug_buffer = createBuffer(4*3, GL_MAP_READ_BIT);

   _fog_accumulate_program = createProgramFromFile("fog_accumulate.glsl");
   _fog_lighting_program = createProgramFromFile("fog_lighting.glsl");
   _draw_light_sprite = createProgramFromFile("draw_light_sprite.glsl");
}

VolumetricFog::~VolumetricFog(){}

void VolumetricFog::render(const RenderData& render_data)
{
   _rr.volumetric_fog_timer->start();

   GLDevice::bindProgram(*_fog_lighting_program);
   GLDevice::bindImage(BI_INSCATTERING_EXTINCTION_VOLUME_IMAGE, *_inscattering_extinction_volume, GL_WRITE_ONLY);
   GLDevice::bindUniformMatrix4(BI_MATRIX_WORLD_VIEW, inverse(render_data.matrix_view_world));
   glUniform4f(BI_FRUSTUM, render_data.frustum.left, render_data.frustum.right, render_data.frustum.bottom, render_data.frustum.top);

   vec3 scattering = _settings.fog_scattering * _settings.fog_scattering_color;
   glUniform4f(BI_SCATTERING_ABSORPTION, scattering.r, scattering.g, scattering.b, _settings.fog_absorption);
   glDispatchCompute(_fog_volume_size.x/16, _fog_volume_size.y/16, _fog_volume_size.z);
   
   glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

   vec3* value = (vec3*)_debug_buffer->map(GL_MAP_READ_BIT);
   int a = 0;
   _debug_buffer->unmap();

   GLDevice::bindProgram(*_fog_accumulate_program);
   GLDevice::bindImage(BI_FOG_VOLUME_IMAGE, *_fog_volume, GL_WRITE_ONLY);   
   GLDevice::bindTexture(BI_INSCATTERING_EXTINCTION_VOLUME, *_inscattering_extinction_volume, *_rr.samplers.linear_clampToEdge);
   GLDevice::bindUniformMatrix4(BI_MATRIX_WORLD_VIEW, inverse(render_data.matrix_view_world));
   glUniform4f(BI_FRUSTUM, render_data.frustum.left, render_data.frustum.right, render_data.frustum.bottom, render_data.frustum.top);
   
   glDispatchCompute(_fog_volume_size.x/16, _fog_volume_size.y/16, 1);

   _rr.volumetric_fog_timer->stop();
}

void VolumetricFog::renderLightSprites(const RenderData& render_data, const Scene& scene)
{
   GLDevice::bindVertexSource(*_rr.fullscreen_triangle_source);

   glDepthMask(GL_FALSE);
   glEnable(GL_PROGRAM_POINT_SIZE);
   GLDevice::bindProgram(*_draw_light_sprite);
   glUniform4f(BI_FRUSTUM, render_data.frustum.left, render_data.frustum.right, render_data.frustum.bottom, render_data.frustum.top);
   vec3 scattering = _settings.fog_scattering * _settings.fog_scattering_color;
   glUniform4f(BI_SCATTERING_ABSORPTION, scattering.r, scattering.g, scattering.b, _settings.fog_absorption);
      
   GLDevice::bindColorBlendState({ GLBlendingMode::Add });
   glDrawArrays(GL_POINTS, 0, (GLsizei)scene.sphere_lights.size());
   GLDevice::bindDefaultColorBlendState();
   glDepthMask(GL_TRUE);

   
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