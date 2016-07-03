#include "AnimationCurve.h"

#include <assert.h>

namespace yare {

AnimationCurve::AnimationCurve()
 : _current_keyframe_index(1)
 , target(nullptr)
{

}

int AnimationCurve::getActiveKeyframeIndex(float x, bool jump)
{
   if (jump)
      _current_keyframe_index = 1;

   for (; _current_keyframe_index < keyframes.size(); ++_current_keyframe_index)
   {
      if (keyframes[_current_keyframe_index].x > x)
         break;
   }

   return _current_keyframe_index;
}


}