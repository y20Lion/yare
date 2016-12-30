~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ VertexShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
layout(location = 0) in vec2 position;

out vec2 pixel_uv;

void main()
{
   gl_Position = vec4(position, 0.0, 1.0);
   pixel_uv = position;
}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ FragmentShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include "glsl_voxelizer_defines.h"
#include "glsl_global_defines.h"
#include "common.glsl"
#include "scene_uniforms.glsl"

layout(binding = BI_DEPTH_TEXTURE) uniform sampler2D depths_texture;
layout(location = BI_VOXELS_TEXTURE) uniform sampler3D voxels;
layout(location = BI_MATRIX_WORLD_PROJ) uniform mat4 matrix_world_proj;
layout(location = BI_VOXELS_AABB_PMIN) uniform vec3 aabb_pmin;
layout(location = BI_VOXELS_AABB_PMAX) uniform vec3 aabb_pmax;

in vec2 pixel_uv;
out vec3 out_color;

vec3 transform(mat4 mat, vec3 v)
{
   vec4 tmp = mat * vec4(v, 1.0);
   return tmp.xyz / tmp.w;
}

vec3 positionInWorldFromDepthBuffer(vec2 pixel_ndc)
{
   float depth_ndc = texture(depths_texture, 0.5*(pixel_ndc + 1.0)).x * 2.0 - 1.0;
   return transform(matrix_world_proj, vec3(pixel_ndc, depth_ndc));
}

vec3 traceRay(vec3 start_voxel_pos, vec3 trace_dir)
{
   vec3 abs_trace_dir = abs(trace_dir);
   vec3 trace_step = trace_dir / max(max(abs_trace_dir.x, abs_trace_dir.y), abs_trace_dir.z);   
   
   vec3 voxel_pos = start_voxel_pos+2.0*trace_step;
   float opacity = 0.0;
   vec4 voxel_color = vec4(0.0);
   for (int i = 0; i < 16 && opacity == 0.0; ++i, voxel_pos += trace_step)
   {          
      voxel_color = texelFetch(voxels, ivec3(voxel_pos), 0);
      opacity = voxel_color.a;
   }
   /*for (int i = 0; i < 32 && opacity == 0.0; ++i, voxel_pos += trace_step)
   { 
      ivec3 pos = ivec3(voxel_pos);
      ivec3 macro_pos = pos  / 4 ;
      ivec3 sub_pos = pos % 4;
      //if (i % 4 == 0)        
         voxel_color = texelFetch(voxels, macro_pos + sub_pos, 0);
      opacity = voxel_color.a;
   }*/

   if (opacity == 0.0)
      return vec3(0);
   else
      return voxel_color.rgb;   
}

void main()
{
   vec3 position_ws = positionInWorldFromDepthBuffer(pixel_uv);
   vec3 normal_ws = normalize(cross(dFdx(position_ws.xyz), dFdy(position_ws.xyz)));
  
   vec3 start_voxel_pos = (position_ws - aabb_pmin) / (aabb_pmax-aabb_pmin) * textureSize(voxels, 0);
   vec3 view_vector = normalize(eye_position - position_ws);
   vec3 trace_dir = reflect(-view_vector, normal_ws);   

   vec3 color = traceRay(start_voxel_pos, trace_dir);
   color += traceRay(start_voxel_pos, normal_ws);

   out_color = color*0.5;
}
