~~~~~~~~~~~~~~~~~~~ VertexShader ~~~~~~~~~~~~~~~~~~~~~
#version 450
#include "glsl_binding_defines.h"
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

layout(std140, binding = BI_SURFACE_DYNAMIC_UNIFORMS) uniform MatUniform
{
   mat4 matrix_view_local;
   mat4 normal_matrix_world_local;
   mat4x3 matrix_world_local;
};

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

layout(std140, binding = BI_SCENE_UNIFORMS) uniform SceneUniforms
{
   mat4 matrix_view_world;
   vec3 eye_position;
   float time;
};

void main(void)
{
   /*vec4 projected_point[2];
   projected_point[0] = matrix_view_world * gl_in[gl_InvocationID].gl_Position;
   projected_point[0].xyz /= projected_point[0].w;

   projected_point[1] = matrix_view_world * gl_in[(gl_InvocationID + 1) % 3].gl_Position;
   projected_point[1].xyz /= projected_point[1].w;

   float d = distance(projected_point[0].xy, projected_point[1].xy);
   float pixels_per_segment = 1.0;
   float tess_level = clamp(d*10.0, 1.0,64.0);*/
   //tess_level = 1.0;

   vec3 p0 = gl_in[gl_InvocationID].gl_Position.xyz;
   vec3 p1 = gl_in[(gl_InvocationID +1) % 3].gl_Position.xyz;


   vec3 midpoint = (p0+p1)*0.5;
   float dist = distance(eye_position, midpoint);
   //dist /= 50.0;
   gl_TessLevelOuter[(gl_InvocationID + 2) % 3] = dist;
   /*gl_TessLevelOuter[1] = tess_level;
   gl_TessLevelOuter[2] = tess_level;*/
   //gl_TessLevelOuter[3] = 2.0;
    barrier();
    if (gl_InvocationID == 0)
      gl_TessLevelInner[0] = gl_TessLevelOuter[(gl_InvocationID + 2) % 3];// max(max(gl_TessLevelOuter[0], gl_TessLevelOuter[1]), gl_TessLevelOuter[2]);
   //gl_TessLevelInner[1] = 2.0;

   gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}

~~~~~~~~~~~~~~~~~~~ TessEvaluationShader ~~~~~~~~~~~~~~~~~~~~~
#version 450
#include "glsl_binding_defines.h"

layout(triangles, fractional_even_spacing, ccw) in;

layout(std140, binding = BI_SURFACE_DYNAMIC_UNIFORMS) uniform MatUniform
{
   mat4 matrix_view_local;
   mat4 normal_matrix_world_local;
   mat4x3 matrix_world_local;
};

layout(std140, binding = BI_SCENE_UNIFORMS) uniform SceneUniforms
{
   mat4 matrix_view_world;
   vec3 eye_position;
   float time;
};

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
layout(binding = BI_SKY_CUBEMAP) uniform samplerCube sky_cubemap;

layout(std140, binding = BI_SCENE_UNIFORMS) uniform SceneUniforms
{
   mat4 matrix_view_world;
   vec3 eye_position;
   float time;
};

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
   vec3 posX = evalOceanPosition(flat_position + vec3(1.0, 0.0, 0.0));
   vec3 posY = evalOceanPosition(flat_position + vec3(0.0, 1.0, 0.0));
   vec3 normal = normalize(cross(posX - pos, posY - pos));
   
   vec3 specular = evalGlossyBSDF(vec3(1.0), normal);
   vec3 eye_vector = normalize(eye_position - attr_position3);
   float fresnel = fresnel(dot(normal, eye_vector));
   vec3 refraction = dot(normal, eye_vector)*vec3(0.01, 0.036, 0.0484)*clamp(attr_position3.z,0.7,1.0)*2.0;//vec3(0.8, 0.9, 0.6);
   vec3 color = mix(refraction, specular, vec3(fresnel));
   shading_result = vec4(color, 1.0);
}