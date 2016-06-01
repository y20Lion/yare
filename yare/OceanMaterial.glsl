~~~~~~~~~~~~~~~~~~~ VertexShader ~~~~~~~~~~~~~~~~~~~~~
#version 450
#include "glsl_binding_defines.h"
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

#include "surface_uniforms.glsl"

void main()
{
   gl_Position = vec4(matrix_world_local*vec4(position, 1.0), 1.0);
   /*vec3 pos = position;
   pos.xyz *= 300.0;
   //pos.z -= 1.7;
   gl_Position = vec4(pos, 1.0);*/
}
~~~~~~~~~~~~~~~~~~~ TessControlShader ~~~~~~~~~~~~~~~~~~~~~
#version 450
#include "glsl_binding_defines.h"
layout(vertices = 3) out;

#include "scene_uniforms.glsl"
#include "common_ocean.glsl"

float getPostProjectionSphereExtent(vec3 origin, float diameter)
{
   vec4 clip_pos = matrix_view_world* vec4(origin, 1.0);
   return abs(diameter * proj_coeff_11/ clip_pos.w);
}

float calculateTessellationFactor(vec3 p0, vec3 p1)
{
   float edge_length = distance(p0, p1);
   vec3 middle = (p0 + p1) / 2;

   return clamp(tessellation_edges_per_screen_height * getPostProjectionSphereExtent(middle, edge_length), 1.0, 64.0);
}

void main(void)
{
   vec3 p0 = gl_in[(gl_InvocationID + 2) % 3].gl_Position.xyz;
   vec3 p1 = gl_in[(gl_InvocationID + 1) % 3].gl_Position.xyz;

   //gl_TessLevelOuter[gl_InvocationID] = calculateTessellationFactor(p0, p1);
   vec3 middle_point = (p0 + p1) / 2;
   float dist = distance(middle_point, eye_position);
   float max_distance = 500.0;
   float percentage = clamp(dist / max_distance, 0.0, 1.0);
   gl_TessLevelOuter[gl_InvocationID] = 1.0 + (1.0-percentage)*63.0;

   barrier();
   if (gl_InvocationID == 0)
      gl_TessLevelInner[0] = (gl_TessLevelOuter[0] + gl_TessLevelOuter[1] + gl_TessLevelOuter[2])/3.0;
   
   gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}

~~~~~~~~~~~~~~~~~~~ TessEvaluationShader ~~~~~~~~~~~~~~~~~~~~~
#version 450
#include "glsl_binding_defines.h"

layout(triangles, fractional_even_spacing, ccw) in;

#include "surface_uniforms.glsl"
#include "scene_uniforms.glsl"

out vec3 attr_position3;
out vec3 flat_position;

#include "common_ocean.glsl"

vec3 interpolate(vec3 v0, vec3 v1, vec3 v2)
{
   return gl_TessCoord.x*v0 + gl_TessCoord.y*v1 + gl_TessCoord.z*v2;
}

vec4 interpolate(vec4 v0, vec4 v1, vec4 v2)
{
   return gl_TessCoord.x*v0 + gl_TessCoord.y*v1 + gl_TessCoord.z*v2;
}

void main()
{
   flat_position = interpolate(gl_in[0].gl_Position.xyz, gl_in[1].gl_Position.xyz, gl_in[2].gl_Position.xyz);   
   attr_position3 = evalOceanPosition(flat_position);

   gl_Position = matrix_view_world * vec4(attr_position3, 1.0);
}

~~~~~~~~~~~~~~~~~~ FragmentShader ~~~~~~~~~~~~~~~~~~~~~~
#version 450
#include "glsl_binding_defines.h"

vec3 light_direction = vec3(0.0, 0.2, 1.0);
vec3 light_color = vec3(1.0, 1.0, 1.0);

#include "scene_uniforms.glsl"

in vec3 attr_position3;
in vec3 flat_position;

layout(location = 0, index = 0) out vec4 shading_result;

#include "common_ocean.glsl"

vec3 evalDiffuseBSDF(vec3 color, vec3 normal)
{
   return max(dot(normal, light_direction), 0.02) * color * light_color;
}

vec3 evalGlossyBSDF(vec3 color, vec3 normal)
{
   vec3 reflected_vector = normalize(reflect(attr_position3 - eye_position, normal));
   /*vec3 a = dFdxFine(reflected_vector);
   vec3 b = dFdxFine(reflected_vector);
   float dist = distance(attr_position3, eye_position);
   return textureGrad(sky_cubemap, reflected_vector, a*dist/5.0, b*dist/5.0).rgb;*/
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
   vec3 pos = evalOceanPosition(flat_position);
   vec3 posX = evalOceanPosition(flat_position + /*dFdx(flat_position)*/vec3(1.0, 0.0, 0.0));
   vec3 posY = evalOceanPosition(flat_position + /*dFdy(flat_position)*/vec3(0.0, 1.0, 0.0));
   vec3 normal = normalize(cross(posX - pos, posY - pos));
   
   vec3 specular = evalGlossyBSDF(vec3(1.0), normal);
   vec3 eye_vector = normalize(eye_position - attr_position3);
   float fresnel = fresnel(dot(normal, eye_vector));
   vec3 refraction = dot(normal, eye_vector)*vec3(0.05, 0.17, 0.25)*0.15*clamp(1.0+pos.z, 1.0, 1.6);//;clamp(attr_position3.z,0.7,1.0)*2.0;//vec3(0.8, 0.9, 0.6);
   vec3 color = mix(refraction, specular, vec3(fresnel));
   shading_result = vec4(color, 1.0);
}