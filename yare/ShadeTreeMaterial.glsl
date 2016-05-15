~~~~~~~~~~~~~~~~~~~ VertexShader ~~~~~~~~~~~~~~~~~~~~~
#version 450
%s
layout(location=1) in vec3 position;
layout(location=2) in vec3 normal;
#ifdef USE_UV
layout(location=3) in vec2 uv;
#endif

layout(location = 12) uniform mat4x3 mat;

out vec3 attr_position;
out vec3 attr_normal;
#ifdef USE_UV
out vec2 attr_uv;
#endif

layout(std140, binding=3) uniform MatUniform 
{
  mat4 matrix_view_local;
  mat4 normal_matrix_world_local;
  mat4x3 matrix_world_local;
};

void main()
{
  gl_Position =  matrix_view_local * vec4(position, 1.0);
  attr_normal =  mat3(normal_matrix_world_local)*normal;
  attr_position = matrix_world_local*vec4(position, 1.0);
#ifdef USE_UV
  attr_uv =  uv;
#endif
}

~~~~~~~~~~~~~~~~~~ FragmentShader ~~~~~~~~~~~~~~~~~~~~~~
#version 450
#include "glsl_binding_defines.h"
%s
vec3 light_direction = vec3(0.0, 0.2, 1.0);
vec3 light_color = vec3(1.0, 1.0, 1.0);
layout(binding = BI_SKY_CUBEMAP) uniform samplerCube sky_cubemap;
layout(location = BI_EYE_POSITION) uniform vec3 eye_position;


in vec3 attr_position;
in vec3 attr_normal;
#ifdef USE_UV
in vec2 attr_uv;
#endif

layout(location = 0, index = 0) out vec4 shading_result;
layout(location = 0, index = 1) out vec4 shading_result_transp_factor;
%s

vec3 evalDiffuseBSDF(vec3 color, vec3 normal)
{
	return max(dot(normal, light_direction),0.02) * color * light_color;
}

vec3 evalGlossyBSDF(vec3 color, vec3 normal)
{
   vec3 reflected_vector = normalize(reflect(attr_position-eye_position, normal));
   return texture(sky_cubemap, reflected_vector).rgb;
}

void sampleTexture(sampler2D tex, vec3 uvw, mat3x2 transform, out vec3 color, out float alpha)
{
	vec4 tex_sample=texture(tex, transform*vec3(uvw.xy,1.0));
	color=tex_sample.rgb;
	alpha=tex_sample.a;
}

 void main()
 {
    vec3 normal = normalize(attr_normal);
    %s
 }