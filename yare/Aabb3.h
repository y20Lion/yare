#pragma once

#include <glm/vec3.hpp>

namespace yare {

using namespace glm;

class Aabb3
{
public:
   Aabb3();
   Aabb3(const vec3& pmin, const vec3& pmax): pmin(pmin), pmax(pmax) {}

   void extend(const vec3& p);

   void setNull();
   bool isNull() const;

   vec3 pmin;
   vec3 pmax;
};



}