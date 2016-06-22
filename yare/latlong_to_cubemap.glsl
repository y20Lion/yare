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
#include "glsl_cubemap_filtering_defines.h"
#include "common_cubemap_filtering.glsl"

layout(binding=BI_LATLONG_TEXTURE) uniform sampler2D latlong_texture;
layout(location=BI_FACE) uniform int face;

in vec2 uv;
out vec4 out_face_color;

void main()
{
   vec3 dir = normalize(faceToDirection(face, uv));
   vec2 latlong_coord = vec2(atan(dir.y, dir.x), acos(dir.z)) / vec2(2.0*PI, PI);
   out_face_color.rgb = min(texture(latlong_texture, vec2(0.5 - latlong_coord.x, latlong_coord.y)).rgb, vec3(100.0));
}