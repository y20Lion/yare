
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
   ivec4 viewport;
   float tessellation_edges_per_screen_height;
   float cluster_z_distribution_factor;
};

struct SphereLight
{
   vec3 color;
   float size;
   vec3 position;
   float radius;
};

struct SpotLight
{
   vec3 color;
   float angle_smooth;
   vec3 position;
   float cos_half_angle;
   vec3 direction;
   float radius;
};

struct RectangleLight
{
   vec3 color;
   float size_x;
   vec3 position;
   float size_y;
   vec3 direction_x;
   float radius;
   vec3 direction_y;
   int padding1;
};

struct SunLight
{
   vec3 color;
   float size;
   vec3 direction;
   int padding1;
};

layout(binding = BI_LIGHT_LIST_HEAD) uniform usampler3D light_list_head;

layout(std430, binding = BI_LIGHT_LIST_DATA_SSBO) buffer LightDataSSBO
{
   int light_list_data[];
};

layout(std430, binding = BI_SPHERE_LIGHTS_SSBO) buffer SphereLightsSSBO
{
   ivec3 light_clusters_dims;
   int padding0;
   SphereLight sphere_lights[];
};

layout(std430, binding = BI_SPOT_LIGHTS_SSBO) buffer SpotLightsSSBO
{
   ivec4 padding1;   
   SpotLight spot_lights[];
};

layout(std430, binding = BI_RECTANGLE_LIGHTS_SSBO) buffer RectangleLightsSSBO
{
   ivec4 padding3;
   RectangleLight rectangle_lights[];
};

layout(std430, binding = BI_SUN_LIGHTS_SSBO) buffer SunLightsSSBO
{
   ivec4 padding2;   
   SunLight sun_lights[];
};
