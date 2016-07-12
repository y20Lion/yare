
#define PI 3.1415926535897932384626433832795

float luminance(vec3 color)
{
   return dot(vec3(0.3f, 0.59f, 0.11f), color);
}

#define saturate(v) clamp(v, 0.0, 1.0)
#define SQR(x) ((x)*(x))

#define LIGHT_SPHERE    0
#define LIGHT_RECTANGLE 1
#define LIGHT_SUN       2
#define LIGHT_SPOT      3

vec3 rgbToHsv(vec3 rgb)
{
   float cmax, cmin, h, s, v, cdelta;
   vec3 c;

   cmax = max(rgb[0], max(rgb[1], rgb[2]));
   cmin = min(rgb[0], min(rgb[1], rgb[2]));
   cdelta = cmax - cmin;

   v = cmax;

   if (cmax != 0.0) 
   {
      s = cdelta / cmax;
   }
   else 
   {
      s = 0.0;
      h = 0.0;
   }

   if (s == 0.0) 
   {
      h = 0.0;
   }
   else 
   {
      c = (vec3(cmax, cmax, cmax) - rgb) / cdelta;

      if (rgb[0] == cmax) h = c[2] - c[1];
      else if (rgb[1] == cmax) h = 2.0 + c[0] - c[2];
      else h = 4.0 + c[1] - c[0];

      h /= 6.0;

      if (h < 0.0)
         h += 1.0;
   }

   return vec3(h, s, v);
}

vec3 hsvToRgb(vec3 hsv)
{
   float i, f, p, q, t, h, s, v;
   vec3 rgb;

   h = hsv[0];
   s = hsv[1];
   v = hsv[2];

   if (s == 0.0)
   {
      rgb = vec3(v, v, v);
   }
   else 
   {
      if (h == 1.0)
         h = 0.0;

      h *= 6.0;
      i = floor(h);
      f = h - i;
      rgb = vec3(f, f, f);
      p = v * (1.0 - s);
      q = v * (1.0 - (s * f));
      t = v * (1.0 - (s * (1.0 - f)));

      if (i == 0.0) rgb = vec3(v, t, p);
      else if (i == 1.0) rgb = vec3(q, v, p);
      else if (i == 2.0) rgb = vec3(p, v, t);
      else if (i == 3.0) rgb = vec3(p, q, v);
      else if (i == 4.0) rgb = vec3(t, p, v);
      else rgb = vec3(v, p, q);
   }

   return rgb;
}

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

float fresnel(float n2, float cos_incident_angle)
{
   float n1 = 1.0; //air   
   float r0 = pow((n1 - n2) / (n1 + n2), 2.0);

   return r0 + (1.0 - r0)*pow(1 - cos_incident_angle, 5.0);
}
