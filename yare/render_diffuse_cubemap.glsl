~~~~~~~~~~~~~~~~~~VertexShader ~~~~~~~~~~~~~~~~~~~~~~~~
#version 450

layout(location = 0) in vec2 position;

out vec2 uv;

void main()
{
   uv = position;
   gl_Position = vec4(position, 0.5, 1.0);
}

~~~~~~~~~~~~~~~~~FragmentShader ~~~~~~~~~~~~~~~~~~~~~~
#version 450
#include "glsl_cubemap_filtering_defines.h"
#include "common_cubemap_filtering.glsl"

layout(binding = BI_INPUT_CUBEMAP) uniform samplerCube input_cubemap;
layout(location = BI_FACE) uniform int face;
layout(location = BI_INPUT_CUBEMAP_LEVEL) uniform int cubemap_level;

in vec2 uv;
out vec4 out_face_color;


void main()
{
   vec3 normal_dir = normalize(faceToDirection(face, uv));

   int input_face_size = textureSize(input_cubemap, cubemap_level).x;
   vec3 sum = vec3(0.0);
   for (int input_face = 0; input_face < 6; ++input_face)
   {
      for (int y = 0; y < input_face_size; ++y)
      {
         for (int x = 0; x < input_face_size; ++x)
         {
            vec3 radiance_dir = normalize(faceToDirection(input_face, (vec2(x, y) + 0.5) / float(input_face_size)));
            vec3 radiance = textureLod(input_cubemap, radiance_dir, cubemap_level).rgb;
            sum += radiance * max(dot(normal_dir, radiance_dir), 0.0) * texelSolidAngle(x, y, input_face_size);
         }
      }
   }
   out_face_color.rgb = sum/PI; //irradiance to diffuse output radiance
}