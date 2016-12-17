// TODO refactor with material shader

////////////////////////////////////// Radiance /////////////////////////////////////////////

float spotLightAttenuation(vec3 light_direction, float cos_spot_max_angle, vec3 spot_direction, float spot_smooth)
{
   float cos_angle = dot(light_direction, spot_direction);
   float cos_angle_start_smooth = (1.0 - cos_spot_max_angle)*spot_smooth + cos_spot_max_angle;

   return smoothstep(cos_spot_max_angle, cos_angle_start_smooth, cos_angle);  
}

vec3 sphereLightIncidentRadiance(SphereLight light, vec3 shade_position, float length)
{
   vec3 light_dir = (light.position - shade_position);
   length = max(light.size, length);
   //length = 0.05;
   float light_distance_sqr = max(dot(light_dir, light_dir), (length*length));
   float rcp_light_distance_sqr = 1.0f / light_distance_sqr;
   light_dir *= sqrt(rcp_light_distance_sqr);

   float restrict_influence_factor = pow(saturate(1.0 - pow(light_distance_sqr / (light.radius*light.radius), 2.0)), 2.0);
   return light.color / (4 * PI) * rcp_light_distance_sqr * restrict_influence_factor;
}

vec3 spotLightIncidentRadiance(SpotLight light, vec3 shade_position, float length)
{
   vec3 light_dir = (light.position - shade_position);
   float light_distance_sqr = max(dot(light_dir, light_dir), length*length);
   float rcp_light_distance_sqr = 1.0f / light_distance_sqr;
   light_dir *= sqrt(rcp_light_distance_sqr);

   float spot_attenuation =  spotLightAttenuation(light_dir, light.cos_half_angle, light.direction, light.angle_smooth);
   spot_attenuation *= smoothstep(0.0, 1.0, light_distance_sqr);

   float restrict_influence_factor = pow(saturate(1.0 - pow(light_distance_sqr / (light.radius*light.radius), 2.0)), 2.0);
   return light.color / (4 * PI) * rcp_light_distance_sqr * restrict_influence_factor * spot_attenuation;
   
}


////////////////////////////////////// Froxels /////////////////////////////////////////////

float convertFroxelZtoDepth(float froxel_z, float znear, float zfar)
{
   float z = pow(froxel_z, 2.0);

   return mix(znear, zfar, z);
}


float convertCameraZtoFroxelZ(float z_in_camera_space, float znear, float zfar)
{
   float z = (z_in_camera_space - znear) / (zfar - znear);
   float froxel_z = pow(z, 1.0f / froxel_z_distribution_factor);

   return clamp(froxel_z, 0.0f, 1.0f);
}

vec3 convertFroxelToCameraCoords(vec3 froxel_coords, vec4 frustum)
{
   float z = convertFroxelZtoDepth(froxel_coords.z, znear, zfar);

   float ratio = z / znear;
   float x = mix(frustum.x, frustum.y, froxel_coords.x) * ratio;
   float y = mix(frustum.z, frustum.w, froxel_coords.y) * ratio;

   return vec3(x, y, -z);
}

vec3 positionInFrustumAlignedVolumeTextures()
{
   vec2 current_ndc01_pos = (gl_FragCoord.xy - viewport.xy) / (viewport.zw);
   float z_eye_space = 2.0 * znear * zfar / (znear + zfar - (2.0*gl_FragCoord.z - 1.0) * (zfar - znear));
   float froxel_z = convertCameraZtoFroxelZ(z_eye_space, znear, zfar);

   return vec3(current_ndc01_pos, froxel_z);
}

FroxelLightLists fetchCurrentFroxelLightLists(vec3 pos_in_frustum)
{
   /*vec4 p = debug_proj_world*vec4(attr_position, 1.0);
   p /= p.w;*/

   ivec3 current_froxel_coords = ivec3(light_froxels_dims * pos_in_frustum);
   /*float z_eye_space = 2.0 * znear * zfar / (znear + zfar - p.z * (zfar - znear));
   float froxel_z = (z_eye_space - znear) / (zfar - znear);

   ivec3 current_froxel_coords = ivec3(light_froxels_dims * vec3((p.xy+1.0)*0.5, froxel_z));*/

   uvec2 froxel_data = texelFetch(light_list_head, current_froxel_coords, 0).xy;
   
   FroxelLightLists froxel_light_lists;
   froxel_light_lists.start_offset = froxel_data.x;
   froxel_light_lists.sphere_light_count = froxel_data.y & 0x3FF;
   froxel_light_lists.spot_light_count = (froxel_data.y >> 10) & 0x3FF;
   froxel_light_lists.rectangle_light_count = (froxel_data.y >> 20) & 0x3FF;

   return froxel_light_lists;
}
