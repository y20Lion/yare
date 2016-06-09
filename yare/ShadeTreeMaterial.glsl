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
#ifdef USE_NORMAL_MAPPING
vec3 tangent = normalize(attr_tangent);
#endif

float vec3ToFloat(vec3 v)
{
   return (v.x + v.y + v.z) / 3.0;
}

vec3 evalDiffuseBSDF(vec3 color, vec3 normal)
{
   return texture(sky_diffuse_cubemap, normal).rgb * color;
}

vec3 evalGlossyBSDF(vec3 color, vec3 normal)
{
   vec3 reflected_vector = normalize(reflect(attr_position-eye_position, normal));
   return texture(sky_cubemap, reflected_vector).rgb;
}

vec3 surfaceGradient(vec3 normal, vec3 dPdx, vec3 dPdy, float dHdx, float dHdy)
{
   vec3 tangentX = cross(dPdy, normal);
   vec3 tangentY = cross(normal, dPdx);
   
   return (tangentX * dHdx + tangentY * dHdy) / dot(dPdx, tangentX);
}

vec3 evalBump(bool invert, float strength, float max_displacement_distance, float heightmap_value, vec2 heightmap_value_differentials, vec3 normal)
{
   vec3 dPdx = dFdxFine(attr_position);
   vec3 dPdy = dFdyFine(attr_position);

   float dHdx = heightmap_value_differentials.x - heightmap_value;
   float dHdy = heightmap_value_differentials.y - heightmap_value;
   //float dHdx = dFdxFine(heightmap_value);
   //float dHdy = dFdyFine(heightmap_value);

   if (invert)
      max_displacement_distance *= -1.0;
   
   float det = dot(dPdx, cross(dPdy, normal));
   float absdet = abs(det);

   vec3 disturbed_normal = normalize(/*absdet**/normal - /*sign(det)**/max_displacement_distance* surfaceGradient(normal, dPdx, dPdy, dHdx, dHdy));
   return  mix(normal, disturbed_normal, strength);
}

#ifdef USE_NORMAL_MAPPING
vec3 evalNormalMap(vec3 normal_color, float scale)
{
   //vec3 fixed_tangent = normalize(tangent - dot(tangent, normal) * normal);
   vec3 bitangent = cross(tangent, normal);  
   vec3 normal_in_tangent_space = 2.0*normal_color - vec3(1.0);
   normal_in_tangent_space.x *= scale;
   normal_in_tangent_space.y *= scale;

   mat3 TBN = mat3(tangent, bitangent, normal);
   vec3 new_normal = TBN * normal_in_tangent_space;   
   return new_normal;
}
#endif

void sampleTexture(sampler2D tex, vec3 uvw, mat3x2 transform, out vec3 color, out float alpha)
{
   vec2 uv = transform*vec3(uvw.xy, 1.0);
	vec4 tex_sample = texture(tex, uv);
	color=tex_sample.rgb;
   alpha = tex_sample.a;
}

void sampleTextureDifferentials(sampler2D tex,                                
                                vec3 uvw,
                                mat3x2 transform,
                                out vec3 color,
                                out vec3 color_at_dx,
                                out vec3 color_at_dy,
                                out float alpha,
                                out float alpha_at_dx,
                                out float alpha_at_dy)
{
   vec2 uv = transform*vec3(uvw.xy, 1.0);
   vec4 tex_sample = texture(tex, uv);
   color = tex_sample.rgb;
   alpha = tex_sample.a;

   /*float inv_tex_size = 1.0 / textureSize(tex, 0).x;
   vec2 dUVdx = dFdx(uv);
   if (length(dUVdx) < inv_tex_size)
      dUVdx = dUVdx / length(dUVdx)*inv_tex_size;

   vec2 dUVdy = dFdy(uv);
   if (length(dUVdy) < inv_tex_size)
      dUVdy = dUVdy / length(dUVdy)*inv_tex_size;*/
   vec2 dUVdx = dFdx(uv);
   vec2 dUVdy = dFdy(uv);
   tex_sample = texture(tex, uv + dUVdx);
   color_at_dx = tex_sample.rgb;
   alpha_at_dx = tex_sample.a;

   tex_sample = texture(tex, uv + dUVdy);
   color_at_dy = tex_sample.rgb;
   alpha_at_dy = tex_sample.a;
}

 void main()
 {    
    %s
    //shading_result.rgb = Image_Texture_Node_Color;
 }