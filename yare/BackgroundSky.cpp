#include "BackgroundSky.h"

#include "BackgroundSky.h"
#include "GLProgram.h"
#include "RenderResources.h"
#include "GLDevice.h"

namespace yare {

BackgroundSky::BackgroundSky(const RenderResources& render_resources)
 : _render_resources(render_resources)
{
   _program = createProgramFromFile("BackgroundSky.glsl");
}

BackgroundSky::~BackgroundSky()
{

}

void BackgroundSky::render()
{
   glEnable(GL_DEPTH_TEST);
   glDepthFunc(GL_LEQUAL);   
   
   GLDevice::bindProgram(*_program);   
   GLDevice::draw(*_render_resources.fullscreen_triangle_source);
}

}