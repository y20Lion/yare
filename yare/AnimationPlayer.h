#pragma once

#include <vector>
#include "AnimationCurve.h"
#include "tools.h"

namespace yare {

class Scene;

class AnimationPlayer
{
public:
   AnimationPlayer();

   void evaluateAndApplyToTargets(float x);

   std::vector<AnimationCurve> curves;

private:
   DISALLOW_COPY_AND_ASSIGN(AnimationPlayer)
};

void bindAnimationCurvesToTargets(const Scene& scene, AnimationPlayer& player);


}