#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <GL/glew.h>

#include "tools.h"

namespace yare {

class GLProgram;
struct RenderResources;

class OceanMaterial
{
public:
   OceanMaterial();
   virtual ~OceanMaterial();

   const GLProgram& program() const { return *_program; }

private:
   DISALLOW_COPY_AND_ASSIGN(OceanMaterial)
   Uptr<GLProgram> _program;   
};

}