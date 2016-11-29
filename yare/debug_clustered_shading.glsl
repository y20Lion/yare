~~~~~~~~~~~~~~~~~~~ VertexShader ~~~~~~~~~~~~~~~~~~~~~
#include "glsl_global_defines.h"

#include "scene_uniforms.glsl"

layout(location = 0) in vec3 position;

layout(location = 0) uniform mat4 captured_matrix_world_proj;
layout(location = 1) uniform vec2 znear_far;
layout(location = 2) uniform int debug_cluster;

layout(std430, binding = 1) buffer EnabledClustersSSBO
{
   bool enabled_clusters[];
};

out vec3 color;


float convertClusterZtoCameraZ(float cluster_z, float znear, float zfar)
{
   float z = pow(cluster_z, cluster_z_distribution_factor);

   return mix(znear, zfar, z);
}

void main()
{
   int cluster_id = gl_VertexID / 24;
   if (enabled_clusters[cluster_id])
   {
      float n = znear_far.x;
      float f = znear_far.y;
      
      float eye_space_depth = convertClusterZtoCameraZ(position.z, znear_far.x, znear_far.y);
      float A = -(f+n)/(f-n);
      float B = -(2.0*f*n)/(f-n);
      float clip_space_depth = (-A*eye_space_depth + B) / eye_space_depth ;

      gl_Position = matrix_proj_world * captured_matrix_world_proj * vec4(2.0*position.xy - 1.0, clip_space_depth, 1.0);
   
   }
   else
      gl_Position = vec4(-10.0f, -10.0f, -10.0f, 1.0f);

   color = (cluster_id == debug_cluster) ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
}

~~~~~~~~~~~~~~~~~~ FragmentShader ~~~~~~~~~~~~~~~~~~~~~~

layout(location = 0) out vec4 shading_result;

in vec3 color;

void main()
{
   shading_result = vec4(color, 1.0);
}
