~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ComputeShader ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#version 450
#include "glsl_cubemap_filtering_defines.h"
#include "common_cubemap_filtering.glsl"

#define WORKGROUP_WIDTH TILE_WIDTH/2

layout(local_size_x = WORKGROUP_WIDTH, local_size_y = WORKGROUP_WIDTH, local_size_z = 1) in;

layout(binding = BI_LATLONG_TEXTURE) uniform sampler2D latlong_texture;
layout(binding = BI_OUTPUT_IMAGE, rgba32f) uniform writeonly image2D output_image;

shared lowp vec4 shared_pixels[WORKGROUP_WIDTH*WORKGROUP_WIDTH];

uint toFlatIndex(uvec2 coord)
{
   return coord.x + coord.y*WORKGROUP_WIDTH;
}

float texelSolidAngle(ivec2 coord)
{
   vec2 size = textureSize(latlong_texture, 0);
   float theta0 = coord.y / size.y * PI;
   float theta1 = (coord.y+1) /size.y * PI;
   return 1.0 / size.x * 2 * PI * (-cos(theta1) + cos(theta0));
}

vec4 latlongValue(sampler2D latlong_texture, ivec2 coord)
{
   vec3 color = max(texelFetch(latlong_texture, coord, 0).rgb - DIR_LIGHT_THRESHOLD, vec3(0.0));
   float solid_angle = any(greaterThan(color, vec3(0))) ? texelSolidAngle(coord) : 0;
   return vec4(color*solid_angle, solid_angle);
}

void main()
{
   ivec2 pixel_coord = ivec2(gl_GlobalInvocationID.xy)*2;
   shared_pixels[toFlatIndex(gl_LocalInvocationID.xy)] =   latlongValue(latlong_texture, pixel_coord + ivec2(0, 0))
                                                         + latlongValue(latlong_texture, pixel_coord + ivec2(1, 0))
                                                         + latlongValue(latlong_texture, pixel_coord + ivec2(0, 1))
                                                         + latlongValue(latlong_texture, pixel_coord + ivec2(1, 1));

   barrier();

   int max_stride = WORKGROUP_WIDTH / 2;
   for (int stride = 1; stride < max_stride; stride *= 2)
   {
      if (mod(vec2(gl_LocalInvocationID.xy), float(stride)) == 0.0)
      {
         shared_pixels[toFlatIndex(gl_LocalInvocationID.xy)] =   shared_pixels[toFlatIndex(gl_LocalInvocationID.xy + uvec2(0, 0))]
                                                               + shared_pixels[toFlatIndex(gl_LocalInvocationID.xy + uvec2(stride,0))]
                                                               + shared_pixels[toFlatIndex(gl_LocalInvocationID.xy + uvec2(0, stride))]
                                                               + shared_pixels[toFlatIndex(gl_LocalInvocationID.xy + uvec2(stride, stride))];
      }
      barrier();      
   }

   ivec2 coord_in_reduced_texture = ivec2(gl_WorkGroupID.xy);
   imageStore(output_image, coord_in_reduced_texture, shared_pixels[0]);

}