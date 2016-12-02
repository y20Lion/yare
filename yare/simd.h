#pragma once

#include <xmmintrin.h>

namespace yare {


/*****************  simdbool  *********************/

_declspec(align(16))
struct simdbool
{
   simdbool(bool val) : val(_mm_castsi128_ps(_mm_set1_epi32((val ? 0xFFFFFFFF : 0x00000000)))) {}
   simdbool(__m128 val) : val(val) {}

   __m128 val;
};

__forceinline const simdbool operator &(const simdbool& a, const simdbool& b) { return _mm_and_ps(a.val, b.val); }
__forceinline const simdbool operator |(const simdbool& a, const simdbool& b) { return _mm_or_ps(a.val, b.val); }
__forceinline const simdbool operator ^(const simdbool& a, const simdbool& b) { return _mm_xor_ps(a.val, b.val); }

__forceinline const simdbool operator &=(simdbool& a, const simdbool& b) { return a = a & b; }
__forceinline const simdbool operator |=(simdbool& a, const simdbool& b) { return a = a | b; }
__forceinline const simdbool operator ^=(simdbool& a, const simdbool& b) { return a = a ^ b; }


/*****************  simdfloat  *********************/

_declspec(align(16))
struct simdfloat
{
   explicit simdfloat(float scalval) : val(_mm_set1_ps(scalval)) {  }
   simdfloat() {}
   simdfloat(__m128 val) : val(val) {}

   void load(float* values) { val = _mm_load_ps(values); }
   void load(float a, float b, float c, float d) { val = _mm_set_ps(a, b, c, d); }

   __m128 val;
};

__forceinline simdfloat operator *(const simdfloat& a, const simdfloat& b) { return _mm_mul_ps(a.val, b.val); }
__forceinline simdfloat operator -(const simdfloat& a, const simdfloat& b) { return _mm_sub_ps(a.val, b.val); }
__forceinline simdfloat operator +(const simdfloat& a, const simdfloat& b) { return _mm_add_ps(a.val, b.val); }
__forceinline simdfloat operator /(const simdfloat& a, const simdfloat& b) { return _mm_div_ps(a.val, b.val); }

__forceinline const simdfloat operator -(const simdfloat& a) { return _mm_xor_ps(a.val, _mm_castsi128_ps(_mm_set1_epi32(0x80000000))); }

__forceinline const simdbool operator >(const simdfloat& a, const simdfloat& b) { return _mm_cmpgt_ps(a.val, b.val); }
__forceinline const simdbool operator <(const simdfloat& a, const simdfloat& b) { return _mm_cmplt_ps(a.val, b.val); }
__forceinline const simdbool operator >=(const simdfloat& a, const simdfloat& b) { return _mm_cmpge_ps(a.val, b.val); }
__forceinline const simdbool operator <=(const simdfloat& a, const simdfloat& b) { return _mm_cmple_ps(a.val, b.val); }

__forceinline simdfloat abs(const simdfloat& a) { return _mm_and_ps(a.val, _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff))); }
__forceinline simdfloat rcp(const simdfloat& a) { return _mm_rcp_ps(a.val); }

/*****************  simdvec3  *********************/

struct simdvec3
{
   simdvec3() {}
   explicit simdvec3(const simdfloat& f) : x(f), y(f), z(f) {}
   explicit simdvec3(const vec3& v) : x(v.x), y(v.y), z(v.z) {}
   simdvec3(simdfloat x, simdfloat y, simdfloat z) : x(x), y(y), z(z) {}

   void load(float* xvalues, float* yvalues, float* zvalues, int offset) { x.load(xvalues + offset); y.load(yvalues + offset); z.load(zvalues + offset); }

   simdfloat x;
   simdfloat y;
   simdfloat z;
};

__forceinline simdvec3 operator *(const simdfloat& f, const simdvec3& b) { return simdvec3(f * b.x, f * b.y, f * b.z); }

__forceinline simdvec3 operator *(const simdvec3& a, const simdvec3& b) { return simdvec3(a.x * b.x, a.y * b.y, a.z * b.z); }
__forceinline simdvec3 operator -(const simdvec3& a, const simdvec3& b) { return simdvec3(a.x - b.x, a.y - b.y, a.z - b.z); }
__forceinline simdvec3 operator +(const simdvec3& a, const simdvec3& b) { return simdvec3(a.x + b.x, a.y + b.y, a.z + b.z); }
__forceinline simdvec3 operator /(const simdvec3& a, const simdvec3& b) { return simdvec3(a.x / b.x, a.y / b.y, a.z / b.z); }

__forceinline simdfloat dot(const simdvec3& a, const simdvec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
__forceinline simdvec3 abs(const simdvec3& a) { return simdvec3(abs(a.x), abs(a.y), abs(a.z)); }
__forceinline simdvec3 rcp(const simdvec3& a) { return simdvec3(rcp(a.x), rcp(a.y), rcp(a.z)); }

template<typename T, typename U>
__forceinline static T mix(T const & x, T const & y, U const & a)
{
   return static_cast<T>(static_cast<U>(x) + a * static_cast<U>(y - x));
}


/*****************  simdvec4  *********************/

struct simdvec4
{
   simdvec4() {}
   explicit simdvec4(const simdfloat& f) : x(f), y(f), z(f), w(f) {}
   explicit simdvec4(const vec4& v) : x(v.x), y(v.y), z(v.z), w(v.w) {}
   simdvec4(simdfloat x, simdfloat y, simdfloat z, simdfloat w) : x(x), y(y), z(z), w(w) {}

   simdfloat x;
   simdfloat y;
   simdfloat z;
   simdfloat w;

   __forceinline const simdvec4 operator -(const simdvec4& a) { return simdvec4(-a.x, -a.y, -a.z, -a.w); }
};

__forceinline simdvec4 operator *(const simdfloat& f, const simdvec4& b) { return simdvec4(f * b.x, f * b.y, f * b.z, f* b.w); }

__forceinline simdvec4 operator *(const simdvec4& a, const simdvec4& b) { return simdvec4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w); }
__forceinline simdvec4 operator -(const simdvec4& a, const simdvec4& b) { return simdvec4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w); }
__forceinline simdvec4 operator +(const simdvec4& a, const simdvec4& b) { return simdvec4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w); }
__forceinline simdvec4 operator /(const simdvec4& a, const simdvec4& b) { return simdvec4(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w); }

__forceinline simdfloat dot(const simdvec4& a, const simdvec4& b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w+b.w; }
__forceinline simdvec4 abs(const simdvec4& a) { return simdvec4(abs(a.x), abs(a.y), abs(a.z), abs(a.w)); }


/*****************  simdmat4  *********************/

struct simdmat4
{
   simdmat4() {}
   explicit simdmat4(const mat4& v) : m {simdvec4(v[0]) , simdvec4(v[1]), simdvec4(v[2]), simdvec4(v[3]) } {}
   
   simdvec4 m[4];

};

__forceinline simdvec4 operator *(const simdmat4& mat, const simdvec4& v)
{
   simdvec4 mov0(v.x);
   simdvec4 mov1(v.y);
   simdvec4 mul0 = mat.m[0] * mov0;
   simdvec4 mul1 = mat.m[1] * mov1;
   simdvec4 add0 = mul0 + mul1;
   simdvec4 mov2(v.z);
   simdvec4 mov3(v.w);
   simdvec4 mul2 = mat.m[2] * mov2;
   simdvec4 mul3 = mat.m[3] * mov3;
   simdvec4 add1 = mul2 + mul3;
   simdvec4 add2 = add0 + add1;
   return add2;
}

/*****************  simdfrustum  *********************/
struct simdfrustum
{
   simdfloat left;
   simdfloat right;
   simdfloat bottom;
   simdfloat top;   
   simdfloat near;
   simdfloat far;
};

}