#pragma once

#include "tools.h"

namespace RadeonRays {
   class IntersectionApi;
   class Event;
}

namespace yare {

class Scene;

class Raytracer
{
public:
   Raytracer();
   DISALLOW_COPY_AND_ASSIGN(Raytracer)

   void init(const Scene& scene);
   void raytraceTest();
   void bakeAmbiantOcclusionVolume();

private:
   void _wait();

private:
   RadeonRays::IntersectionApi* _api;
   RadeonRays::Event* _status_event;
};

}