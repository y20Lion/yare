#include "AnimationPlayer.h"

#include <regex>
#include <string>
#include <assert.h>

#include "Scene.h"
#include "Skeleton.h"
#include "GLTexture.h"

namespace yare {

AnimationPlayer::AnimationPlayer()
{

}

void AnimationPlayer::evaluateAndApplyToTargets(float x)
{
   for (auto& curve : curves)
      curve.evaluateAndApplyToTarget(fmod(x, 30.0));
}

static void _bindTarget(const Scene& scene, AnimationCurve& curve)
{
   static std::regex bone_path_regex("bone/(.*)/(.*)/([0-3])");
   

   std::smatch regex_result;
   if (std::regex_search(curve.target_path, regex_result, bone_path_regex))
   {
      const std::string& bone_name = regex_result[1];
      const std::string& transformation_component = regex_result[2];
      int component_index = std::stoi(regex_result[3]);

      Skeleton& skeleton = *scene.skeletons[0];
      Bone& bone = skeleton.bone(bone_name);

      if (transformation_component == "location")
         curve.target = &bone.parent_to_bone_transform.location[component_index];
      else if (transformation_component == "scale")
         curve.target = &bone.parent_to_bone_transform.scale[component_index];
      else if (transformation_component == "rotation_quaternion")
         curve.target = &bone.parent_to_bone_transform.quaternion[component_index==0 ? 3 : component_index-1];
      /*else
         assert(false);*/ // TODO fix silent ignore
   }
   else
      assert(false);
   /* // TODO handle objects
   else if (std::regex_search(curve.target_path, regex_result, transform_regex))
   {
      "location"
         "scale"
         "rotation_euler"
   }*///static std::regex transform_regex("(.*)/(.*)/)");

}

void bindAnimationCurvesToTargets(const Scene& scene, AnimationPlayer& player)
{
   for (auto& curve : player.curves)
   {
      _bindTarget(scene, curve);
   }
}

}