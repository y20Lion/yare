
#define PI 3.1415926535897932384626433832795

vec3 faceToDirection(int face, vec2 uv)
{
   switch (face)
   {
   case 0: return vec3(1.0, -uv.y, -uv.x);
   case 1: return vec3(-1.0, -uv.y, uv.x);
   case 2: return vec3(uv.x, 1.0, uv.y);
   case 3: return vec3(uv.x, -1.0, -uv.y);
   case 4: return vec3(uv.x, -uv.y, 1.0);
   case 5: return vec3(-uv.x, -uv.y, -1.0);
   default: return vec3(0.0);
   }
}

vec2 directionToSphericalCoordinates(vec3 dir)
{
   float theta = acos(dir.z);
   float phi = atan(dir.y, dir.x);
   return vec2(phi, theta);
}

float areaElement(float x, float y)
{
   return atan(x*y, sqrt(x*x + y*y + 1));
}

float texelSolidAngle(int x, int y, int face_size)
{
   float inv_resolution = 1.0 / float(face_size);
   vec2 uv01 = (vec2(x, y) + vec2(0.5))*inv_resolution;
   // uv is in range [-1, 1]
   vec2 uv = 2.0*uv01 - vec2(1.0);

   // get projected area for this texel
   float x0 = uv.x - inv_resolution;
   float y0 = uv.y - inv_resolution;
   float x1 = uv.x + inv_resolution;
   float y1 = uv.y + inv_resolution;
   float solid_angle = areaElement(x0, y0) - areaElement(x0, y1) - areaElement(x1, y0) + areaElement(x1, y1);
   return solid_angle;
}

float Klm[9] = 
{
   0.5*sqrt(1.0 / PI),
   -sqrt(3.0 / (4.0*PI)), sqrt(3.0 / (4.0*PI)), -sqrt(3.0 / (4.0*PI)),
   0.5*sqrt(15.0 / (PI)), -0.5*sqrt(15.0 / (PI)), 0.25*sqrt(5.0 / PI), -0.5*sqrt(15.0 / PI), 0.25*sqrt(15 / PI)
};

float legendre_coeff[9][10] = {
//1  x  y  z    x^2 y^2 z^2 yz xz xy
{ 1, 0, 0, 0,    0, 0, 0,   0, 0, 0 },

{ 0, 0, 1, 0,    0, 0, 0,   0, 0, 0 },
{ 0, 0, 0, 1,    0, 0, 0,   0, 0, 0 },
{ 0, 1, 0, 0,    0, 0, 0,   0, 0, 0 },

{ 0, 0, 0, 0,    0, 0, 0,   0, 0, 1 },
{ 0, 0, 0, 0,    0, 0, 0,   1, 0, 0 },
{ -1, 0, 0, 0,    0, 0, 3,   0, 0, 0 },
{ 0, 0, 0, 0,    0, 0, 0,   0, 1, 0 },
{ 0, 0, 0, 0,    1, -1, 0,   0, 0, 0 } };

float cos_sph_harmonics_coeffs[9] = 
{
   PI,
   2.0f * PI / 3.0f, 2.0f * PI / 3.0f, 2.0f * PI / 3.0f,
   PI / 4.0f, PI / 4.0f, PI / 4.0f, PI / 4.0f, PI / 4.0f
};