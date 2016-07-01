#pragma once

#include <vector>
#include "tools.h"

namespace yare {

struct Keyframe
{
   float x;
   float y;
};

class AnimationCurve
{
public:
   AnimationCurve();

   void evaluateAndApplyToTarget(float x);

   std::vector<Keyframe> keyframes;
   std::string target_path;
   float* target;

private:
   int _current_keyframe_index;
};

}