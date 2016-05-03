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

class ShadeTreeMaterial;
class RenderMesh;

struct SurfaceInstance
{
    glm::mat4x3 matrix_world_local;    
    Sptr<RenderMesh> mesh;
    Sptr<ShadeTreeMaterial> material;
    std::map<int, Sptr<GLVertexSource>> vertex_sources;
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
    glm::mat3 normal_matrix_world_local;
    Sptr<GLVertexSource> vertex_source;
};

class Scene
{
public:
    
    void update();

    Camera camera;
    std::vector<SurfaceInstance> surfaces;
    std::vector<Light> lights;

    std::vector<MainViewSurfaceData> main_view_surface_data;
    glm::mat4x4 _matrix_view_world;
};

Scene createBasicScene();

}