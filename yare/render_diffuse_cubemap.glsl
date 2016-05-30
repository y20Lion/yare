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
#include "glsl_cubemap_converter_defines.h"
#define PI 3.1415926535897932384626433832795

layout(binding = BI_INPUT_CUBEMAP) uniform samplerCube input_cubemap;
layout(location = BI_FACE) uniform int face;
layout(location = BI_INPUT_CUBEMAP_LEVEL) uniform int cubemap_level;

in vec2 uv;
out vec4 out_face_color;

vec3 faceToDirection(int face, vec2 uv)
{
   switch (face)
   {
   case 0: return vec3(1.0, -uv.y, -uv.x);
   case 1: return vec3(-1.0, -uv.y, uv.x);
   case 2: return vec3(uv.x, 1.0, uv.y);
   case 3: return vec3(uv.x, -1.0, -uv.y);
   case 4: return vec3(uv.x, -uv.y, 1.0);
   case 5: return vec3(-uv.x, -uv.y, -1.0);
   default: return vec3(0.0);
   }
}

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
            sum += radiance * max(dot(normal_dir, radiance_dir), 0.0);
         }
      }
   }
   out_face_color.rgb = sum / (input_face_size*input_face_size*6/2);
}