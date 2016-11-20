#include "Camera.h"

#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace yare {

using namespace glm;

PointOfView::PointOfView()
    : from(vec3(3.0, 0.0, 0.0))
    , to(vec3(0.0, 0.0, 0.0))
    , up(vec3(0.0, 0.0, 1.0))
{
}

mat4 PointOfView::lookAtMatrix() const
{
    return lookAt(from, to, up);
}

vec3 PointOfView::rightDirection() const
{
    return normalize(cross(to - from, up));
}

Frustum::Frustum()
    : left(-1.0f)
    , right(1.0f)
    , bottom(-1.0f)
    , top(1.0f)
    , near(0.05f)
    , far(100.0f)
{
}

}