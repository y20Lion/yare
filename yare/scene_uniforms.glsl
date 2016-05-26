
layout(binding = BI_SKY_CUBEMAP) uniform samplerCube sky_cubemap;
layout(std140, binding = BI_SCENE_UNIFORMS) uniform SceneUniforms
{
   mat4 matrix_view_world;
   vec3 eye_position;
   float time;
   float delta_time;
};
