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

   __m128 val;
};

__forceinline simdfloat operator *(const simdfloat& a, const simdfloat& b) { return _mm_mul_ps(a.val, b.val); }
__forceinline simdfloat operator -(const simdfloat& a, const simdfloat& b) { return _mm_sub_ps(a.val, b.val); }
__forceinline simdfloat operator +(const simdfloat& a, const simdfloat& b) { return _mm_add_ps(a.val, b.val); }

__forceinline const simdfloat operator -(const simdfloat& a) { return _mm_xor_ps(a.val, _mm_castsi128_ps(_mm_set1_epi32(0x80000000))); }

__forceinline const simdbool operator >(const simdfloat& a, const simdfloat& b) { return _mm_cmpgt_ps(a.val, b.val); }
__forceinline const simdbool operator <(const simdfloat& a, const simdfloat& b) { return _mm_cmplt_ps(a.val, b.val); }
__forceinline const simdbool operator >=(const simdfloat& a, const simdfloat& b) { return _mm_cmpge_ps(a.val, b.val); }
__forceinline const simdbool operator <=(const simdfloat& a, const simdfloat& b) { return _mm_cmple_ps(a.val, b.val); }

__forceinline simdfloat abs(const simdfloat& a) { return _mm_and_ps(a.val, _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff))); }

/*****************  simdvec3  *********************/

struct simdvec3
{
   simdvec3() {}
   explicit simdvec3(const vec3& v) : x(v.x), y(v.y), z(v.z) {}
   simdvec3(simdfloat x, simdfloat y, simdfloat z) : x(x), y(y), z(z) {}

   void load(float* xvalues, float* yvalues, float* zvalues, int offset) { x.load(xvalues + offset); y.load(yvalues + offset); z.load(zvalues + offset); }

   simdfloat x;
   simdfloat y;
   simdfloat z;

   __forceinline const simdvec3 operator -(const simdvec3& a) { return simdvec3(-a.x, -a.y, -a.z); }
};

__forceinline simdfloat dot(const simdvec3& a, const simdvec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

__forceinline simdvec3 abs(const simdvec3& a) { return simdvec3(abs(a.x), abs(a.y), abs(a.z)); }

}