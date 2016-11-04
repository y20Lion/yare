#pragma once

#include <glm/vec3.hpp>

namespace yare {

using namespace glm;

class Aabb3
{
public:
   Aabb3();

   void extend(const vec3& p);

   void setNull();
   bool isNull() const;

   vec3 pmin;
   vec3 pmax;
};



}