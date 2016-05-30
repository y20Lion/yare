#pragma once

#include <map>
#include <vector>
#include <glm/mat4x3.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>

#include "tools.h"
#include "Camera.h"
#include "GLVertexSource.h"

namespace yare {

class IMaterial;
class RenderMesh;
class GLTextureCubemap;

struct SurfaceInstance
{
    glm::mat4x3 matrix_world_local;    
    Sptr<RenderMesh> mesh;
    Sptr<IMaterial> material;    
    std::map<int, Sptr<GLVertexSource>> vertex_sources;

    glm::mat3 normal_matrix_world_local;    
    Sptr<GLVertexSource> vertex_source_for_material;
};

enum class LightType { Point, Spot };

struct Light
{
    LightType type;
    glm::vec3 colour;
    glm::vec3 position;    
    float intensity;
    float radius;
    float angle;
    glm::vec3 direction;
};

struct MainViewSurfaceData
{
    glm::mat4 matrix_view_local;    
};

struct RenderData
{
   std::vector<MainViewSurfaceData> main_view_surface_data;
   glm::mat4x4 matrix_view_world;
};

class Scene
{
public:
   Scene() {}

   Scene(const Scene& other) = default;
   ~Scene();
   
   Camera camera;
   std::vector<SurfaceInstance> surfaces;
   std::vector<Light> lights;
   Uptr<GLTextureCubemap> sky_cubemap;
   Uptr<GLTextureCubemap> sky_diffuse_cubemap;

   RenderData render_data[2];
};


}