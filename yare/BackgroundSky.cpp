#include "BackgroundSky.h"

#include "BackgroundSky.h"
#include "GLProgram.h"
#include "RenderResources.h"
#include "Scene.h"
#include "GLDevice.h"

namespace yare {

BackgroundSky::BackgroundSky(const RenderResources& render_resources)
 : _render_resources(render_resources)
{
   _program = createProgramFromFile("background_sky.glsl");
}

BackgroundSky::~BackgroundSky()
{

}

void BackgroundSky::render(const RenderData& render_data)
{    
   GLDevice::bindProgram(*_program);   
   //GLDevice::bindUniformMatrix4(1, inverse(render_data.matrix_proj_world));
   GLDevice::draw(*_render_resources.fullscreen_triangle_source);
}

}