
layout(binding = BI_SKY_CUBEMAP) uniform samplerCube sky_cubemap;
layout(binding = BI_SKY_DIFFUSE_CUBEMAP) uniform samplerCube sky_diffuse_cubemap;
layout(binding = BI_SSAO_TEXTURE) uniform sampler2D ssao_texture;
layout(std140, binding = BI_SCENE_UNIFORMS) uniform SceneUniforms
{
   mat4 matrix_proj_world;
   vec3 eye_position;
   float time;
   float delta_time;
   float znear;
   float zfar;
   float proj_coeff_11;
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
