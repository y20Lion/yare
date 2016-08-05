~~~~~~~~~~~~~~~~~~~ VertexShader ~~~~~~~~~~~~~~~~~~~~~
#include "glsl_global_defines.h"
%s
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
#ifdef USE_UV
layout(location=2) in vec2 uv;
#endif
#ifdef USE_NORMAL_MAPPING
layout(location = 3) in vec3 tangent;
#endif

#ifdef USE_SKINNING
layout(location = 4) in uvec4 bone_index;
layout(location = 5) in vec4 bone_weight;
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
#include "scene_uniforms.glsl"
#ifdef USE_SKINNING
layout(std430, binding = BI_SKINNING_PALETTE_SSBO) buffer SkinningPaletteSSBO
{
   mat4x3 skinning_matrix[];
};
#endif
void main()
{
#ifdef USE_SKINNING
   vec3 pos_world = matrix_world_local*vec4(position, 1.0);
   vec3 normal_world = mat3(normal_matrix_world_local)*normal;
   
   vec3 skinned_position = vec3(0);
   vec3 skinned_normal = vec3(0);
   for (int i = 0; i < 4; ++i)
   {      
      skinned_position += bone_weight[i] * (skinning_matrix[bone_index[i]]* vec4(pos_world, 1.0));
      skinned_normal += bone_weight[i] * (mat3(skinning_matrix[bone_index[i]]) * normal_world);
   }

   gl_Position = matrix_proj_world * vec4(skinned_position, 1.0);
   attr_normal = skinned_normal;
   attr_position = skinned_position;
#else 
   gl_Position = matrix_proj_local * vec4(position, 1.0);
   attr_normal = mat3(normal_matrix_world_local)*normal;
   attr_position = matrix_world_local*vec4(position, 1.0);
#endif
#ifdef USE_UV
   attr_uv =  uv;
#endif
#ifdef USE_NORMAL_MAPPING
   attr_tangent = mat3(normal_matrix_world_local)*tangent;
#endif
}

~~~~~~~~~~~~~~~~~~ FragmentShader ~~~~~~~~~~~~~~~~~~~~~~
#include "glsl_global_defines.h"
%s
#include "scene_uniforms.glsl"
#include "common.glsl"
#include "common_node_mix.glsl"

layout(std430, binding = BI_HAMMERSLEY_SAMPLES_SSBO) buffer HammersleySamples
{   
   vec2 hammersley_samples[];
};

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

vec3 normal = gl_FrontFacing ? normalize(attr_normal) : -normalize(attr_normal);
#ifdef USE_NORMAL_MAPPING
vec3 tangent = normalize(attr_tangent);
#endif
vec3 view_vector = normalize(eye_position - attr_position);
float ssao = texelFetch(ssao_texture, ivec2(gl_FragCoord.xy), 0).r;

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

   float gamma0 = acos(dot(-n0, n1));
   float gamma1 = acos(dot(-n1, n2));
   float gamma2 = acos(dot(-n2, n3));
   float gamma3 = acos(dot(-n3, n0));

   return gamma0 + gamma1 + gamma2 + gamma3 - 2.0 * PI;
}

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

      //irradiance *= rectangleSolidAngle(p0, p1, p2, p3) * 0.2;
      irradiance = rectangleSolidAngle(p0, p1, p2, p3) * (dot(light_dir, normal));
   }
   return irradiance;
}

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
   vec3 env_irradiance = texture(sky_diffuse_cubemap, normal).rgb*ssao;
   return  (irradiance + env_irradiance) * color / PI; // color/PI is the diffuse brdf constant value
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
   return NoX / (NoX*(1 - k) + k);
}

float GSmithFactor(float alpha, float NoV, float NoL)
{
   return GSchlick(alpha, NoV) * GSchlick(alpha, NoL);
}

float evalMicrofacetGGX(float alpha, vec3 N, vec3 V, vec3 L)
{
   vec3 H = normalize(L+V); // half vector
   float NoV = saturate(dot(N, V));
   float NoL = saturate(dot(N, L));
   float HoV = saturate(dot(H, V));
   float NoH = saturate(dot(N, H));

   return DFactor(alpha, NoH) * GSmithFactor(alpha, NoV, NoL)/ (4.0*NoV); 
   // The NoL at the denominator is cancelled out by the cos factor when evaluation the brdf
}

vec3 importanceSampleDirGGX(vec2 Xi, float Roughness, vec3 N)
{
   float a = Roughness;
   float Phi = 2 * PI * Xi.x;
   float CosTheta = sqrt((1 - Xi.y) / (1 + (a*a - 1) * Xi.y));
   float SinTheta = sqrt(1 - CosTheta * CosTheta);
   vec3 H;
   H.x = SinTheta * cos(Phi);
   H.y = SinTheta * sin(Phi);
   H.z = CosTheta;
   vec3 UpVector = abs(N.z) < 0.999 ? vec3(0, 0, 1) : vec3(1, 0, 0);
   vec3 TangentX = normalize(cross(UpVector, N));
   vec3 TangentY = cross(N, TangentX);
   // Tangent to world space
   return TangentX * H.x + TangentY * H.y + N * H.z;
}

float rand(vec2 co)
{
   return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

vec3 importanceSampleSkyCubemap(float roughness, vec3 N, vec3 V)
{
   const int num_samples = 32;
   
   float width = textureSize(sky_cubemap, 0).x;
   float mip_count = log2(width)+1;

   vec3 env_radiance = vec3(0);
   for (int i = 0; i < num_samples; ++i)
   {      
      vec2 sample2D = hammersley_samples[i];

      vec3 H = importanceSampleDirGGX(sample2D, roughness, N);
      vec3 L = 2 * dot(V, H) * H - V;
      float NoV = dot(N, V);
      float NoL = dot(N, L);
      float NoH = dot(N, H);
      float VoH = dot(V, H);

      if (NoL > 0)
      {
         float pdf = DFactor(roughness, NoH) * NoH / (4 * VoH);
         float omegaS = 1.0 / (num_samples * pdf);
         float omegaP = 4.0 * PI / (6.0 * width * width);
         float mip_level = clamp(0.5 * log2(omegaS / omegaP)/*+1*/, 0, mip_count);
                     
         vec3 sample_color = textureLod(sky_cubemap, L, mip_level).rgb;
         float G = GSmithFactor(roughness, NoV, NoL);
         
         // Incident light = sample_color * NoL
         // Microfacet specular = D*G*F*NoL / (4*NoL*NoV)
         // pdf = D * NoH / (4 * VoH)
         //SpecularLighting += sample_color * F * G * VoH / (NoH * NoV);
         env_radiance += sample_color*G * VoH / (NoH * NoV);
      }
   }   

   return env_radiance / num_samples;
}

vec3 evalGlossyBSDF(vec3 color, vec3 normal, float roughness)
{
   roughness = max(roughness, 0.001);
   vec3 exit_radiance = vec3(0.0);

   for (int i = 0; i < lights.length(); ++i)
   {
      Light light = lights[i];
      if (light.type == LIGHT_SPHERE)
      {
         vec3 light_position = light.data[0].xyz;
         float light_size = light.data[0].w;
         vec3 light_vector = light_position - attr_position;

         vec3 reflection_vector = reflect(-view_vector, normal);
         vec3 center_to_ray =  dot(light_vector, reflection_vector)*reflection_vector - light_vector;
         vec3 closest_point = light_vector + center_to_ray * saturate(light_size / length(center_to_ray));
         vec3 light_dir = normalize(closest_point);
         float alpha_prime = saturate(roughness+light_size / (length(light_vector) * 3.0));
         float renormalization_factor = (roughness*roughness) / (alpha_prime*alpha_prime);

         exit_radiance += renormalization_factor* evalMicrofacetGGX(roughness, normal, view_vector, light_dir) * pointLightIncidentRadiance(light.color, closest_point+ attr_position);
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
     
   vec3 sky_radiance = importanceSampleSkyCubemap(roughness, normal, view_vector)*ssao;

   return color*(exit_radiance+sky_radiance);
}

vec3 evalEmissionBSDF(vec3 color, float strength)
{
   return color*strength;
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
	color=srgbToLinear(tex_sample.rgb);//TODO not for bump and normal
   alpha=tex_sample.a;
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

float evalFresnel(float ior, vec3 normal)
{
   return fresnel(ior, dot(normal, view_vector));
}

vec3 evalCurveRgb(vec3 color, float fac, sampler1D red_curve, sampler1D green_curve, sampler1D blue_curve)
{
   float min_x = 0.0;
   float max_x = 1.0;
   color = (color - vec3(min_x)) / (max_x - min_x);
   
   vec3 result;
   result.r = texture(red_curve, color.r).x;
   result.g = texture(green_curve, color.g).x;
   result.b = texture(blue_curve, color.b).x;
   return mix(color, result, fac);
}

void evalLayerWeight(float blend, vec3 normal, out float out_fresnel, out float out_facing)
{
   float cosi = dot(view_vector, normal);

   float eta = max(1.0 - blend, 1e-5);
   eta = gl_FrontFacing ? 1.0 / eta : eta;
   out_fresnel = fresnel(eta, cosi);

   out_facing = abs(cosi);
   if (blend != 0.5) 
   {
      blend = clamp(blend, 0.0, 1.0 - 1e-5);
      blend = (blend < 0.5) ? 2.0 * blend : 0.5 / (1.0 - blend);
      out_facing = pow(out_facing, blend);
   }
   out_facing = 1.0 - out_facing;
}

 void main()
 {    
    %s
       //shading_result.rgb = vec3(fresnel(1.45, dot(normal, normalize(eye_position - attr_position))));
       //shading_result.rgb = vec3(GroupFresnel_Node_Fac- fresnel(1.45, dot(normal, normalize(eye_position - attr_position))));
       //shading_result.rgb = vec3(Fresnel_Node_Fac);
    //shading_result_transp_factor.xyzw = vec4(0.0);

    /*if (all(greaterThan(shading_result_transp_factor.xyz,vec3(0.99))))
       discard;*/ // TODO add

       //shading_result.rgb = vec3(texelFetch(ssao_texture, ivec2(gl_FragCoord.xy), 0).r/10.0);
       //shading_result.rgb = vec3((texelFetch(ssao_texture, ivec2(gl_FragCoord.xy), 0).r));
 }