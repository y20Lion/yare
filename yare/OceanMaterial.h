#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <GL/glew.h>

#include "tools.h"
#include "IMaterial.h"

namespace yare {

class GLProgram;
struct RenderResources;

class OceanMaterial : public IMaterial
{
public:
   OceanMaterial();
   virtual ~OceanMaterial();

   const GLProgram& program() const { return *_program; }

   virtual int requiredMeshFields(MaterialVariant material_variant) override;
   virtual const GLProgram& compile(MaterialVariant material_variant) override { return *_program; }
   virtual void bindTextures() override { }
   virtual bool isTransparent() override { return false;  }
   virtual bool hasTessellation() override { return true; }

private:
   DISALLOW_COPY_AND_ASSIGN(OceanMaterial)
   Uptr<GLProgram> _program;   
};

}