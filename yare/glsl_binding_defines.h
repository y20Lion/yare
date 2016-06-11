#pragma once

// uniforms
#define BI_KERNEL_SIZE 1
#define BI_BLOOM_THRESHOLD 2

// images
#define BI_INPUT_IMAGE 0
#define BI_OUTPUT_IMAGE 1
#define BI_HISTOGRAMS_IMAGE 2

// textures
#define BI_SKY_CUBEMAP 10
#define BI_SKY_DIFFUSE_CUBEMAP 11

#define BI_INPUT_TEXTURE 0
#define BI_SCENE_TEXTURE 4
#define BI_BLOOM_TEXTURE 1

// uniforms buffers
#define BI_SCENE_UNIFORMS 1
#define BI_SURFACE_CONSTANT_UNIFORMS 2
#define BI_SURFACE_DYNAMIC_UNIFORMS 3

// ssbos
#define BI_EXPOSURE_VALUES_SSBO 4
#define BI_LIGHTS_SSBO 5