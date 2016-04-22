#pragma once

#include <vector>
#include <glm/mat4x3.hpp>
#include <glm/mat4x4.hpp>

#include "tools.h"
#include "Camera.h"
#include "GLVertexSource.h"

struct SurfaceInstance
{
    glm::mat4x3 matrix_world_local;    
    Sptr<GLVertexSource> mesh;
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

struct SurfaceRenderData
{
    glm::mat4x4 matrix_view_local;
};

class Scene
{
public:
    
    void update();

    Camera camera;
    std::vector<SurfaceInstance> surfaces;
    std::vector<Light> lights;

    std::vector<SurfaceRenderData> surfaces_render_data;
    glm::mat4x4 _matrix_view_world;
};

Scene createBasicScene();

