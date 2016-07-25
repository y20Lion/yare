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
      rotation_matrix = mat4_cast(quat(rotation.x, rotation.y, rotation.z, rotation.w)); break;
   case RotationType::EulerZYX:
      rotation_matrix = eulerAngleX(rotation.x) * eulerAngleY(rotation.y) * eulerAngleZ(rotation.z); break;
   case RotationType::EulerYZX:
      rotation_matrix = eulerAngleX(rotation.x) * eulerAngleZ(rotation.z) * eulerAngleY(rotation.y); break;
   case RotationType::EulerZXY:
      rotation_matrix = eulerAngleY(rotation.y) * eulerAngleX(rotation.x) * eulerAngleZ(rotation.z); break;
   case RotationType::EulerXZY:
      rotation_matrix = eulerAngleY(rotation.y) * eulerAngleZ(rotation.z) * eulerAngleX(rotation.x); break;
   case RotationType::EulerYXZ:
      rotation_matrix = eulerAngleZ(rotation.z) * eulerAngleX(rotation.x) * eulerAngleY(rotation.y); break;
   case RotationType::EulerXYZ:
      rotation_matrix = eulerAngleZ(rotation.z) * eulerAngleY(rotation.y) * eulerAngleX(rotation.x); break;
   case RotationType::AxisAngle:
      rotation_matrix = rotate(rotation.x, vec3(rotation.y, rotation.z, rotation.w)); break;
   }

   return translate(location)*rotation_matrix*glm::scale(scale);
}

}