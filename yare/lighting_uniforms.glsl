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
   ivec3 light_froxels_dims;
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

struct FroxelLightLists
{
   unsigned int start_offset;
   unsigned int sphere_light_count;
   unsigned int spot_light_count;
   unsigned int rectangle_light_count;
} froxel_light_lists;
