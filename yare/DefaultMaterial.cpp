#include "DefaultMaterial.h"
#include "RenderMesh.h"
#include "GLDevice.h"
#include "GLVertexSource.h"
#include "GLProgram.h"

namespace yare {

DefaultMaterial::DefaultMaterial()
{
   _program = createProgramFromFile("DefaultMaterial.glsl");
}

DefaultMaterial::~DefaultMaterial()
{

}

int DefaultMaterial::requiredMeshFields(MaterialVariant material_variant)
{
   return int(MeshFieldName::Position) | int(MeshFieldName::Normal);
}


}