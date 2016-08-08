#include "Raytracer.h"

#include <radeon_rays.h>
#include <assert.h>
#include <glm/gtc/random.hpp>
#include <iostream>

#include "Scene.h"
#include "RenderMesh.h"
#include "TransformHierarchy.h"
#include "GLTexture.h"

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

static matrix _convertMatrix(const mat4x3 mat)
{
   return matrix(mat[0][0], mat[1][0], mat[2][0], mat[3][0],
                 mat[0][1], mat[1][1], mat[2][1], mat[3][1], 
                 mat[0][2], mat[1][2], mat[2][2], mat[3][2],
                       0.0,       0.0,       0.0,       1.0);
}

void Raytracer::init(const Scene& scene)
{
   std::vector<int> indices(500000);
   for (int i = 0; i < 500000; ++i)
   {
      indices[i] = i;
   }
   
   int i = 0;
   for (const auto& surface : scene.opaque_surfaces)
   {
      float* vertices = (float*)surface.mesh->mapVertices(MeshFieldName::Position);
      int vertex_count = surface.mesh->vertexCount();
      Shape* shape = _api->CreateMesh(vertices, vertex_count, 3 * sizeof(float), indices.data(), 0, nullptr, vertex_count/3);
      shape->SetId(i++);

      auto mat = scene.transform_hierarchy->nodeWorldToLocalMatrix(surface.transform_node_index);
      matrix radeon_mat = _convertMatrix(mat);
      shape->SetTransform((radeon_mat), inverse(radeon_mat));
      _api->AttachShape(shape);
      surface.mesh->unmapVertices();
   }

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

   //_api->QueryOcclusion(ray_buffer, 1, isect_buffer, nullptr, nullptr);

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

static vec3 _position(int x, int y, int z, const AOVolume& ao_volume)
{
   vec3 corner = ao_volume.position - 0.5f*ao_volume.size;
   return corner + ao_volume.size * vec3(float(x) + 0.5f, float(y) + 0.5f, float(z) + 0.5f) / float(ao_volume.resolution);
}

static float3 _convertVec3(const vec3& vec)
{
   return float3(vec.x, vec.y, vec.z);
}

void Raytracer::bakeAmbiantOcclusionVolume(Scene& scene)
{   
    AOVolume& ao_volume = scene.ambient_occlusion_volumes[0];
    ao_volume.resolution = 128;
   auto ao_texture = std::make_unique<float[]>(ao_volume.resolution*ao_volume.resolution*ao_volume.resolution); 
   //auto ao_texture_final = std::make_unique<char[]>(ao_volume.resolution*ao_volume.resolution*ao_volume.resolution);
   int ray_count = ao_volume.resolution*ao_volume.resolution*ao_volume.resolution;
   int voxel_count = ray_count;
   auto ray_buffer = _api->CreateBuffer(sizeof(ray)*ray_count, nullptr);
   auto isect_buffer = _api->CreateBuffer(sizeof(int)*ray_count, nullptr);

   #pragma omp parallel for
   for (int z = 0; z < ao_volume.resolution; ++z)
   {
      for (int y = 0; y < ao_volume.resolution; ++y)
      {
         for (int x = 0; x < ao_volume.resolution; ++x)
         {           
            (ao_texture.get())[z*(ao_volume.resolution*ao_volume.resolution) + y*ao_volume.resolution + x] = 0.0;
         }
      }
   }   
   int pass_count = 200;
   for (int pass = 0; pass < pass_count; ++pass)
   {      
      ray* rays = nullptr;
      _api->MapBuffer(ray_buffer, kMapWrite, 0, sizeof(ray)*ray_count, (void**)&rays, &_status_event);
      _wait();

      vec3 pass_direction = glm::sphericalRand(1.0f);
      
      #pragma omp parallel for
      for (int z = 0; z < ao_volume.resolution; ++z)
      {
         for (int y = 0; y < ao_volume.resolution; ++y)
         {
            for (int x = 0; x < ao_volume.resolution; ++x)
            {
               

               vec3 pos = _position(x, y, z, ao_volume);
               rays[z*(ao_volume.resolution*ao_volume.resolution) + y*ao_volume.resolution + x] = ray(_convertVec3(pos), _convertVec3(pass_direction)/*float3(0.f, 0.f, -1.f)*/, 1000000.f);
            }
         }
      }

      _api->UnmapBuffer(ray_buffer, rays, &_status_event);
      _wait();

      _api->QueryOcclusion(ray_buffer, ray_count, isect_buffer, nullptr, nullptr);

      int* intersections = nullptr;
      _api->MapBuffer(isect_buffer, kMapRead, 0, sizeof(int)*ray_count, (void**)&intersections, &_status_event);
      _wait();

   #pragma omp parallel for
      for (int z = 0; z < ao_volume.resolution; ++z)
      {
         for (int y = 0; y < ao_volume.resolution; ++y)
         {
            for (int x = 0; x < ao_volume.resolution; ++x)
            {
               int shape_id = intersections[z*(ao_volume.resolution*ao_volume.resolution) + y*ao_volume.resolution + x];
               (ao_texture.get())[z*(ao_volume.resolution*ao_volume.resolution) + y*ao_volume.resolution + x] += float(shape_id != -1);
            }
         }
      }

      _api->UnmapBuffer(isect_buffer, intersections, &_status_event);
      _wait();
      std::cout << pass << std::endl;
   }

   #pragma omp parallel for
   for (int i = 0; i < voxel_count; ++i)
   {
      float& voxel_value = (ao_texture.get())[i];
      voxel_value = 1.0 - voxel_value /float(pass_count);
   }

   scene.ao_volume = createTexture3D(ao_volume.resolution, ao_volume.resolution, ao_volume.resolution, GL_R16F, ao_texture.get());

}

void Raytracer::_wait()
{
   _status_event->Wait();
   _api->DeleteEvent(_status_event);
}


}
