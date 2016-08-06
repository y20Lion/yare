#include "OceanMaterial.h"

#include "GLProgram.h"
#include "RenderMesh.h"
#include "GLDevice.h"
#include "GLVertexSource.h"
#include "glsl_global_defines.h"

namespace yare {

OceanMaterial::OceanMaterial()
{
   _program = createProgramFromFile("OceanMaterial.glsl");
}

OceanMaterial::~OceanMaterial()
{

}

int OceanMaterial::requiredMeshFields(MaterialVariant material_variant)
{
   return int(MeshFieldName::Position) | int(MeshFieldName::Normal);
}



}