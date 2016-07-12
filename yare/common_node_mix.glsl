
vec3 node_mix_blend(float t, vec3 col1, vec3 col2)
{
   return mix(col1, col2, t);
}

vec3 node_mix_add(float t, vec3 col1, vec3 col2)
{
   return mix(col1, col1 + col2, t);
}

vec3 node_mix_mul(float t, vec3 col1, vec3 col2)
{
   return mix(col1, col1 * col2, t);
}

vec3 node_mix_screen(float t, vec3 col1, vec3 col2)
{
   float tm = 1.0 - t;

   return vec3(1.0) - (vec3(tm) + t * (vec3(1.0) - col2)) * (vec3(1.0) - col1);
}

vec3 node_mix_overlay(float t, vec3 col1, vec3 col2)
{
   float tm = 1.0 - t;

   vec3 outcol = col1;

   if (outcol.x < 0.5)
      outcol.x *= tm + 2.0 * t * col2.x;
   else
      outcol.r = 1.0 - (tm + 2.0 * t * (1.0 - col2.r)) * (1.0 - outcol.r);

   if (outcol.g < 0.5)
      outcol.g *= tm + 2.0 * t * col2.g;
   else
      outcol.g = 1.0 - (tm + 2.0 * t * (1.0 - col2.g)) * (1.0 - outcol.g);

   if (outcol.b < 0.5)
      outcol.b *= tm + 2.0 * t * col2.b;
   else
      outcol.b = 1.0 - (tm + 2.0 * t * (1.0 - col2.b)) * (1.0 - outcol.b);

   return outcol;
}

vec3 node_mix_sub(float t, vec3 col1, vec3 col2)
{
   return mix(col1, col1 - col2, t);
}

vec3 node_mix_div(float t, vec3 col1, vec3 col2)
{
   float tm = 1.0 - t;

   vec3 outcol = col1;

   if (col2.r != 0.0) outcol.r = tm * outcol.r + t * outcol.r / col2.r;
   if (col2.g != 0.0) outcol.g = tm * outcol.g + t * outcol.g / col2.g;
   if (col2.b != 0.0) outcol.b = tm * outcol.b + t * outcol.b / col2.b;

   return outcol;
}

vec3 node_mix_diff(float t, vec3 col1, vec3 col2)
{
   return mix(col1, abs(col1 - col2), t);
}

vec3 node_mix_dark(float t, vec3 col1, vec3 col2)
{
   return min(col1, col2) * t + col1 * (1.0 - t);
}

vec3 node_mix_light(float t, vec3 col1, vec3 col2)
{
   return max(col1, col2 * t);
}

vec3 node_mix_dodge(float t, vec3 col1, vec3 col2)
{
   vec3 outcol = col1;

   if (outcol.r != 0.0) {
      float tmp = 1.0 - t * col2.r;
      if (tmp <= 0.0)
         outcol.r = 1.0;
      else if ((tmp = outcol.r / tmp) > 1.0)
         outcol.r = 1.0;
      else
         outcol.r = tmp;
   }
   if (outcol.g != 0.0) {
      float tmp = 1.0 - t * col2.g;
      if (tmp <= 0.0)
         outcol.g = 1.0;
      else if ((tmp = outcol.g / tmp) > 1.0)
         outcol.g = 1.0;
      else
         outcol.g = tmp;
   }
   if (outcol.b != 0.0) {
      float tmp = 1.0 - t * col2.b;
      if (tmp <= 0.0)
         outcol.b = 1.0;
      else if ((tmp = outcol.b / tmp) > 1.0)
         outcol.b = 1.0;
      else
         outcol.b = tmp;
   }

   return outcol;
}

vec3 node_mix_burn(float t, vec3 col1, vec3 col2)
{
   float tmp, tm = 1.0 - t;

   vec3 outcol = col1;

   tmp = tm + t * col2.r;
   if (tmp <= 0.0)
      outcol.r = 0.0;
   else if ((tmp = (1.0 - (1.0 - outcol.r) / tmp)) < 0.0)
      outcol.r = 0.0;
   else if (tmp > 1.0)
      outcol.r = 1.0;
   else
      outcol.r = tmp;

   tmp = tm + t * col2.g;
   if (tmp <= 0.0)
      outcol.g = 0.0;
   else if ((tmp = (1.0 - (1.0 - outcol.g) / tmp)) < 0.0)
      outcol.g = 0.0;
   else if (tmp > 1.0)
      outcol.g = 1.0;
   else
      outcol.g = tmp;

   tmp = tm + t * col2.b;
   if (tmp <= 0.0)
      outcol.b = 0.0;
   else if ((tmp = (1.0 - (1.0 - outcol.b) / tmp)) < 0.0)
      outcol.b = 0.0;
   else if (tmp > 1.0)
      outcol.b = 1.0;
   else
      outcol.b = tmp;

   return outcol;
}

vec3 node_mix_hue(float t, vec3 col1, vec3 col2)
{
   vec3 outcol = col1;
   vec3 hsv2 = rgbToHsv(col2);

   if (hsv2[1] != 0.0) {
      vec3 hsv = rgbToHsv(outcol);
      hsv[0] = hsv2[0];
      vec3 tmp = hsvToRgb(hsv);

      outcol = mix(outcol, tmp, t);
   }

   return outcol;
}

vec3 node_mix_sat(float t, vec3 col1, vec3 col2)
{
   float tm = 1.0 - t;

   vec3 outcol = col1;

   vec3 hsv = rgbToHsv(outcol);

   if (hsv.g != 0.0) {
      vec3 hsv2 = rgbToHsv(col2);

      hsv.g = tm * hsv[1] + t * hsv2[1];
      outcol = hsvToRgb(hsv);
   }

   return outcol;
}

vec3 node_mix_val(float t, vec3 col1, vec3 col2)
{
   float tm = 1.0 - t;

   vec3 hsv = rgbToHsv(col1);
   vec3 hsv2 = rgbToHsv(col2);

   hsv[2] = tm * hsv[2] + t * hsv2[2];

   return hsvToRgb(hsv);
}

vec3 node_mix_color(float t, vec3 col1, vec3 col2)
{
   vec3 outcol = col1;
   vec3 hsv2 = rgbToHsv(col2);

   if (hsv2[1] != 0.0) {
      vec3 hsv = rgbToHsv(outcol);
      hsv[0] = hsv2[0];
      hsv[1] = hsv2[1];
      vec3 tmp = hsvToRgb(hsv);

      outcol = mix(outcol, tmp, t);
   }

   return outcol;
}

vec3 node_mix_soft(float t, vec3 col1, vec3 col2)
{
   float tm = 1.0 - t;

   vec3 one = vec3(1.0);
   vec3 scr = one - (one - col2) * (one - col1);

   return tm * col1 + t * ((one - col1) * col2 * col1 + col1 * scr);
}

vec3 node_mix_linear(float t, vec3 col1, vec3 col2)
{
   vec3 outcol = col1;

   if (col2.r > 0.5)
      outcol.r = col1.r + t * (2.0 * (col2.r - 0.5));
   else
      outcol.x = col1.x + t * (2.0 * (col2.x) - 1.0);

   if (col2.g > 0.5)
      outcol.g = col1.g + t * (2.0 * (col2.g - 0.5));
   else
      outcol.g = col1.g + t * (2.0 * (col2.g) - 1.0);

   if (col2.b > 0.5)
      outcol.b = col1.b + t * (2.0 * (col2.b - 0.5));
   else
      outcol.b = col1.b + t * (2.0 * (col2.b) - 1.0);

   return outcol;
}