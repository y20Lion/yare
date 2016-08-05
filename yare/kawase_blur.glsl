~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ComputeShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include "glsl_film_postprocessing_defines.h"

layout(binding = BI_INPUT_TEXTURE) uniform sampler2D input_texture;
layout(binding = BI_OUTPUT_IMAGE, rgba32f) uniform writeonly image2D output_image;
layout(location = BI_KERNEL_SIZE) uniform int kernel_size;

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

void main()
{
   ivec2 dimensions = imageSize(output_image);
   vec2 uv = (vec2(gl_GlobalInvocationID.xy) + vec2(0.5)) / dimensions;   
   vec2 pixel_size = 1.0 / dimensions;

   vec2 dUV = (pixel_size * (float(kernel_size)+0.5));
   vec2 texcoord = vec2(0);

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

   imageStore(output_image, ivec2(gl_GlobalInvocationID.xy), vec4(color.rgb, 1.0));
}
