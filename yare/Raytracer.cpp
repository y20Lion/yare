#include "Raytracer.h"

#include <radeon_rays.h>
#include <assert.h>

namespace yare {

using namespace RadeonRays;

Raytracer::Raytracer()
{
   int nativeidx = -1;
   for (uint32_t idx = 0; idx < IntersectionApi::GetDeviceCount(); ++idx)
   {
      DeviceInfo devinfo;
      IntersectionApi::GetDeviceInfo(idx, devinfo);

      if (devinfo.type == DeviceInfo::kGpu)
      {
         nativeidx = idx;
      }
   }

   assert(nativeidx != -1);

   _api = IntersectionApi::Create(nativeidx);
}

void Raytracer::init(const Scene& scene)
{
   // Mesh vertices
   float vertices[] = {
      -1.f,-1.f,0.f,
      1.f,-1.f,0.f,
      0.f,1.f,0.f,

   };

   // Indices
   int indices[] = { 0, 1, 2 };
   // Number of vertices for the face
   int numfaceverts[] = { 3 };

   Shape* shape = _api->CreateMesh(vertices, 3, 3 * sizeof(float), indices, 0, numfaceverts, 1);
   shape->SetId(42);
   _api->AttachShape(shape);
   _api->Commit();
}

void Raytracer::raytraceTest()
{
   // Prepare the ray
   ray r(float3(0.f, 0.f, -10.f), float3(0.f, 0.f, 1.f), 10000.f);
  // r.SetMask(0xFFFFFFFF);
   
   auto ray_buffer = _api->CreateBuffer(sizeof(ray), &r);
   auto isect_buffer = _api->CreateBuffer(sizeof(Intersection), nullptr);
   //auto isect_flag_buffer = _api->CreateBuffer(sizeof(int), nullptr);

   // Commit geometry update
   //ASSERT_NO_THROW(api_->Commit());

   // Intersect
   _api->QueryIntersection(ray_buffer, 1, isect_buffer, nullptr, nullptr);

   Intersection* tmp = nullptr;
   Intersection isect;
   _api->MapBuffer(isect_buffer, kMapRead, 0, sizeof(Intersection), (void**)&tmp, &_status_event);
   _wait();
   isect = *tmp;
   _api->UnmapBuffer(isect_buffer, tmp, &_status_event);
   _wait();

   // Check results
   //isect.shapeid, mesh->GetId();
}


void Raytracer::bakeAmbiantOcclusionVolume()
{

}

void Raytracer::_wait()
{
   _status_event->Wait();
   _api->DeleteEvent(_status_event);
}


}
