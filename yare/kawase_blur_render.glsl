~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~VertexShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#version 450

layout(location = 0) in vec2 position;

void main()
{
   gl_Position = vec4(position, -1.0, 1.0);
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~FragmentShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#version 450
#include "glsl_film_postprocessing_defines.h"

layout(binding = BI_INPUT_TEXTURE) uniform sampler2D input_texture;
layout(location = BI_KERNEL_SIZE) uniform int kernel_size;

layout(location = 0) out vec4 out_color;

void main()
{
   ivec2 dimensions = textureSize(input_texture, 0);
   vec2 uv = gl_FragCoord.xy / dimensions;
   vec2 pixel_size = 1.0 / dimensions;

   vec2 dUV = (pixel_size * (float(kernel_size) + 0.5));
   vec2 texcoord = vec2(0.0);

   texcoord.x = uv.x - dUV.x;
   texcoord.y = uv.y + dUV.y;
   vec4 color = texture(input_texture, texcoord, 0);

   texcoord.x = uv.x + dUV.x;
   texcoord.y = uv.y + dUV.y;
   color += texture(input_texture, texcoord, 0);

   texcoord.x = uv.x - dUV.x;
   texcoord.y = uv.y - dUV.y;
   color += texture(input_texture, texcoord, 0);

   texcoord.x = uv.x + dUV.x;
   texcoord.y = uv.y - dUV.y;
   color += texture(input_texture, texcoord, 0);

   color *= 0.25;

   out_color = vec4(color.rgb, 1.0);
}
