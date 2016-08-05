~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ComputeShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include "glsl_film_postprocessing_defines.h"

layout(binding = BI_INPUT_TEXTURE) uniform sampler2D input_texture;
layout(binding = BI_OUTPUT_IMAGE, rgba32f) uniform writeonly image2D half_size_image;

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

void main()
{
   ivec2 dimensions = imageSize(half_size_image);

   vec2 coord = (vec2(gl_GlobalInvocationID.xy) + vec2(0.5))/ dimensions; // in the middle of a 4 texels block
   vec3 color = texture(input_texture, coord, 0).rgb;
   imageStore(half_size_image, ivec2(gl_GlobalInvocationID.xy), vec4(color, 1.0));
}
