#include "Skeleton.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include "GLBuffer.h"
#include "matrix_math.h"

namespace yare {
using namespace glm;

Skeleton::Skeleton(int bone_count)
: _bone_count(bone_count)
{
   bones.resize(bone_count);
   skeleton_to_bone_bind_pose_matrices.resize(bone_count);

   _skinning_palette_ssbo = createPersistentBuffer(sizeof(mat4x4)* bone_count);
}

void Skeleton::update()
{
   mat4x4* buffer = (mat4x4*)_skinning_palette_ssbo->getCurrentWindowPtr();
   for (int i = 0; i < _bone_count; ++i)
   {
      auto& bone_pose = bones[i].parent_to_bone_transform;
      mat4x4 world_to_bone_bind_pose_matrix = toMat4(world_to_skeleton_matrix)*toMat4(skeleton_to_bone_bind_pose_matrices[i]);
      mat4x4 world_to_bone_pose_matrix =  world_to_bone_bind_pose_matrix*translate(bone_pose.location)*mat4_cast(bone_pose.quaternion)*scale(bone_pose.scale);

      buffer[i] = world_to_bone_pose_matrix*inverse(world_to_bone_bind_pose_matrix);
   }
}

}