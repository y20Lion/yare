#include "AnimationPlayer.h"

#include <regex>
#include <string>
#include <assert.h>

#include "Scene.h"
#include "Skeleton.h"
#include "GLTexture.h"
#include "TransformHierarchy.h"

namespace yare {

AnimationPlayer::AnimationPlayer()
: _previous_evaluated_x(FLT_MAX)
{

}

void AnimationPlayer::evaluateAndApplyToTargets(float x)
{
   x = fmod(x, 20.0f);
   bool jumped = (x < _previous_evaluated_x);
   _previous_evaluated_x = x;

   for (auto& action : actions)
   {
      action.active_keyframes.resize(action.curves.size());
      _updateCurveActiveKeyframes(x, action, jumped);
      _evaluateCurves(x, action);
   }
}

void AnimationPlayer::_updateCurveActiveKeyframes(float x, Action& action, bool jump)
{
   for (int i = 0; i < int(action.active_keyframes.size()); ++i)
   {
      if (!jump && x <= action.active_keyframes[i].second.x)
         continue;

      auto& curve = action.curves[i];
      int active_index = curve.getActiveKeyframeIndex(x, jump);

      action.active_keyframes[i] = std::make_pair(curve.keyframes[active_index-1], curve.keyframes[active_index]);
   }
}

void AnimationPlayer::_evaluateCurves(float x, const Action& action)
{
   for (int i = 0; i < int(action.active_keyframes.size()); ++i)
   {
      const auto& before = action.active_keyframes[i].first;
      const auto& after = action.active_keyframes[i].second;

      float mix_factor = (x - before.x) / (after.x - before.x);
      *(action.curves[i].target) = (1.0f - mix_factor)*before.y + mix_factor*after.y;
   }
}

static void _bindToTransform(const std::string& transformation_component, int component_index, Transform& transform, AnimationCurve& curve)
{
   if (transformation_component == "location")
      curve.target = &transform.location[component_index];
   else if (transformation_component == "scale")
      curve.target = &transform.scale[component_index];
   else if (transformation_component == "rotation_quaternion")
      curve.target = &transform.rotation[component_index];
   else if (transformation_component == "rotation_euler")
      curve.target = &transform.rotation[component_index];
   else if (transformation_component == "rotation_axis_angle")
      curve.target = &transform.rotation[component_index];
}

static void _bindTarget(const Scene& scene, const std::string& object_name, AnimationCurve& curve)
{
   static std::regex bone_path_regex("bone/(.*)/(.*)/([0-3])");
   static std::regex surface_path_regex("transform/(.*)/([0-3])");   

   std::smatch regex_result;
   if (std::regex_search(curve.target_path, regex_result, bone_path_regex))
   {
      const std::string& bone_name = regex_result[1];
      const std::string& transformation_component = regex_result[2];
      int component_index = std::stoi(regex_result[3]);

      Skeleton& skeleton = *scene.name_to_skeleton.at(object_name);
      Bone& bone = skeleton.bone(bone_name);

      _bindToTransform(transformation_component, component_index, bone.local_transform, curve);

      /*else
         assert(false);*/ // TODO fix silent ignore
   }
   else if (std::regex_search(curve.target_path, regex_result, surface_path_regex))
   {
      int node_index = scene.object_name_to_transform_node_index.at(object_name);
      Transform& transform = scene.transform_hierarchy->nodeParentToLocalTransform(node_index);
      
      const std::string& transformation_component = regex_result[1];
      int component_index = std::stoi(regex_result[2]);

      _bindToTransform(transformation_component, component_index, transform, curve);
   }
   else
      assert(false);
}

void bindAnimationCurvesToTargets(const Scene& scene, AnimationPlayer& player)
{
   for (auto& action : player.actions)
   {
      for (auto& curve : action.curves)
      {
         _bindTarget(scene, action.target_object, curve);
      }
   }
}

}