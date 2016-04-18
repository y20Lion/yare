#pragma once

#include <glm/vec3.hpp>
#include <glm/fwd.hpp>

struct PointOfView
{
    PointOfView();
    glm::vec3 from;
    glm::vec3 to;
    glm::vec3 up;

    glm::mat4 lookAtMatrix() const;
    glm::vec3 rightDirection() const;
};

struct Frustum
{
    Frustum();
    float left, right, bottom, top, near, far;
};

class Camera
{
public:
    Frustum frustum;
    PointOfView point_of_view;
};