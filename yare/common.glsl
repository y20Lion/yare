
#define PI 3.1415926535897932384626433832795

float luminance(vec3 color)
{
   return dot(vec3(0.3f, 0.59f, 0.11f), color);
}

#define saturate(v) clamp(v, 0.0, 1.0)

#define LIGHT_SPHERE    0
#define LIGHT_RECTANGLE 1
#define LIGHT_SUN       2
#define LIGHT_SPOT      3

vec3 linearToSrgb(vec3 linear_color)
{
   vec3 result_lo = linear_color * 12.92;
   vec3 result_hi = (pow(abs(linear_color), vec3(1.0 / 2.4)) * 1.055) - 0.055;
   vec3 result;
   result.x = linear_color.x <= 0.0031308 ? result_lo.x : result_hi.x;
   result.y = linear_color.y <= 0.0031308 ? result_lo.y : result_hi.y;
   result.z = linear_color.z <= 0.0031308 ? result_lo.z : result_hi.z;
   return result;
}

vec3 srgbToLinear(vec3 srgb_color)
{
   vec3 result_lo = srgb_color / 12.92;
   vec3 result_hi = pow((srgb_color + 0.055) / 1.055, vec3(2.4));
   vec3 result;
   result.x = (srgb_color.x <= 0.04045) ? result_lo.x : result_hi.x;
   result.y = (srgb_color.y <= 0.04045) ? result_lo.y : result_hi.y;
   result.z = (srgb_color.z <= 0.04045) ? result_lo.z : result_hi.z;
   return result;
 }
