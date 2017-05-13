~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ VertexShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
layout(location = 0) in vec2 position;

out vec2 pixel_uv;

void main()
{
   gl_Position = vec4(position, 0.0, 1.0);
   pixel_uv = position;
}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ FragmentShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define USE_SDF_VOLUME

#include "glsl_voxelizer_defines.h"
#include "glsl_global_defines.h"
#include "common.glsl"
#include "scene_uniforms.glsl"

layout(binding = BI_DEPTH_TEXTURE)               uniform sampler2D depths_texture;
layout(binding = BI_VOXELS_OCCLUSION_TEXTURE)    uniform sampler3D occlusion_voxels;
layout(binding = BI_VOXELS_ILLUMINATION_TEXTURE) uniform sampler3D illumination_voxels;
layout(binding = BI_HEMISPHERE_SAMPLES_TEXTURE)  uniform sampler1D hemisphere_samples;
layout(location = BI_MATRIX_WORLD_PROJ)          uniform mat4 matrix_world_proj;
layout(location = BI_VOXELS_AABB_PMIN)           uniform vec3 aabb_pmin;
layout(location = BI_VOXELS_AABB_PMAX)           uniform vec3 aabb_pmax;

in vec2 pixel_uv;
out vec4 out_color;

vec3 transform(mat4 mat, vec3 v)
{
   vec4 tmp = mat * vec4(v, 1.0);
   return tmp.xyz / tmp.w;
}

vec3 positionInWorldFromDepthBuffer(vec2 pixel_ndc, float depth)
{
   float depth_ndc = depth * 2.0 - 1.0;
   return transform(matrix_world_proj, vec3(pixel_ndc, depth_ndc));
}

const int max_steps = 32;

vec4 traceRay(vec3 start_voxel_pos, vec3 trace_dir)
{
   
   
   vec3 abs_trace_dir = abs(trace_dir);
   vec3 trace_step = trace_dir / max(max(abs_trace_dir.x, abs_trace_dir.y), abs_trace_dir.z);   
   
   vec3 voxel_pos = start_voxel_pos+trace_step;
   float opacity = 0.0;

   //return vec4(texelFetch(illumination_voxels, ivec3(start_voxel_pos), 0).rgb, 1.0);
   
   int i = 0;
   while (i < max_steps && opacity == 0.0)
   {
      voxel_pos += trace_step;
      i++;
      opacity = texelFetch(occlusion_voxels, ivec3(voxel_pos), 0).r; 
   }

   if (opacity == 0.0)
      return vec4(0,0,0,1);

   vec3 result = texelFetch(illumination_voxels, ivec3(voxel_pos), 0).rgb* (1.0 - smoothstep(max_steps*0.8, float(max_steps), float(i)));
   float weight = 1.0;//float(max_steps+1-i)/max_steps;
    //return vec4(vec3(1.0), 1.0);
   return vec4(result*weight, weight);
}

float traceRaySDF(vec3 start_voxel_pos, vec3 trace_dir)
{
   
   
   vec3 voxel_size = sdf_volume_size / textureSize(sdf_volume, 0);

   vec3 current_pos = start_voxel_pos;
   current_pos += trace_dir*voxel_size*0.25;
   float res = 1.0f;

   
   float t = 0.0;
   for (int i = 0; i < max_steps; ++i)
   {
      
      vec3 uvw = (current_pos - sdf_volume_bound_min) / sdf_volume_size;
      if (clamp(uvw, vec3(0.0), vec3(1.0)) != uvw)
         return res;
                 
      float distance = texture(sdf_volume, uvw).r;
      res = min(res, 10.0*abs(distance)/t);
      if (res < 0.05 )
         return 0.0;

      t += abs(distance);
      current_pos += trace_dir*abs(distance);
   }

   return res;
}

vec3 transformLocalSpaceToWorldSpace(vec3 H, vec3 N)
{
   vec3 UpVector = abs(N.z) < 0.999 ? vec3(0, 0, 1) : vec3(1, 0, 0);
   vec3 TangentX = normalize(cross(UpVector, N));
   vec3 TangentY = cross(N, TangentX);
   // Tangent to world space
   return TangentX * H.x + TangentY * H.y + N * H.z;
}

float rand()
{
   vec2 co = gl_FragCoord.xy;
   return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

void main()
{
   float depth = texture(depths_texture, 0.5*(pixel_uv + 1.0)).x;
   if (depth == 1.0)
      discard;


   float z_eye_space = 2.0f * znear * zfar / (znear + zfar - (2.0*depth-1.0) * (zfar - znear));

   vec3 position_ws = positionInWorldFromDepthBuffer(pixel_uv, depth);
   vec3 normal_ws = normalize(cross(dFdx(position_ws.xyz), dFdy(position_ws.xyz)));
  
   vec3 voxel_size = (aabb_pmax-aabb_pmin)/textureSize(occlusion_voxels, 0);
   vec3 start_voxel_pos = (position_ws - aabb_pmin) / voxel_size;
   vec3 view_vector = normalize(eye_position - position_ws);
   vec3 trace_dir = normalize(reflect(-view_vector, normal_ws));   

   ivec2 repeat_coords = ivec2(gl_FragCoord.xy) % SAMPLES_SIDE;
   int sample_index = repeat_coords.x + repeat_coords.y * SAMPLES_SIDE;

   vec4 color = vec4(0);
   start_voxel_pos += normal_ws*(2.0*voxel_size.x);
   for (int i = 0; i < RAY_COUNT; ++i)
   {
      //vec3 diffuse_dir = texelFetch(hemisphere_samples, RAY_COUNT*sample_index+i, 0).xyz;
      vec3 diffuse_dir = texelFetch(hemisphere_samples, (int(rand()*12000) + i) % (12000), 0).xyz;
      diffuse_dir = transformLocalSpaceToWorldSpace(diffuse_dir, normal_ws);
      color += traceRay(start_voxel_pos, diffuse_dir);
   }
   color /= color.a;
   //color += traceRay(start_voxel_pos, normal_ws);
   /*vec3 color = vec3(traceRaySDF(position_ws, trace_dir));
   color += traceRay(position_ws, normal_ws);*/
   out_color = vec4(color.rgb, z_eye_space);
}
