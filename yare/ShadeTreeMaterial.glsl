~~~~~~~~~~~~~~~~~~~ VertexShader ~~~~~~~~~~~~~~~~~~~~~
#version 450
#include "glsl_binding_defines.h"
%s
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
#ifdef USE_UV
layout(location=2) in vec2 uv;
#endif
#ifdef USE_NORMAL_MAPPING
layout(location = 3) in vec3 tangent;
#endif

out vec3 attr_position;
out vec3 attr_normal;
#ifdef USE_UV
out vec2 attr_uv;
#endif
#ifdef USE_NORMAL_MAPPING
out vec3 attr_tangent;
#endif

#include "surface_uniforms.glsl"

void main()
{
  gl_Position =  matrix_view_local * vec4(position, 1.0);
  attr_normal = mat3(normal_matrix_world_local)*normal;
  attr_position = matrix_world_local*vec4(position, 1.0);
#ifdef USE_UV
  attr_uv =  uv;
#endif
#ifdef USE_NORMAL_MAPPING
  attr_tangent = mat3(normal_matrix_world_local)*tangent;
#endif
}

~~~~~~~~~~~~~~~~~~ FragmentShader ~~~~~~~~~~~~~~~~~~~~~~
#version 450
#include "glsl_binding_defines.h"
%s
#include "scene_uniforms.glsl"
#include "common.glsl"

in vec3 attr_position;
in vec3 attr_normal;
#ifdef USE_UV
in vec2 attr_uv;
#endif
#ifdef USE_NORMAL_MAPPING
in vec3 attr_tangent;
#endif

layout(location = 0, index = 0) out vec4 shading_result;
layout(location = 0, index = 1) out vec4 shading_result_transp_factor;
%s

vec3 normal = normalize(attr_normal);
#ifdef USE_NORMAL_MAPPING
vec3 tangent = normalize(attr_tangent);
#endif

float vec3ToFloat(vec3 v)
{
   return (v.x + v.y + v.z) / 3.0;
}

float rectangleSolidAngle(vec3 p0, vec3 p1, vec3 p2, vec3 p3)
{
   vec3 v0 = p0 - attr_position;
   vec3 v1 = p1 - attr_position;
   vec3 v2 = p2 - attr_position;
   vec3 v3 = p3 - attr_position;

   vec3 n0 = normalize(cross(v0, v1));
   vec3 n1 = normalize(cross(v1, v2));
   vec3 n2 = normalize(cross(v2, v3));
   vec3 n3 = normalize(cross(v3, v0));

   float g0 = acos(dot(-n0, n1));
   float g1 = acos(dot(-n1, n2));
   float g2 = acos(dot(-n2, n3));
   float g3 = acos(dot(-n3, n0));

   return g0 + g1 + g2 + g3 - 2.0 * PI;
}

/*float rectangleSolidAngle(vec3 light_pos, vec2 rec_size, vec3 plane_dir_x, vec3 plane_dir_y)
{
   vec3 pos_in_light_basis = mat4x3(plane_dir_x), plane_dir_y, cross(plane_dir_x, plane_dir_y), light_pos) * vec4(attr_position,1.0);
   asin()
   return g0 + g1 + g2 + g3 - 2.0 * PI;
}*/

float rectangleLightIrradiance(vec3 light_pos, vec2 rec_size, vec3 plane_dir_x, vec3 plane_dir_y)
{   
   vec3 light_dir = normalize(light_pos - attr_position);
   vec3 light_plane_normal = cross(plane_dir_x, plane_dir_y);
   
   float irradiance = 0.0;
   if (dot(light_dir, light_plane_normal) > 0)
   {
      float half_width = rec_size.x * 0.5;
      float half_height = rec_size.y * 0.5;
            
      vec3 p0 = light_pos + plane_dir_x * -half_width + plane_dir_y *  half_height;
      vec3 p1 = light_pos + plane_dir_x * -half_width + plane_dir_y * -half_height;
      vec3 p2 = light_pos + plane_dir_x *  half_width + plane_dir_y * -half_height;
      vec3 p3 = light_pos + plane_dir_x *  half_width + plane_dir_y *  half_height;

      // sample light contribution for rectangle corners and rectangle center
      irradiance += saturate(dot(normalize(p0 - attr_position), normal)); // saturate is used to keep samples above the lamp horizon
      irradiance += saturate(dot(normalize(p1 - attr_position), normal));
      irradiance += saturate(dot(normalize(p2 - attr_position), normal));
      irradiance += saturate(dot(normalize(p3 - attr_position), normal));
      irradiance += saturate(dot(light_dir, normal));

      irradiance *= rectangleSolidAngle(p0, p1, p2, p3) * 0.2;

      //irradiance = rectangleSolidAngle(p0, p1, p2, p3) * (dot(light_dir, normal));
   }
   return irradiance;
}

// from cycles: they do some weird shit in here
float spotLightAttenuation(vec3 light_direction, float cos_spot_max_angle, vec3 spot_direction,  float spot_smooth)
{
   float cos_angle = dot(light_direction, spot_direction);

   float attenuation = cos_angle;
   
   if (cos_angle <= cos_spot_max_angle)
   {
      attenuation = 0.0;
   }
   else 
   {      
      /*float t = attenuation - cos_spot_max_angle;
      if (t < spot_smooth && spot_smooth != 0.0f)
      {
         t = t / spot_smooth;
         attenuation *= t * t * (3.0 - 2.0 * t);//smoothstep(0.0, 1.0, t / spot_smooth);
      }*/
      float t = (acos(cos_angle))/ acos(cos_spot_max_angle);
      if (t > spot_smooth)
      {
         t = t / spot_smooth;
         attenuation = smoothstep(1.0, 0.0, t);
      }
         attenuation = 1.0;
      
   }
   
   return attenuation;
}

vec3 pointLightIncidentRadiance(vec3 light_power, vec3 light_position)
{
   float light_distance = distance(light_position, attr_position);   
   return light_power / (4 * PI*light_distance*light_distance);
}

vec3 evalDiffuseBSDF(vec3 color, vec3 normal)
{
   vec3 irradiance = vec3(0.0);
   for (int i = 0; i < lights.length(); ++i)
   {
      Light light = lights[i];
      if (light.type == LIGHT_SPHERE)
      {
         vec3 light_position = light.data[0].xyz;
         vec3 light_dir = normalize(light_position - attr_position);         
         irradiance += max(dot(light_dir, normal), 0.0) * pointLightIncidentRadiance(light.color, light_position);
      }
      else if (light.type == LIGHT_SUN)
      {
         vec3 light_dir = light.data[0].xyz;
         irradiance += max(dot(light_dir, normal), 0.0) * light.color;
      }
      else if (light.type == LIGHT_RECTANGLE)
      {
         vec2 light_size = vec2(light.data[1].w, light.data[2].w);
         irradiance += rectangleLightIrradiance(light.data[0].xyz, light_size, light.data[1].xyz, light.data[2].xyz) * light.color;
      }
      else if (light.type == LIGHT_SPOT)
      {
         vec3 light_position = light.data[0].xyz;
         vec3 light_dir = normalize(light_position - attr_position);
         float spot_attenuation = spotLightAttenuation(light_dir, light.data[0].w, light.data[1].xyz, light.data[1].w);
         
         // we consider the interior of flashlight as if made of black albedo material.
         // This means that not all the light source power is redirected to the light cone, some is lost in the directions outside of the cone.
         irradiance += max(dot(light_dir, normal), 0.0) * pointLightIncidentRadiance(light.color, light_position) * spot_attenuation;
      }
   }
   vec3 env_irradiance = texture(sky_diffuse_cubemap, normal).rgb;
   return (irradiance /*+ env_irradiance*/) * color/PI; // color/PI is the diffuse brdf constant value
}

float DFactor(float alpha, float NoH)
{
   float alpha2 = SQR(alpha);
   return alpha2 / (PI*SQR(SQR(NoH)*(alpha2 - 1) + 1));
}

float GGGX(float alpha, float NoX)
{
   float alpha2 = SQR(alpha);
   return 2* (NoX * NoX) / ((NoX * NoX) + sqrt(1 + alpha2 * (1 - NoX * NoX)));
}

float GSchlick(float alpha, float NoX)
{
   float k = alpha / 2.0;
   //float k = alpha*sqrt(2/PI);
   return NoX / (NoX*(1 - k) + k);
}

float GSmithFactor(float alpha, float NoV, float NoL)
{
   return GGGX(alpha, NoV) * GGGX(alpha, NoL);
}

float evalMicrofacetGGX(float alpha, vec3 N, vec3 V, vec3 L)
{
   vec3 H = normalize(L+V); // half vector
   float NoV = saturate(dot(N, V));
   float NoL = saturate(dot(N, L));
   float HoV = saturate(dot(H, V));
   float NoH = saturate(dot(N, H));

   return DFactor(alpha, NoH) * GSmithFactor(alpha, NoV, NoL)/ (4.0*NoL*NoV) * NoL;
}

vec3 evalGlossyBSDF(vec3 color, vec3 normal)
{
   float roughness = 0.5;
   vec3 exit_radiance = vec3(0.0);
   vec3 view_vector = normalize(eye_position - attr_position);

   for (int i = 0; i < lights.length(); ++i)
   {
      Light light = lights[i];
      if (light.type == LIGHT_SPHERE)
      {
         vec3 light_position = light.data[0].xyz;
         vec3 light_vector = normalize(light_position - attr_position);         
         
         exit_radiance += evalMicrofacetGGX(roughness, normal, view_vector, light_vector) * pointLightIncidentRadiance(light.color, light_position);
      }
      else if (light.type == LIGHT_SUN)
      {
         vec3 light_vector = light.data[0].xyz;
         exit_radiance += evalMicrofacetGGX(roughness, normal, view_vector, light_vector) * light.color;
      }
      else if (light.type == LIGHT_SPOT)
      {
         vec3 light_position = light.data[0].xyz;
         vec3 light_vector = normalize(light_position - attr_position);         
         float spot_attenuation = spotLightAttenuation(light_vector, light.data[0].w, light.data[1].xyz, light.data[1].w);
         
         exit_radiance += evalMicrofacetGGX(roughness, normal, view_vector, light_vector) * pointLightIncidentRadiance(light.color, light_position) * spot_attenuation;
      }
   }   
     
   return exit_radiance;// texture(sky_cubemap, reflected_vector).rgb;
}

vec3 surfaceGradient(vec3 normal, vec3 dPdx, vec3 dPdy, float dHdx, float dHdy)
{
   vec3 tangentX = cross(dPdy, normal);
   vec3 tangentY = cross(normal, dPdx);
   
   return (tangentX * dHdx + tangentY * dHdy) / dot(dPdx, tangentX);
}

vec3 evalBump(bool invert, float strength, float max_displacement_distance, float heightmap_value, vec2 heightmap_value_differentials, vec3 normal)
{
   vec3 dPdx = dFdxFine(attr_position);
   vec3 dPdy = dFdyFine(attr_position);

   float dHdx = heightmap_value_differentials.x - heightmap_value;
   float dHdy = heightmap_value_differentials.y - heightmap_value;
   //float dHdx = dFdxFine(heightmap_value);
   //float dHdy = dFdyFine(heightmap_value);

   if (invert)
      max_displacement_distance *= -1.0;
   
   float det = dot(dPdx, cross(dPdy, normal));
   float absdet = abs(det);

   vec3 disturbed_normal = normalize(/*absdet**/normal - /*sign(det)**/max_displacement_distance* surfaceGradient(normal, dPdx, dPdy, dHdx, dHdy));
   return  mix(normal, disturbed_normal, strength);
}

#ifdef USE_NORMAL_MAPPING
vec3 evalNormalMap(vec3 normal_color, float scale)
{
   //vec3 fixed_tangent = normalize(tangent - dot(tangent, normal) * normal);
   vec3 bitangent = cross(tangent, normal);  
   vec3 normal_in_tangent_space = 2.0*normal_color - vec3(1.0);
   normal_in_tangent_space.x *= scale;
   normal_in_tangent_space.y *= scale;

   mat3 TBN = mat3(tangent, bitangent, normal);
   vec3 new_normal = TBN * normal_in_tangent_space;   
   return new_normal;
}
#endif

void sampleTexture(sampler2D tex, vec3 uvw, mat3x2 transform, out vec3 color, out float alpha)
{
   vec2 uv = transform*vec3(uvw.xy, 1.0);
	vec4 tex_sample = texture(tex, uv);
	color=tex_sample.rgb;
   alpha = tex_sample.a;
}

void sampleTextureDifferentials(sampler2D tex,                                
                                vec3 uvw,
                                mat3x2 transform,
                                out vec3 color,
                                out vec3 color_at_dx,
                                out vec3 color_at_dy,
                                out float alpha,
                                out float alpha_at_dx,
                                out float alpha_at_dy)
{
   vec2 uv = transform*vec3(uvw.xy, 1.0);
   vec4 tex_sample = texture(tex, uv);
   color = tex_sample.rgb;
   alpha = tex_sample.a;

   /*float inv_tex_size = 1.0 / textureSize(tex, 0).x;
   vec2 dUVdx = dFdx(uv);
   if (length(dUVdx) < inv_tex_size)
      dUVdx = dUVdx / length(dUVdx)*inv_tex_size;

   vec2 dUVdy = dFdy(uv);
   if (length(dUVdy) < inv_tex_size)
      dUVdy = dUVdy / length(dUVdy)*inv_tex_size;*/
   vec2 dUVdx = dFdx(uv);
   vec2 dUVdy = dFdy(uv);
   tex_sample = texture(tex, uv + dUVdx);
   color_at_dx = tex_sample.rgb;
   alpha_at_dx = tex_sample.a;

   tex_sample = texture(tex, uv + dUVdy);
   color_at_dy = tex_sample.rgb;
   alpha_at_dy = tex_sample.a;
}

 void main()
 {    
    %s
    //shading_result.rgb = lights[0].data[0].xyz;
 }