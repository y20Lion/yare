#pragma once

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtx/quaternion.hpp>

namespace yare {

using namespace glm;

enum class RotationType { Quaternion, EulerXYZ, EulerXZY, EulerYXZ, EulerYZX, EulerZXY, EulerZYX, AxisAngle };

struct Transform
{
   vec3 location;
   RotationType rotation_type;
   vec4 rotation; 
   // in case of quaternion rotation.x is quat.w, rotation.y is quat.x, rotation.z is quat.y, rotation.w is quat.w
   // It's ugly but makes things easier to import blender animations curves
   vec3 scale;    

   mat4x4 toMatrix() const;
};



}