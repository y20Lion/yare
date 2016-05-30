~~~~~~~~~~~~~~~~~~ VertexShader ~~~~~~~~~~~~~~~~~~~~~~~~
#version 450

layout(location=0) in vec2 position;

out vec2 uv;

void main()
{
   uv = position;
   gl_Position = vec4(position, 0.5, 1.0);
}

~~~~~~~~~~~~~~~~~ FragmentShader ~~~~~~~~~~~~~~~~~~~~~~
#version 450
#include "glsl_cubemap_converter_defines.h"
#define PI 3.1415926535897932384626433832795

layout(binding=BI_LATLONG_TEXTURE) uniform sampler2D latlong_texture;
layout(location=BI_FACE) uniform int face;

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
   vec3 dir = normalize(faceToDirection(face, uv));
   vec2 latlong_coord = vec2(atan(dir.y, dir.x), acos(dir.z)) / vec2(2.0*PI, PI);
   out_face_color = pow(texture(latlong_texture, vec2(latlong_coord.x, 1.0-latlong_coord.y)), vec4(1.3)); //todo yvain fixme
}