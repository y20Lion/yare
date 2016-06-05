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

virtual void render(const RenderResources& resources, const GLVertexSource& mesh_source) override;
virtual int requiredMeshFields() override;

private:
DISALLOW_COPY_AND_ASSIGN(DefaultMaterial)
   Uptr<GLProgram> _program;
};


}
