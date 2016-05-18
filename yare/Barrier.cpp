#include "Barrier.h"

namespace yare {

void Barrier::wait()
{
   std::unique_lock<std::mutex> lock(_mutex);
   unsigned int gen = _generation;
   if (--_count == 0)
   {
      _generation++;
      _count = _count_reset_value;
      _cond.notify_all();
      return;
   }
   while (gen == _generation)
      _cond.wait(lock);
}

}