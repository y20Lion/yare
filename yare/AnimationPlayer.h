#pragma once

#include <vector>
#include "AnimationCurve.h"
#include "tools.h"

namespace yare {

class Scene;

struct Action
{
   std::string target_object;
   std::vector<AnimationCurve> curves;
   
};

class AnimationPlayer
{
public:
   AnimationPlayer();

   void evaluateAndApplyToTargets(float x);

   std::vector<AnimationCurve> curves;

private:
   void _updateCurveActiveKeyframes(float x, bool jump);
   void _evaluateCurves(float x);
   
private:   
   DISALLOW_COPY_AND_ASSIGN(AnimationPlayer)
   std::vector<std::pair<Keyframe, Keyframe>> _active_keyframes;
   float _previous_evaluated_x;
};

void bindAnimationCurvesToTargets(const Scene& scene, AnimationPlayer& player);


}