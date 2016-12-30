~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ VertexShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include "glsl_global_defines.h"
#include "glsl_voxelizer_defines.h"

layout(location = 0) in vec3 position;

#include "surface_uniforms.glsl"

void main()
{
   gl_Position = vec4(matrix_world_local * vec4(position, 1.0), 1.0);  
}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ GeometryShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include "glsl_voxelizer_defines.h"

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = BI_ORTHO_MATRICES) uniform mat4 matrix_ortho_world[3];

flat out int main_axis; 

void main() 
{
	vec3 triangle_normal = cross(gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz, gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz);
   triangle_normal = abs(normalize(triangle_normal));

   if (triangle_normal.x > triangle_normal.y && triangle_normal.x > triangle_normal.z)
   {     
      main_axis = 0;
	}
	else if (triangle_normal.y > triangle_normal.x && triangle_normal.y > triangle_normal.z)
   {      
      main_axis = 1;
   }
   else
   {
      main_axis = 2;
	}

   for (int i = 0; i < 3; i++) 
	{
      gl_Position = matrix_ortho_world[main_axis]* gl_in[i].gl_Position;
	   EmitVertex();
	}
   EndPrimitive();
}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ FragmentShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include "glsl_voxelizer_defines.h"

layout(binding = BI_VOXELS_IMAGE, rgba16f) uniform writeonly image3D voxels;

flat in int main_axis;

void main()
{
   int tex_size = imageSize(voxels).x;

   vec3 voxel_coord = vec3(gl_FragCoord.xy, tex_size*gl_FragCoord.z);
   if (main_axis == 0)
   {
      voxel_coord = voxel_coord.zxy;
   }
   else if(main_axis == 1)
   {
      voxel_coord = voxel_coord.xzy;
   }
   else if (main_axis == 2)
   {
      voxel_coord = voxel_coord.xyz;
   }

   imageStore(voxels, ivec3(voxel_coord), vec4(1.0));
}