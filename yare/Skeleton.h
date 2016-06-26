#pragma once

#include <vector>
#include <glm/vec4.hpp>
#include <glm/vec2.hpp>
#include <glm/mat4x3.hpp>

#include "tools.h"

namespace yare
{
using namespace glm;
class GLPersistentlyMappedBuffer;

struct BoneLocalTransform
{
   vec4 quaternion;
   vec3 location;
   vec3 scale; 
   mat4x3 pose_matrix;
};

struct Bone
{
   std::string name;
   BoneLocalTransform parent_to_bone_transform;
   int parent;
   std::vector<int> chilren;
};

class Skeleton
{
public:
   Skeleton(int bone_count);
   
   void update();

   std::string name;
   mat4x3 world_to_skeleton_matrix;
   std::vector<Bone> bones;
   std::vector<mat4x3> skeleton_to_bone_bind_pose_matrices;

   GLPersistentlyMappedBuffer& skinningPalette() { return *_skinning_palette_ssbo;  }
private:   
   DISALLOW_COPY_AND_ASSIGN(Skeleton)
   int _bone_count;
   Uptr<GLPersistentlyMappedBuffer> _skinning_palette_ssbo;
};

}