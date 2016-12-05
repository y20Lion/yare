~~~~~~~~~~~~~~~~~~~ VertexShader ~~~~~~~~~~~~~~~~~~~~~
#include "glsl_global_defines.h"

#include "scene_uniforms.glsl"

layout(location = 0) in vec3 position;

layout(location = 0) uniform mat4 captured_matrix_world_proj;
layout(location = 1) uniform vec2 znear_far;
layout(location = 2) uniform int debug_froxel;

layout(std430, binding = 1) buffer EnabledFroxelsSSBO
{
   bool enabled_froxels[];
};

out vec3 color;


float convertFroxelZtoCameraZ(float froxel_z, float znear, float zfar)
{
   float z = pow(froxel_z, froxel_z_distribution_factor);

   return mix(znear, zfar, z);
}

void main()
{
   int froxel_id = gl_VertexID / 24;
   if (enabled_froxels[froxel_id])
   {
      float n = znear_far.x;
      float f = znear_far.y;
      
      float eye_space_depth = convertFroxelZtoCameraZ(position.z, znear_far.x, znear_far.y);
      float A = -(f+n)/(f-n);
      float B = -(2.0*f*n)/(f-n);
      float clip_space_depth = (-A*eye_space_depth + B) / eye_space_depth ;

      gl_Position = matrix_proj_world * captured_matrix_world_proj * vec4(2.0*position.xy - 1.0, clip_space_depth, 1.0);
   
   }
   else
      gl_Position = vec4(-10.0f, -10.0f, -10.0f, 1.0f);

   color = (froxel_id == debug_froxel) ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
}

~~~~~~~~~~~~~~~~~~ FragmentShader ~~~~~~~~~~~~~~~~~~~~~~

layout(location = 0) out vec4 shading_result;

in vec3 color;

void main()
{
   shading_result = vec4(color, 1.0);
}
