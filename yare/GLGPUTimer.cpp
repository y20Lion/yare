#include "GLGPUTimer.h"

#include <iostream>

namespace yare {

int GLGPUTimer::_used_query_index = 0;

GLGPUTimer::GLGPUTimer()
{
   glGenQueries(QUERY_COUNT, _query_start);
   glGenQueries(QUERY_COUNT, _query_stop);
   
   // dummy queries for first frame
   for (int i = 0; i < QUERY_COUNT; ++i)
   {      
      glQueryCounter(_query_start[i], GL_TIMESTAMP);
      glQueryCounter(_query_stop[i], GL_TIMESTAMP);
   }   
}

GLGPUTimer::~GLGPUTimer()
{
   glDeleteQueries(QUERY_COUNT, _query_start);
   glDeleteQueries(QUERY_COUNT, _query_stop);
}

void GLGPUTimer::start()
{
   glQueryCounter(_query_start[_used_query_index], GL_TIMESTAMP);
}

void GLGPUTimer::stop()
{
   glQueryCounter(_query_stop[_used_query_index], GL_TIMESTAMP);
}

double GLGPUTimer::elapsedTimeInMs(TimerResult timer_result) const
{
   GLuint64 start, stop;
   int index;
   if (timer_result == TimerResult::CurrentFrame)
   {
      index = _used_query_index;
   }
   else
   {
      index = (_used_query_index - QUERY_COUNT - 1);
      if (index < 0)
         index = QUERY_COUNT - 1;
   }
   
   glGetQueryObjectui64v(_query_start[index], GL_QUERY_RESULT, &start);
   glGetQueryObjectui64v(_query_stop[index], GL_QUERY_RESULT, &stop);
   return (stop-start)/1000000.0;
}

void GLGPUTimer::printElapsedTimeInMs(TimerResult timer_result) const
{
   std::cout << elapsedTimeInMs(timer_result) << std::endl;
}

void GLGPUTimer::swapCounters()
{ 
   _used_query_index = (_used_query_index + 1) % QUERY_COUNT;
}



}