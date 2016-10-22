#include "Raytracer.h"

#include <radeon_rays_cl.h>
#include <assert.h>
#include <glm/gtc/random.hpp>
#include <iostream>
#include <CLW/CLW.h>

#include "Scene.h"
#include "RenderMesh.h"
#include "TransformHierarchy.h"
#include "GLTexture.h"
#include "matrix_math.h"

namespace yare {

using namespace RadeonRays;

CLWContext CreateOpenCLContext()
{
   std::vector<CLWPlatform> platforms;
   CLWPlatform::CreateAllPlatforms(platforms);

   for (int i = 0; i < platforms.size(); ++i)
   {
      for (int d = 0; d < (int)platforms[i].GetDeviceCount(); ++d)
      {
         if (platforms[i].GetDevice(d).GetType() != CL_DEVICE_TYPE_GPU)
            continue;

         return CLWContext::Create(platforms[i].GetDevice(d));
      }
   }
   return CLWContext::Create(platforms[0].GetDevice(0));
}

Raytracer::Raytracer()
{
   _ocl_context = CreateOpenCLContext();
   cl_device_id id = _ocl_context.GetDevice(0).GetID();
   cl_command_queue queue = _ocl_context.GetCommandQueue(0);
   _api = IntersectionApiCL::CreateFromOpenClContext(_ocl_context, id, queue);

   _raytracer_prog = CLWProgram::CreateFromFile("raytracer_kernels.cl", _ocl_context);  
}

Raytracer::~Raytracer()
{
   IntersectionApi::Delete(_api);
}

static matrix _convertMatrix(const mat4x3 mat)
{
   return matrix(mat[0][0], mat[1][0], mat[2][0], mat[3][0],
                 mat[0][1], mat[1][1], mat[2][1], mat[3][1], 
                 mat[0][2], mat[1][2], mat[2][2], mat[3][2],
                       0.0,       0.0,       0.0,       1.0);
}

static void _fillFaceNormals(const mat3& matrix_normal_world_local, vec3* vertices, vec4* mapped_face_normals, int normals_offset, int face_count)
{
   #pragma omp parallel for
   for (int i = 0; i < face_count; ++i)
   {
      vec3 side1 = vertices[3*i + 1] - vertices[3*i];
      vec3 side2 = vertices[3*i + 2] - vertices[3*i];
      mapped_face_normals[normals_offset + i].xyz = matrix_normal_world_local * normalize(cross(side1, side2));
   }
}

void Raytracer::init(const Scene& scene)
{
   int total_triangle_count = 0;
   int max_triangles_for_one_mesh = 0;
   for (const auto& surface : scene.opaque_surfaces)
   {      
      total_triangle_count += surface.mesh->triangleCount();
      max_triangles_for_one_mesh = std::max(max_triangles_for_one_mesh, surface.mesh->triangleCount());
   }
      
   std::vector<int> indices(max_triangles_for_one_mesh*3);
   for (int i = 0; i < max_triangles_for_one_mesh * 3; ++i)
   {
      indices[i] = i;
   }

   float3* mapped_face_normals;
   _ocl_face_normal_buffer = _ocl_context.CreateBuffer<float3>(total_triangle_count, CL_MEM_READ_WRITE);
   _ocl_context.MapBuffer(0, _ocl_face_normal_buffer, CL_MAP_WRITE, &mapped_face_normals).Wait();
   
   int i = 0;
   int normals_offset = 0;
   for (const auto& surface : scene.opaque_surfaces)
   {
      auto mat = scene.transform_hierarchy->nodeWorldToLocalMatrix(surface.transform_node_index);

      float* vertices = (float*)surface.mesh->mapVertices(MeshFieldName::Position);
      _fillFaceNormals(normalMatrix(mat), (vec3*)vertices, (vec4*)mapped_face_normals, normals_offset, surface.mesh->triangleCount());

      int vertex_count = surface.mesh->vertexCount();

      Shape* shape = _api->CreateMesh(vertices, vertex_count, 3 * sizeof(float), indices.data(), 0, nullptr, vertex_count / 3);
      shape->SetId(normals_offset);
      matrix radeon_mat = _convertMatrix(mat);
      shape->SetTransform((radeon_mat), inverse(radeon_mat));
      _api->AttachShape(shape);

      surface.mesh->unmapVertices();

      normals_offset += vertex_count / 3;
   }

   _ocl_context.UnmapBuffer(0, _ocl_face_normal_buffer, mapped_face_normals).Wait();

   _api->Commit();
}

static vec3 _position(int x, int y, int z, const AOVolume& ao_volume)
{
   vec3 corner = ao_volume.position - 0.5f*ao_volume.size;
   return corner + ao_volume.size * vec3(float(x) + 0.5f, float(y) + 0.5f, float(z) + 0.5f) / vec3(ao_volume.resolution);
}

static float3 _convertVec3(const vec3& vec)
{
   return float3(vec.x, vec.y, vec.z);
}

static cl_float3 _toCL(const vec3& vec)
{
   return cl_float3{ vec.x, vec.y, vec.z };
}

void Raytracer::bakeAmbiantOcclusionVolume(Scene& scene)
{   
   AOVolume& ao_volume = *scene.ao_volume;
   ivec3 resolution = ao_volume.resolution;
   int ray_count = resolution.x*resolution.y*resolution.z;
   int voxel_count = ray_count;
   auto final_ao_texture = std::make_unique<unsigned char[]>(voxel_count);

   auto ocl_ray_buffer =  _ocl_context.CreateBuffer<ray>(ray_count, CL_MEM_READ_WRITE);
   auto ray_buffer = _api->CreateFromOpenClBuffer(ocl_ray_buffer);

   auto ocl_intersection_buffer = _ocl_context.CreateBuffer<int>(ray_count, CL_MEM_READ_WRITE);
   auto isect_buffer = _api->CreateFromOpenClBuffer(ocl_intersection_buffer);

   auto ocl_ao_volume_buffer = _ocl_context.CreateBuffer<float>(ray_count, CL_MEM_READ_WRITE);   
   _ocl_context.FillBuffer(0, ocl_ao_volume_buffer, 0.0f, ray_count);


   CLWKernel rays_kernel = _raytracer_prog.GetKernel("initRays");
   rays_kernel.SetArg(0, _toCL(resolution));
   rays_kernel.SetArg(1, _toCL(ao_volume.size));
   rays_kernel.SetArg(2, _toCL(ao_volume.position));
   rays_kernel.SetArg(3, ray_count);
   rays_kernel.SetArg(4, ocl_ray_buffer);
   size_t gs[] = { resolution.x, resolution.y, resolution.z };
   size_t ls[] = { 1, 1, 1 };
   _ocl_context.Launch3D(0, gs, ls, rays_kernel);
   
   int pass_count = 4000;
   int progress = 0;
   for (int pass = 0; pass < pass_count; ++pass)
   {      
      vec3 pass_direction = glm::sphericalRand(1.0f);

      CLWKernel rays_kernel = _raytracer_prog.GetKernel("updateRaysDirection");
      rays_kernel.SetArg(0, _toCL(pass_direction));
      rays_kernel.SetArg(1, ray_count);
      rays_kernel.SetArg(2, ocl_ray_buffer);
      _ocl_context.Launch1D(0, ray_count, 1, rays_kernel);

      _api->QueryOcclusion(ray_buffer, ray_count, isect_buffer, nullptr, nullptr);

      CLWKernel accumulate_kernel = _raytracer_prog.GetKernel("accumulateAO");
      accumulate_kernel.SetArg(0, ray_count);
      accumulate_kernel.SetArg(1, ocl_intersection_buffer);
      accumulate_kernel.SetArg(2, ocl_ao_volume_buffer);
      _ocl_context.Launch1D(0, ray_count, 1, accumulate_kernel);

      int new_progress = int(float(pass) / float(pass_count) * 100.0f);
      if (new_progress != progress)
      {
         progress = new_progress;
         std::cout << "Submission progress: " << progress << "%\r";
      }
   }

   float* ao_texture = nullptr;
   _ocl_context.MapBuffer(0, ocl_ao_volume_buffer, CL_MAP_READ, &ao_texture).Wait();
   #pragma omp parallel for
   for (int i = 0; i < voxel_count; ++i)
   {
      float voxel_value = ao_texture[i];
      float ao = 2.0f*(1.0f - voxel_value / float(pass_count));
      (final_ao_texture.get())[i] = (unsigned char)(clamp(sqrtf(ao) * 255.0f, 0.0f, 255.0f));
   }
   _ocl_context.UnmapBuffer(0, ocl_ao_volume_buffer, ao_texture).Wait();

   scene.ao_volume->ao_texture = createTexture3D(resolution.x, resolution.y, resolution.z, GL_R8, final_ao_texture.get());
}

void Raytracer::bakeSignedDistanceFieldVolume(Scene& scene)
{
   SDFVolume& sdf_volume = *scene.sdf_volume;
   ivec3 resolution = sdf_volume.resolution;
   int ray_count = resolution.x*resolution.y*resolution.z;
   int voxel_count = ray_count;
   
   auto ocl_ray_buffer = _ocl_context.CreateBuffer<ray>(ray_count, CL_MEM_READ_WRITE);
   auto ray_buffer = _api->CreateFromOpenClBuffer(ocl_ray_buffer);

   auto ocl_intersection_buffer = _ocl_context.CreateBuffer<Intersection>(ray_count, CL_MEM_READ_WRITE);
   auto isect_buffer = _api->CreateFromOpenClBuffer(ocl_intersection_buffer);

   auto ocl_sdf_volume_accumulation = _ocl_context.CreateBuffer<float3>(ray_count, CL_MEM_READ_WRITE);
   _ocl_context.FillBuffer(0, ocl_sdf_volume_accumulation, float3(FLT_MAX, 0.0f, 0.0f), ray_count);

   CLWKernel rays_kernel = _raytracer_prog.GetKernel("initRays");
   rays_kernel.SetArg(0, _toCL(resolution));
   rays_kernel.SetArg(1, _toCL(sdf_volume.size));
   rays_kernel.SetArg(2, _toCL(sdf_volume.position));
   rays_kernel.SetArg(3, ray_count);
   rays_kernel.SetArg(4, ocl_ray_buffer);
   size_t gs[] = { resolution.x, resolution.y, resolution.z };
   size_t ls[] = { 1, 1, 1 };
   _ocl_context.Launch3D(0, gs, ls, rays_kernel);

   int pass_count = 1000;
   int progress = 0;
   for (int pass = 0; pass < pass_count; ++pass)
   {
      vec3 pass_direction = glm::sphericalRand(1.0f);

      CLWKernel rays_kernel = _raytracer_prog.GetKernel("updateRaysDirection");
      rays_kernel.SetArg(0, _toCL(pass_direction));
      rays_kernel.SetArg(1, ray_count);
      rays_kernel.SetArg(2, ocl_ray_buffer);
      _ocl_context.Launch1D(0, ray_count, 1, rays_kernel);

      _api->QueryIntersection(ray_buffer, ray_count, isect_buffer, nullptr, nullptr);

      CLWKernel accumulate_kernel = _raytracer_prog.GetKernel("accumulateSDF");
      accumulate_kernel.SetArg(0, ray_count);
      accumulate_kernel.SetArg(1, _toCL(pass_direction));
      accumulate_kernel.SetArg(2, ocl_intersection_buffer);
      accumulate_kernel.SetArg(3, _ocl_face_normal_buffer);
      accumulate_kernel.SetArg(4, ocl_sdf_volume_accumulation);
      _ocl_context.Launch1D(0, ray_count, 1, accumulate_kernel);

      int new_progress = int(float(pass) / float(pass_count) * 100.0f);
      if (new_progress != progress)
      {
         progress = new_progress;
         std::cout << "Submission progress: " << progress << "%\r";
      }
   }

   auto final_sdf_texture = std::make_unique<float[]>(voxel_count);
   float3* mapped_accumulated_sdf = nullptr;
   _ocl_context.MapBuffer(0, ocl_sdf_volume_accumulation, CL_MAP_READ, &mapped_accumulated_sdf).Wait();
   #pragma omp parallel for
   for (int i = 0; i < voxel_count; ++i)
   {
      float3 voxel_value = mapped_accumulated_sdf[i];
      if (voxel_value.z == 0.0)
         int a = 0;

      float average_face_hit_side = voxel_value.z > 0 ? voxel_value.y/voxel_value.z : 1.0f;
      float signed_distance = average_face_hit_side > 0.0 ? voxel_value.x : -voxel_value.x;
      (final_sdf_texture.get())[i] = average_face_hit_side > 0 ? voxel_value.x : -voxel_value.x;
   }
   _ocl_context.UnmapBuffer(0, ocl_sdf_volume_accumulation, mapped_accumulated_sdf).Wait();

   scene.sdf_volume->sdf_texture = createTexture3D(resolution.x, resolution.y, resolution.z, GL_R16F, final_sdf_texture.get());
}

void Raytracer::_wait()
{
   _status_event->Wait();
   _api->DeleteEvent(_status_event);
}


}
