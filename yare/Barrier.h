#pragma once

#include <condition_variable>
#include "tools.h"

namespace yare {

class Barrier
{
public:
   explicit Barrier(unsigned int count) :
      _count(count), _generation(0),
      _count_reset_value(count)
   {
   }   
   
   void wait();

private:
   DISALLOW_COPY_AND_ASSIGN(Barrier)
   std::mutex _mutex;
   std::condition_variable _cond;
   unsigned int _count;
   unsigned int _generation;
   unsigned int _count_reset_value;
};

}