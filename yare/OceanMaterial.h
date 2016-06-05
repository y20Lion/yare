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

   virtual void render(const RenderResources& resources, const GLVertexSource& mesh_source) override;
   virtual int requiredMeshFields() override;

private:
   DISALLOW_COPY_AND_ASSIGN(OceanMaterial)
   Uptr<GLProgram> _program;   
};

}