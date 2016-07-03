#pragma once

#include <string>
#include <vector>
#include "AnimationCurve.h"
#include "tools.h"

namespace yare {

class Scene;

struct Action
{
   std::string target_object;
   std::vector<AnimationCurve> curves;
   std::vector<std::pair<Keyframe, Keyframe>> active_keyframes;
};

class AnimationPlayer
{
public:
   AnimationPlayer();

   void evaluateAndApplyToTargets(float x);

   std::vector<Action> actions;

private:
   void _updateCurveActiveKeyframes(float x, Action& action, bool jump);
   void _evaluateCurves(float x, const Action& action);
   
private:   
   DISALLOW_COPY_AND_ASSIGN(AnimationPlayer)
   
   float _previous_evaluated_x;
};

void bindAnimationCurvesToTargets(const Scene& scene, AnimationPlayer& player);


}