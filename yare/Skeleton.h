#pragma once

#include <vector>
#include <map>
#include <glm/vec4.hpp>
#include <glm/vec2.hpp>
#include <glm/mat4x3.hpp>
#include <glm/gtc/quaternion.hpp>

#include "tools.h"
#include "Transform.h"

namespace yare
{
using namespace glm;
class GLDynamicBuffer;

struct Bone
{
   std::string name;
   Transform local_transform;
   mat4x3 parent_to_bone_matrix;
   int parent;
   std::vector<int> chilren;
};

class Skeleton
{
public:
   Skeleton(int bone_count);
   
   void update();

   Bone& bone(const std::string& bone_name) { return bones[bone_name_to_index[bone_name]];  }

   std::string name;
   mat4x3 world_to_skeleton_matrix;
   std::vector<Bone> bones;
   std::map<std::string, int> bone_name_to_index;
   std::vector<mat4x3> skeleton_to_bone_bind_pose_matrices;
   int root_bone_index;

   GLDynamicBuffer& skinningPalette() { return *_skinning_palette_ssbo;  }

private:
   void _updateBone(int index, const mat4x4 parent_to_bone_matrix);
private:   
   DISALLOW_COPY_AND_ASSIGN(Skeleton)
   int _bone_count;
   std::vector<mat4x4> _skeleton_to_bone_pose_matrices;
   Uptr<GLDynamicBuffer> _skinning_palette_ssbo;
};

}