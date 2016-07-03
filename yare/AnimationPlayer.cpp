#include "AnimationPlayer.h"

#include <regex>
#include <string>
#include <assert.h>

#include "Scene.h"
#include "Skeleton.h"
#include "GLTexture.h"

namespace yare {

AnimationPlayer::AnimationPlayer()
: _previous_evaluated_x(FLT_MAX)
{

}

void AnimationPlayer::evaluateAndApplyToTargets(float x)
{
   _active_keyframes.resize(curves.size());
   
   x = fmod(x, 200.0f);
   bool jumped = (x < _previous_evaluated_x);
   _previous_evaluated_x = x;

   _updateCurveActiveKeyframes(x, jumped);
   _evaluateCurves(x);
}

void AnimationPlayer::_updateCurveActiveKeyframes(float x, bool jump)
{
   for (int i = 0; i < int(_active_keyframes.size()); ++i)
   {
      if (!jump && x <= _active_keyframes[i].second.x)
         continue;

      auto& curve = curves[i];
      int active_index = curve.getActiveKeyframeIndex(x, jump);

      _active_keyframes[i] = std::make_pair(curve.keyframes[active_index-1], curve.keyframes[active_index]);
   }
}

void AnimationPlayer::_evaluateCurves(float x)
{
   for (int i = 0; i < int(_active_keyframes.size()); ++i)
   {
      const auto& before = _active_keyframes[i].first;
      const auto& after = _active_keyframes[i].second;

      float mix_factor = (x - before.x) / (after.x - before.x);
      *(curves[i].target) = (1.0f - mix_factor)*before.y + mix_factor*after.y;
   }
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
         curve.target = &bone.local_transform.location[component_index];
      else if (transformation_component == "scale")
         curve.target = &bone.local_transform.scale[component_index];
      else if (transformation_component == "rotation_quaternion")
         curve.target = &bone.local_transform.quaternion[component_index==0 ? 3 : component_index-1];
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