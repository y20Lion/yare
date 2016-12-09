// TODO refactor with material shader

float spotLightAttenuation(vec3 light_direction, float cos_spot_max_angle, vec3 spot_direction, float spot_smooth)
{
   float cos_angle = dot(light_direction, spot_direction);
   float cos_angle_start_smooth = (1.0 - cos_spot_max_angle)*spot_smooth + cos_spot_max_angle;

   return smoothstep(cos_spot_max_angle, cos_angle_start_smooth, cos_angle);  
}

vec3 sphereLightIncidentRadiance(SphereLight light, vec3 shade_position, float length)
{
   vec3 light_dir = (light.position - shade_position);
   float light_distance_sqr = max(dot(light_dir, light_dir), (length*length*0.5));
   float rcp_light_distance_sqr = 1.0f / light_distance_sqr;
   light_dir *= sqrt(rcp_light_distance_sqr);

   float restrict_influence_factor = pow(saturate(1.0 - pow(light_distance_sqr / (light.radius*light.radius), 2.0)), 2.0);
   return light.color / (4 * PI) * rcp_light_distance_sqr * restrict_influence_factor;
}

vec3 spotLightIncidentRadiance(SpotLight light, vec3 shade_position, float length)
{
   vec3 light_dir = (light.position - shade_position);
   float light_distance_sqr = max(dot(light_dir, light_dir), (length*length*0.5));
   float rcp_light_distance_sqr = 1.0f / light_distance_sqr;
   light_dir *= sqrt(rcp_light_distance_sqr);

   float spot_attenuation =  spotLightAttenuation(light_dir, light.cos_half_angle, light.direction, light.angle_smooth);
   spot_attenuation *= smoothstep(0.0, 1.0, light_distance_sqr);

   float restrict_influence_factor = pow(saturate(1.0 - pow(light_distance_sqr / (light.radius*light.radius), 2.0)), 2.0);
   return light.color / (4 * PI) * rcp_light_distance_sqr * restrict_influence_factor * spot_attenuation;
   
}
