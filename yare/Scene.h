#pragma once

#include <map>
#include <vector>
#include <glm/mat4x3.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>

#include "tools.h"
#include "Camera.h"
#include "GLVertexSource.h"
#include "IMaterial.h"
#include "Transform.h"
#include "SurfaceRange.h"

namespace yare {

using namespace glm;

class IMaterial;
class RenderMesh;
class Skeleton;
class GLTextureCubemap;
class GLTexture2D;
class GLTexture3D;
class AnimationPlayer;
class TransformHierarchy;

struct SurfaceInstance
{
   int transform_node_index;
   vec3 center_in_local_space;
   Sptr<RenderMesh> mesh;
   Sptr<Skeleton> skeleton;
   Sptr<IMaterial> material;
   MaterialVariant material_variant;
   const GLProgram* material_program;
        
   Sptr<GLVertexSource> vertex_source_for_material;
   Sptr<GLVertexSource> vertex_source_position_normal;
};

enum class LightType { Sphere = 0, Rectangle = 1, Sun = 2, Spot = 3 };

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
   float radius;

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
    glm::mat4 matrix_proj_local;
    glm::mat4 matrix_world_local;
    glm::mat3 normal_matrix_world_local;
};

struct SurfaceDistanceSortItem
{
   int surface_index;
   float distance;
};

struct RenderData
{
   std::vector<MainViewSurfaceData> main_view_surface_data;
   
   std::vector<SurfaceDistanceSortItem> surfaces_sorted_by_distance;
   glm::mat4x4 matrix_proj_world;
   glm::mat4x4 matrix_view_proj;
   glm::mat4x4 matrix_proj_view;
   glm::mat4x4 matrix_view_world;
   Frustum frustum;

   vec3 points[4];
};

struct AOVolume
{
   int transform_node_index;
   vec3 position;
   vec3 size;
   ivec3 resolution;
   Uptr<GLTexture3D> ao_texture;  
};

struct SDFVolume
{
   int transform_node_index;
   vec3 position;
   vec3 size;
   ivec3 resolution;
   Uptr<GLTexture3D> sdf_texture;
};

class Scene
{
public:
   Scene();

   Scene(const Scene& other) = default;
   ~Scene();

   std::map<std::string, Skeleton*> name_to_skeleton;
   std::map<std::string, int> object_name_to_transform_node_index;

   Camera camera;
   Uptr<AOVolume> ao_volume;
   Uptr<SDFVolume> sdf_volume;
   std::vector<Sptr<Skeleton>> skeletons;
   std::vector<Sptr<IMaterial>> materials;   
   std::vector<Light> lights;
   std::vector<SurfaceInstance> surfaces;
   SurfaceRange opaque_surfaces;
   SurfaceRange transparent_surfaces;

   Uptr<GLTextureCubemap> sky_cubemap;
   Uptr<GLTexture2D> sky_latlong;
   Uptr<GLTextureCubemap> sky_diffuse_cubemap;
   Uptr<GLTextureCubemap> sky_diffuse_cubemap_sh;

   Uptr<AnimationPlayer> animation_player;
   Uptr<TransformHierarchy> transform_hierarchy;

   RenderData render_data[2];
};


}