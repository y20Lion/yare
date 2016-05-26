#pragma once

#include <GL/glew.h>

#include "tools.h"

namespace yare {

static const int QUERY_COUNT = 10;

class GLGPUTimer
{
public:
   GLGPUTimer();
   ~GLGPUTimer();
      
   void start();
   void stop();
   double elapsedTimeInMs() const;  

   static void swapCounters();

private:
    DISALLOW_COPY_AND_ASSIGN(GLGPUTimer)
    static int _used_query_index;
    GLuint _query_start[QUERY_COUNT];
    GLuint _query_stop[QUERY_COUNT];
};


}