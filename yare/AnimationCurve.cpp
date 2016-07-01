#include "AnimationCurve.h"

#include <assert.h>

namespace yare {

AnimationCurve::AnimationCurve()
 : _current_keyframe_index(1)
 , target(nullptr)
{

}

void AnimationCurve::evaluateAndApplyToTarget(float x)
{
   assert(target != nullptr);

   _current_keyframe_index = 1;// TODO FIXME

   if (x <= keyframes.front().x)
   {
      *target = keyframes.front().y;
      return;
   }
   if (x >= keyframes.back().x)
   {
      *target = keyframes.back().y;
      return;
   }

   for (; _current_keyframe_index < keyframes.size(); ++_current_keyframe_index)
   {
      if (keyframes[_current_keyframe_index].x > x)
         break;
   }


   const auto& before = keyframes[_current_keyframe_index-1];
   const auto& after = keyframes[_current_keyframe_index];

   float mix_factor = (x - before.x) / (after.x - before.x);

   *target = (1.0 - mix_factor)*before.y + mix_factor*after.y;
}

}