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

class DefaultMaterial : public IMaterial
{
public:
DefaultMaterial();
virtual ~DefaultMaterial();

const GLProgram& program() const { return *_program; }

virtual int requiredMeshFields(MaterialVariant material_variant) override;
virtual const GLProgram& compile(MaterialVariant material_variant) override { return *_program; }
virtual void bindTextures() override { }
virtual bool isTransparent() override { return false; }
virtual bool hasTessellation() override { return false; }

private:
DISALLOW_COPY_AND_ASSIGN(DefaultMaterial)
   Uptr<GLProgram> _program;
};


}
