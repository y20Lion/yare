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

    glm::mat3 normal_matrix_world_local;    
    Sptr<GLVertexSource> vertex_source_for_material;
};

enum class LightType { Sphere = 0, Rectangle = 1, Sun = 2, Spot = 3 };

/*struct LightSphereData
{

};

struct LightSunData
{
   glm::vec3 direction;
   float size;
};

struct LightRectangleData
{   
   glm::vec3 position;
   float padding;
   glm::vec3 direction_x;
   float size_x;
   glm::vec3 direction_y;
   float size_y;
};

struct LightSunData
{
   glm::vec3 direction;
   float size;
};

struct LightSpotData
{
   glm::vec3 position;
   float size_blend;
};

struct Light
{
    LightType type;
    glm::vec3 color;    
    union
    {
       LightSphereData sphere;
       LightRectangleData rectangle;
       LightSunData sun;
       LightSpotData spot;
    };    
};*/

struct LightSphereData
{
   float size;
};

struct LightRectangleData
{
   float size_x;
   float size_y;
};

struct LightSunData
{
   float size;
};

struct LightSpotData
{
   float angle;
   float angle_blend;
};

struct Light
{
   LightType type;
   glm::vec3 color;
   float strength;
   glm::mat4x3 world_to_local_matrix;
   
   union
   {
      LightSphereData sphere;
      LightRectangleData rectangle;
      LightSunData sun;
      LightSpotData spot;
   };
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
   Uptr<GLTextureCubemap> sky_diffuse_cubemap_sh;

   RenderData render_data[2];
};


}