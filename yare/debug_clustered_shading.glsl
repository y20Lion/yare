~~~~~~~~~~~~~~~~~~~ VertexShader ~~~~~~~~~~~~~~~~~~~~~
#include "glsl_global_defines.h"

#include "scene_uniforms.glsl"

layout(location = 0) in vec3 position;

layout(location = 0) uniform mat4 captured_matrix_world_proj;

layout(std430, binding = 1) buffer EnabledClustersSSBO
{
   bool enabled_clusters[];
};

void main()
{
   if (enabled_clusters[gl_VertexID/24])
      gl_Position = matrix_proj_world * captured_matrix_world_proj * vec4(2.0*position - 1.0, 1.0);
   else
      gl_Position = vec4(-10.0f, -10.0f, -10.0f, 1.0f);
}

~~~~~~~~~~~~~~~~~~ FragmentShader ~~~~~~~~~~~~~~~~~~~~~~

layout(location = 0) out vec4 shading_result;

void main()
{
   shading_result = vec4(1.0, 0.0, 0.0, 1.0);
}
