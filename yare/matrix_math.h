#pragma once

#include <glm/mat4x3.hpp>
#include <glm/mat4x4.hpp>

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
}