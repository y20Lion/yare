~~~~~~~~~~~~~~~~~~~ VertexShader ~~~~~~~~~~~~~~~~~~~~~
#version 450
#include "glsl_binding_defines.h"
%s
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
#ifdef USE_UV
layout(location=2) in vec2 uv;
#endif
#ifdef USE_NORMAL_MAPPING
layout(location = 3) in vec3 tangent;
#endif

out vec3 attr_position;
out vec3 attr_normal;
#ifdef USE_UV
out vec2 attr_uv;
#endif
#ifdef USE_NORMAL_MAPPING
out vec3 attr_tangent;
#endif

#include "surface_uniforms.glsl"

void main()
{
  gl_Position =  matrix_view_local * vec4(position, 1.0);
  attr_normal = mat3(normal_matrix_world_local)*normal;
  attr_position = matrix_world_local*vec4(position, 1.0);
#ifdef USE_UV
  attr_uv =  uv;
#endif
#ifdef USE_NORMAL_MAPPING
  attr_tangent = mat3(normal_matrix_world_local)*tangent;
#endif
}

~~~~~~~~~~~~~~~~~~ FragmentShader ~~~~~~~~~~~~~~~~~~~~~~
#version 450
#include "glsl_binding_defines.h"
%s
#include "scene_uniforms.glsl"

in vec3 attr_position;
in vec3 attr_normal;
#ifdef USE_UV
in vec2 attr_uv;
#endif
#ifdef USE_NORMAL_MAPPING
in vec3 attr_tangent;
#endif

layout(location = 0, index = 0) out vec4 shading_result;
layout(location = 0, index = 1) out vec4 shading_result_transp_factor;
%s

vec3 normal = normalize(attr_normal);
vec3 tangent = normalize(attr_tangent);

vec3 evalDiffuseBSDF(vec3 color, vec3 normal)
{
   return texture(sky_diffuse_cubemap, normal).rgb * color;
}

vec3 evalGlossyBSDF(vec3 color, vec3 normal)
{
   vec3 reflected_vector = normalize(reflect(attr_position-eye_position, normal));
   return texture(sky_cubemap, reflected_vector).rgb;
}

vec3 evalNormalMap(vec3 normal_color, float scale)
{
   //Tangent = normalize(Tangent - dot(Tangent, Normal) * Normal);
   vec3 bitangent = cross(tangent, normal);  
   vec3 normal_in_tangent_space = 2.0*normal_color - vec3(1.0);
   normal_in_tangent_space.x *= scale;
   normal_in_tangent_space.y *= scale;

   mat3 TBN = mat3(tangent, bitangent, normal);
   vec3 new_normal = TBN * normal_in_tangent_space;   
   return new_normal;
}

void sampleTexture(sampler2D tex, vec3 uvw, mat3x2 transform, out vec3 color, out float alpha)
{
	vec4 tex_sample = texture(tex, transform*vec3(uvw.xy,1.0));
	color=tex_sample.rgb;
	alpha=tex_sample.a;
}

 void main()
 {    
    %s
    //shading_result.rgb = vec3(attr_uv, 0.0);
 }