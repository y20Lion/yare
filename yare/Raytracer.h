#pragma once

#include "tools.h"

#include <CLW/CLWProgram.h>
#include <CLW/CLWContext.h>
#include <math/ray.h>

namespace RadeonRays {
   class IntersectionApiCL;
   class Event;   
}
class CLWContext;

namespace yare {

class Scene;

class Raytracer
{
public:
   Raytracer();
   ~Raytracer();
   DISALLOW_COPY_AND_ASSIGN(Raytracer)

   void init(const Scene& scene);
   void bakeAmbiantOcclusionVolume(Scene& scene);
   void bakeSignedDistanceFieldVolume(Scene& scene);

private:
   void _wait();

private:
   RadeonRays::IntersectionApiCL* _api;
   RadeonRays::Event* _status_event;
   CLWContext _ocl_context;
   CLWProgram _raytracer_prog;
   CLWBuffer<RadeonRays::float3> _ocl_face_normal_buffer;
};

}