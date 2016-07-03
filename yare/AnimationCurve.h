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

   int getActiveKeyframeIndex(float x, bool jump);

   std::vector<Keyframe> keyframes;
   std::string target_path;
   float* target;

private:
   int _current_keyframe_index;
};

}