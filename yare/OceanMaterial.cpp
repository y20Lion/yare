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

void OceanMaterial::render(const GLVertexSource& mesh_source, const GLProgram& program_)
{
   GLDevice::bindProgram(program());
   GLDevice::bindVertexSource(mesh_source);
   glDrawArrays(GL_PATCHES, 0, mesh_source.vertexCount());
}

int OceanMaterial::requiredMeshFields(MaterialVariant material_variant)
{
   return int(MeshFieldName::Position) | int(MeshFieldName::Normal);
}



}