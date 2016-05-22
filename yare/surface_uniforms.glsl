
layout(std140, binding = BI_SURFACE_DYNAMIC_UNIFORMS) uniform SurfaceDynamicUniforms
{
   mat4 matrix_view_local;
   mat4 normal_matrix_world_local;
   mat4x3 matrix_world_local;
};