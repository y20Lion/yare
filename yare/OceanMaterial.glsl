~~~~~~~~~~~~~~~~~~~VertexShader ~~~~~~~~~~~~~~~~~~~~~
#version 450
#include "glsl_binding_defines.h"
layout(location = 1) in vec3 position;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(std140, binding = 3) uniform MatUniform
{
mat4 matrix_view_local;
mat4 normal_matrix_world_local;
mat4x3 matrix_world_local;
};

void main()
{
   gl_Position = vec4(matrix_world_local*vec4(position, 1.0), 1.0);
}
~~~~~~~~~~~~~~~~~~~ TessControlShader ~~~~~~~~~~~~~~~~~~~~~
#version 450

layout(vertices = 3) out;

void main(void)
{
   gl_TessLevelOuter[0] = 32.0;
   gl_TessLevelOuter[1] = 32.0;
   gl_TessLevelOuter[2] = 32.0;
   //gl_TessLevelOuter[3] = 2.0;

   gl_TessLevelInner[0] = 32.0;
   //gl_TessLevelInner[1] = 2.0;

   gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}

~~~~~~~~~~~~~~~~~~~ TessEvaluationShader ~~~~~~~~~~~~~~~~~~~~~
#version 450
#include "glsl_binding_defines.h"
#define PI 3.1415926535897932384626433832795
layout(triangles, equal_spacing, ccw) in;

layout(std140, binding = 3) uniform MatUniform
{
mat4 matrix_view_local;
mat4 normal_matrix_world_local;
mat4x3 matrix_world_local;
};

layout(location = BI_TIME) uniform float time;

out vec3 attr_position3;
out vec3 attr_normal3;

vec3 interpolate(vec3 v0, vec3 v1, vec3 v2)
{
   return gl_TessCoord.x*v0 + gl_TessCoord.y*v1 + gl_TessCoord.z*v2;
}

vec4 interpolate(vec4 v0, vec4 v1, vec4 v2)
{
   return gl_TessCoord.x*v0 + gl_TessCoord.y*v1 + gl_TessCoord.z*v2;
}

vec3 evalOceanPosition(vec3 position)
{
   vec3 result = position;
   float lambda = 2.0;
   float speed = 0.3;
   result.z = 1.5*pow(sin(2 * PI / lambda*(position.x + speed*time))+1.0,2.0);
   result.z += 1.5*pow(sin(2 * PI / lambda*0.3*(position.y - speed*time))+1.0,2.0);
   return result;
}

void main()
{
   vec3 flat_position = interpolate(gl_in[0].gl_Position.xyz, gl_in[1].gl_Position.xyz, gl_in[2].gl_Position.xyz);
   
   vec3 pos = evalOceanPosition(flat_position);
   vec3 posX = evalOceanPosition(flat_position +vec3(0.001,0.0,0.0));
   vec3 posY = evalOceanPosition(flat_position + vec3(0.0, 0.0001, 0.0));
   attr_normal3 = normalize(cross(posX- pos, posY- pos));
   attr_position3 = pos;

   gl_Position = matrix_view_local * vec4(attr_position3, 1.0);
}

~~~~~~~~~~~~~~~~~~ FragmentShader ~~~~~~~~~~~~~~~~~~~~~~
#version 450
#include "glsl_binding_defines.h"

vec3 light_direction = vec3(0.0, 0.2, 1.0);
vec3 light_color = vec3(1.0, 1.0, 1.0);
layout(binding = BI_SKY_CUBEMAP) uniform samplerCube sky_cubemap;
layout(location = BI_EYE_POSITION) uniform vec3 eye_position;
layout(location = BI_TIME) uniform float time;

in vec3 attr_position3;
in vec3 attr_normal3;

#define PI 3.1415926535897932384626433832795

layout(location = 0, index = 0) out vec4 shading_result;

vec3 evalOceanPosition(vec3 position)
{
   vec3 result = position;
   float lambda = 2.0;
   float speed = 0.3;
   result.z = 1.5*pow(sin(2 * PI / lambda*(position.x + speed*time)) + 1.0, 2.0);
   result.z += 1.5*pow(sin(2 * PI / lambda*0.3*(position.x - speed*time)) + 1.0, 2.0);
   return result;
}


vec3 evalDiffuseBSDF(vec3 color, vec3 normal)
{
   return max(dot(normal, light_direction), 0.02) * color * light_color;
}

vec3 evalGlossyBSDF(vec3 color, vec3 normal)
{
   vec3 reflected_vector = normalize(reflect(attr_position3 - eye_position, normal));
   return texture(sky_cubemap, reflected_vector).rgb;
}

void sampleTexture(sampler2D tex, vec3 uvw, mat3x2 transform, out vec3 color, out float alpha)
{
   vec4 tex_sample = texture(tex, transform*vec3(uvw.xy, 1.0));
   color = tex_sample.rgb;
   alpha = tex_sample.a;
}

void main()
{
   vec3 normal = normalize(attr_normal3);
   shading_result =  vec4(evalGlossyBSDF(vec3(0.5), normal), 1.0);
}