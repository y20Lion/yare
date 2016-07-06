#pragma once

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtx/quaternion.hpp>

namespace yare {

using namespace glm;

enum class RotationType { Quaternion, EulerXYZ, EulerXZY, EulerYXZ, EulerYZX, EulerZXY, EulerZYX };

struct Transform
{
   vec3 location;
   RotationType rotation_type;
   quat rotation_quaternion;
   vec3 rotation_euler;   
   vec3 scale;

   mat4x4 toMatrix() const;
};


}