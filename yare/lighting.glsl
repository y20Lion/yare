// TODO refactor with material shader

float spotLightAttenuation(vec3 light_direction, float cos_spot_max_angle, vec3 spot_direction, float spot_smooth)
{
   float cos_angle = dot(light_direction, spot_direction);

   float attenuation = cos_angle;

   if (cos_angle <= cos_spot_max_angle) //outside of cone
   {
      attenuation = 0.0;
   }
   else
   {
      spot_smooth = 1.0 - spot_smooth;
      float t = (1.0 - cos_angle) / (1.0 - cos_spot_max_angle);
      if (t - spot_smooth > 0)
      {
         t = saturate((t - spot_smooth) / (1.0 - spot_smooth));
         attenuation = smoothstep(0.0, 1.0, 1.0 - t);
      }
      else
         attenuation = 1.0;
   }

   return attenuation;
}

vec3 pointLightIncidentRadiance(vec3 light_power, vec3 light_position, float light_radius, vec3 shade_position)
{
   float light_distance = distance(light_position, shade_position);
   float restrict_influence_factor = pow(saturate(1.0 - pow(light_distance / light_radius, 4.0)), 2.0);
   return light_power / (4 * PI*light_distance*light_distance) * restrict_influence_factor;
}
