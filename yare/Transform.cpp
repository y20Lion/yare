#include "Transform.h"

#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/transform.hpp>

#include "matrix_math.h"

namespace yare {

mat4x4 Transform::toMatrix() const
{
   mat4x4 rotation_matrix;
   switch (rotation_type)
   {
   case RotationType::Quaternion:
      rotation_matrix = mat4_cast(rotation_quaternion); break;
   case RotationType::EulerXYZ:
      rotation_matrix = eulerAngleXYZ(rotation_euler.x, rotation_euler.y, rotation_euler.z); break;
   case RotationType::EulerXZY:
      rotation_matrix = eulerAngleX(rotation_euler.x) * eulerAngleZ(rotation_euler.z) * eulerAngleY(rotation_euler.y); break;
   case RotationType::EulerYXZ:
      rotation_matrix = eulerAngleY(rotation_euler.y) * eulerAngleX(rotation_euler.x) * eulerAngleZ(rotation_euler.z); break;
   case RotationType::EulerYZX:
      rotation_matrix = eulerAngleY(rotation_euler.y) * eulerAngleZ(rotation_euler.z) * eulerAngleX(rotation_euler.x); break;
   case RotationType::EulerZXY:
      rotation_matrix = eulerAngleZ(rotation_euler.z) * eulerAngleX(rotation_euler.x) * eulerAngleY(rotation_euler.y); break;
   case RotationType::EulerZYX:
      rotation_matrix = eulerAngleZ(rotation_euler.z) * eulerAngleY(rotation_euler.y) * eulerAngleX(rotation_euler.x); break;
   }

   return translate(location)*rotation_matrix*glm::scale(scale);
}

}