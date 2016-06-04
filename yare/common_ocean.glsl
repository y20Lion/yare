#pragma once

#define PI 3.1415926535897932384626433832795

float hash(vec2 p) 
{
   float h = dot(p, vec2(127.1, 311.7));
   return fract(sin(h)*43758.5453123);
}
float noise(in vec2 p)
{
   vec2 i = floor(p);
   vec2 f = fract(p);
   vec2 u = f*f*(3.0 - 2.0*f);
   return -1.0 + 2.0*mix(mix(hash(i + vec2(0.0, 0.0)),
      hash(i + vec2(1.0, 0.0)), u.x),
      mix(hash(i + vec2(0.0, 1.0)),
         hash(i + vec2(1.0, 1.0)), u.x), u.y);
}

float waveFunction(vec2 uv, float crest) 
{
   uv += noise(uv);   
   vec2 wv = 1.0 - (sin(uv));   
   return  pow(wv.x*wv.y, crest);   
}

const int ITER_GEOMETRY = 3;
const float SEA_HEIGHT = 2.0;
const float SEA_CHOPPY = 4.0;
const float SEA_SPEED = 0.8;
const float SEA_FREQ = 0.16;
const vec3 SEA_BASE = vec3(0.1, 0.19, 0.22);
const vec3 SEA_WATER_COLOR = vec3(0.8, 0.9, 0.6);
mat2 octave_matrix = mat2(1.6, 1.2, -1.2, 1.6);

float fresnel(float cos_incident_angle)
{
   float n1 = 1.0; //air
   float n2 = 1.33; //water
   float r0 = pow((n1-n2)/(n1+n2), 2.0);

   return r0 + (1.0 - r0)*pow(1 - cos_incident_angle, 5.0);
}

vec3 evalOceanPosition(vec3 position)
{
   float sea_wavelength = 100.0;
   float speed = 2.0;
   float freq = 2 * PI / sea_wavelength;
   float amplitude = 2.0;
   float crest = 0.75;
   vec2 uv = position.xy; uv.x *= 0.6;

   float height = 0.0;
   for (int i = 0; i < ITER_GEOMETRY; i++) 
   {
      float d = waveFunction((uv + speed*time)*freq, crest);
      d += waveFunction((uv+0.3*uv - speed*time)*freq, crest);
      height += d* amplitude;;

      uv *= octave_matrix;
      freq *= 2.0;
      amplitude *= 0.15;
      //crest = mix(crest, 1.0, 0.2);
   }
   
   position.z += height/**2.5*/;
   return position;
}

/*vec3 evalOceanPosition(vec3 position)
{
vec3 result = position;
float lambda = 2.0;
float speed = 0.3;
result.z = 0.5*sin(2 * PI / lambda*(position.x + speed*time)) + 1.0;
result.z += 0.5*sin(2 * PI / lambda*0.3*(position.x - speed*time)) + 1.0;
return result;
}*/
