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
, root_bone_index(-1)
{
   bones.resize(bone_count);
   skeleton_to_bone_bind_pose_matrices.resize(bone_count);
   _skeleton_to_bone_pose_matrices.resize(bone_count);

   _skinning_palette_ssbo = createDynamicBuffer(sizeof(mat4x4)* bone_count);
}

void Skeleton::update()
{
   _updateBone(root_bone_index, mat4(1.0));
   
   mat4x4* buffer = (mat4x4*)_skinning_palette_ssbo->getUpdateSegmentPtr();
   for (int i = 0; i < _bone_count; ++i)
   {
      mat4x3 world_to_bone_bind_pose_matrix = composeAS(world_to_skeleton_matrix,skeleton_to_bone_bind_pose_matrices[i]);
      mat4x3 world_to_bone_pose_matrix = composeAS(world_to_skeleton_matrix, _skeleton_to_bone_pose_matrices[i]);

      buffer[i] = composeAS(world_to_bone_pose_matrix,inverseAS(world_to_bone_bind_pose_matrix));
   }
}

void Skeleton::_updateBone(int index, const mat4x4 skeleton_to_parent_matrix)
{
   auto& bone = bones[index];
   auto& bone_pose = bone.local_transform;

   mat4x3 skeleteon_to_current_bone = composeAS(skeleton_to_parent_matrix, bone.parent_to_bone_matrix); 
   _skeleton_to_bone_pose_matrices[index] = composeAS(skeleteon_to_current_bone, toMat4x3(translate(bone_pose.location)*mat4_cast(bone_pose.rotation_quaternion)*scale(bone_pose.scale)));

   for (int child_index : bone.chilren)
      _updateBone(child_index, _skeleton_to_bone_pose_matrices[index]);
}

}