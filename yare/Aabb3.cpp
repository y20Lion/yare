#include "Aabb3.h"

#include <limits>
#include <algorithm>
#include <glm/common.hpp>

namespace yare {

Aabb3::Aabb3()
{
   setNull();
}

void Aabb3::extend(const vec3& p)
{
   pmin = glm::min(p, pmin);
   pmax = glm::max(p, pmax);
}

void Aabb3::setNull()
{ 
   pmin = vec3(std::numeric_limits<float>::infinity());
   pmax = vec3(-std::numeric_limits<float>::infinity());
}

bool Aabb3::isNull() const
{
   return pmin.x > pmax.x || pmin.y > pmax.y || pmin.z > pmax.z;
}

}