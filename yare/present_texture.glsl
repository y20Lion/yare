~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~VertexShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#version 450
#include "glsl_global_defines.h"

layout(location = 0) in vec2 position;

void main()
{
   gl_Position = vec4(position, -1.0, 1.0);
}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~FragmentShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#version 450
#include "glsl_global_defines.h"
#define BI_INPUT_TEXTURE 0

layout(binding = BI_INPUT_TEXTURE) uniform sampler2D scene_texture;
//layout(origin_upper_left) in vec4 gl_FragCoord;
layout(location = 0) out vec4 out_color;

void main()
{
   vec4 col = texelFetch(scene_texture, ivec2(gl_FragCoord.xy), 0);
   out_color.rgb = col.w != 0.0? col.rgb/col.w : col.rgb;
}
