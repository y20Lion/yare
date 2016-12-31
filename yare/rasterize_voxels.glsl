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

layout(binding = BI_VOXELS_GBUFFER_IMAGE, rgba16f) uniform writeonly image3D voxels;
layout(location = BI_ORTHO_MATRICES) uniform mat4 matrix_ortho_world[3];

flat out int main_axis; 
flat out vec4 triangle_aabb; 
out vec2 pos_cs;

vec4 aabbOfDilatedTriangle(vec4 pos[3], vec2 half_pixel_size)
{
   vec4 aabb;
   aabb.xy = pos[0].xy;
	aabb.zw = pos[0].xy;

	aabb.xy = min( pos[1].xy, aabb.xy );
	aabb.zw = max( pos[1].xy, aabb.zw );
	
	aabb.xy = min( pos[2].xy, aabb.xy );
	aabb.zw = max( pos[2].xy, aabb.zw );

   aabb.xy -= half_pixel_size;
	aabb.zw += half_pixel_size;

   return aabb;
}

void dilateTriangle(vec2 half_pixel, inout vec4[3] pos)
{
   vec3 line[3]; // contains abc coefficients of line: ax + by + c = 0
   vec3 edge0 = vec3(pos[1].xy - pos[0].xy, 0);
   vec3 edge1 = vec3(pos[2].xy - pos[1].xy, 0);
   vec3 edge2 = vec3(pos[0].xy - pos[2].xy, 0);

   // compute the 3 lines equations that support the edge segments.
   line[0] = cross(edge0, vec3(pos[0].xy, 1));
   line[1] = cross(edge1, vec3(pos[1].xy, 1));
   line[2] = cross(edge2, vec3(pos[2].xy, 1));

   // move the 3 lines towards the exterior of the triangle (of a half pixel distance)
   line[0].z -= dot(half_pixel, abs(vec2(line[0].xy)));
   line[1].z -= dot(half_pixel, abs(vec2(line[1].xy)));
   line[2].z -= dot(half_pixel, abs(vec2(line[2].xy)));

   // compute intersection of new triangle lines to find where the new vertices are
   vec3 res0 = cross(line[2], line[0]);
   vec3 res1 = cross(line[0], line[1]);
   vec3 res2 = cross(line[1], line[2]);
   pos[0].xy = res0.xy / res0.z;
   pos[1].xy = res1.xy / res1.z;
   pos[2].xy = res2.xy / res2.z;
}

void main() 
{
	vec3 triangle_normal = cross(gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz, gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz);
   triangle_normal = abs(normalize(triangle_normal));

   vec2 half_pixel;
   vec3 voxels_size = vec3(imageSize(voxels));
   if (triangle_normal.x > triangle_normal.y && triangle_normal.x > triangle_normal.z)
   {     
      main_axis = 0;
      half_pixel = 1.0 / voxels_size.yz;
	}
	else if (triangle_normal.y > triangle_normal.x && triangle_normal.y > triangle_normal.z)
   {      
      main_axis = 1;
      half_pixel = 1.0 / voxels_size.zx;
   }
   else
   {
      main_axis = 2;
      half_pixel = 1.0 / voxels_size.xy;
	}

   vec4 pos[3];
   for (int i = 0; i < 3; ++i)
   {
      pos[i] = matrix_ortho_world[main_axis]* gl_in[i].gl_Position;
   }

   triangle_aabb = aabbOfDilatedTriangle(pos, half_pixel);

   dilateTriangle(half_pixel, pos);   
   
   for (int i = 0; i < 3; ++i)
   {
      gl_Position = pos[i];
      pos_cs = pos[i].xy;
      EmitVertex();
   }
   EndPrimitive();
}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ FragmentShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include "glsl_voxelizer_defines.h"

layout(binding = BI_VOXELS_GBUFFER_IMAGE, rgba16f) uniform writeonly image3D voxels;

flat in int main_axis;
flat in vec4 triangle_aabb; 
in vec2 pos_cs;

void main()
{
   if (any(lessThan(pos_cs, triangle_aabb.xy)) || any(greaterThan(pos_cs, triangle_aabb.zw)))
      discard;

   int tex_size = imageSize(voxels).x;

   ivec3 main_dir_step;
   vec3 voxel_coord = vec3(gl_FragCoord.xy, tex_size*gl_FragCoord.z);
   if (main_axis == 0)
   {
      main_dir_step = ivec3(1, 0, 0);
      voxel_coord = voxel_coord.zxy;
   }
   else if(main_axis == 1)
   {
      main_dir_step = ivec3(0, 1, 0);
      voxel_coord = voxel_coord.xzy;
   }
   else if (main_axis == 2)
   {
      main_dir_step = ivec3(0, 0, 1);
      voxel_coord = voxel_coord.xyz;
   }
   
   imageStore(voxels, ivec3(voxel_coord), vec4(1.0));
   if (fwidth(tex_size*gl_FragCoord.z) > 0.5)
   {
      imageStore(voxels, ivec3(voxel_coord)+main_dir_step, vec4(1.0));
      imageStore(voxels, ivec3(voxel_coord)-main_dir_step, vec4(1.0));
   }
}