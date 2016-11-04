#pragma once

#include <glm/mat4x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_inverse.hpp>

namespace yare {
   using namespace glm;

inline mat4 toMat4(const mat4x3& matrix)
{
   mat4 result;
   result[0] = vec4(matrix[0], 0.0);
   result[1] = vec4(matrix[1], 0.0);
   result[2] = vec4(matrix[2], 0.0);
   result[3] = vec4(matrix[3], 1.0);
   return result;
}

inline mat4x3 toMat4x3(const mat4& matrix)
{
   mat4x3 result;
   result[0] = matrix[0].xyz;
   result[1] = matrix[1].xyz;
   result[2] = matrix[2].xyz;
   result[3] = matrix[3].xyz;
   return result;
}

inline mat4x3 inverseAS(const mat4x3& affine_space)
{
   mat3 rotation = inverse(mat3(affine_space));
   mat4x3 result = rotation;
   result[3] = -rotation*affine_space[3];
   return result;
}

inline mat4x3 composeAS(const mat4x3& affine_space1, const mat4x3& affine_space2)
{
   mat3 rotation1 = mat3(affine_space1);
   mat3 rotation2 = mat3(affine_space2);

   mat4x3 result = rotation1 * rotation2;
   result[3] = rotation1*affine_space2[3] + affine_space1[3];
   return result;
}
inline mat3 normalMatrix(const mat4x4& matrix)
{
   return mat3(transpose(inverse(matrix)));
}

inline vec3 project(const mat4x4& matrix, const vec3& point)
{
   vec4 vec_hs = matrix * vec4(point, 1.0f);
   return vec_hs.xyz / vec3(vec_hs.w);
}


}