
#define PI 3.1415926535897932384626433832795

float luminance(vec3 color)
{
   return dot(vec3(0.3f, 0.59f, 0.11f), color);
}

#define saturate(v) clamp(v, 0.0, 1.0)