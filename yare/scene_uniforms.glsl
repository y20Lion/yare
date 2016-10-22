
layout(binding = BI_SKY_CUBEMAP) uniform samplerCube sky_cubemap;
layout(binding = BI_SKY_DIFFUSE_CUBEMAP) uniform samplerCube sky_diffuse_cubemap;
layout(binding = BI_SSAO_TEXTURE) uniform sampler2D ssao_texture;

#ifdef USE_AO_VOLUME
layout(binding = BI_AO_VOLUME) uniform sampler3D ao_volume;
#endif

#ifdef USE_SDF_VOLUME
layout(binding = BI_SDF_VOLUME) uniform sampler3D sdf_volume;
#endif

layout(std140, binding = BI_SCENE_UNIFORMS) uniform SceneUniforms
{
   mat4 matrix_proj_world;
   vec3 eye_position;
   float time;
   vec3 ao_volume_bound_min;
   float delta_time;
   vec3 ao_volume_size;
   float proj_coeff_11;

   vec3 sdf_volume_bound_min;
   float znear;
   vec3 sdf_volume_size;
   float zfar;   
   float tessellation_edges_per_screen_height;
};

struct Light
{
   vec3 color;
   int type;   
   vec4 data[3];
};

layout(std430, binding = BI_LIGHTS_SSBO) buffer LightsSSBO
{
   vec4 dummy;
   Light lights[];
};
