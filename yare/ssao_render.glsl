~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ VertexShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

layout(location = 0) in vec2 position;

out vec2 pixel_uv;

void main()
{
   gl_Position = vec4(position, 0.0, 1.0);
   pixel_uv = position;
}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ FragmentShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include "glsl_ssao_defines.h"
#include "common.glsl"
layout(binding = BI_DEPTHS_TEXTURE) uniform sampler2D depths_texture;
layout(binding = BI_NORMALS_TEXTURE) uniform sampler2D normals_texture;
layout(binding = BI_RANDOM_TEXTURE) uniform sampler1D random_texture;
layout(location = BI_MATRIX_VIEW_PROJ) uniform mat4 matrix_view_proj;
layout(location = BI_MATRIX_VIEW_WORLD) uniform mat4 matrix_view_world;

in vec2 pixel_uv;
out float out_ambient_occlusion;

vec3 transform(mat4 mat, vec3 v)
{
   vec4 tmp = mat * vec4(v, 1.0);
   return tmp.xyz / tmp.w;
}

vec3 positionInViewFromDepthBuffer(vec2 pixel_ndc)
{
   float depth_ndc = texture(depths_texture, 0.5*(pixel_ndc + 1.0)).x * 2.0 - 1.0;
   return transform(matrix_view_proj, vec3(pixel_ndc, depth_ndc));
}

float rand(vec2 co)
{
   return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

const int sample_count = 10;
const float ssao_darken = 5.0;
const float ssao_reject_distance = 1.0;
const float ssao_radius = 0.05;

float occlusionAngle(vec2 direction, vec3 position_cs, vec3 normal_cs)
{  
   vec2 sample_pixel_ndc = pixel_uv + direction*ssao_radius;
   vec3 sample_position_cs = positionInViewFromDepthBuffer(sample_pixel_ndc);
   vec3 v = normalize(sample_position_cs - position_cs);

   float depth_difference = abs(position_cs.z - sample_position_cs.z);
   return mix(acos(dot(v, normal_cs)), PI, clamp(depth_difference / ssao_reject_distance, 0.0, 1.0));
}

void main()
{
   vec3 position_cs = positionInViewFromDepthBuffer(pixel_uv);
   vec3 normal_cs = mat3(matrix_view_world) * normalize(texture(normals_texture, 0.5*(pixel_uv + 1.0)).xyz);

   int offset = clamp(int(rand(gl_FragCoord.xy) * 1024), 1, 500);
   out_ambient_occlusion = 0.0;
   float u;
   for (int i = 0; i < sample_count; ++i)
   {
      u = texelFetch(random_texture, i + offset, 0).x;
      vec2 random_direction = u*vec2(cos(u * 2*PI), sin(u * 2*PI));
      
      float angle_left = occlusionAngle(random_direction, position_cs, normal_cs);
      float angle_right = occlusionAngle(-random_direction, position_cs, normal_cs);
      float ao = min((angle_left+ angle_right)/PI, 1.0);

      out_ambient_occlusion += ao;
   }
   out_ambient_occlusion = pow(out_ambient_occlusion/sample_count, ssao_darken);
}