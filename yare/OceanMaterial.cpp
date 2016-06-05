#include "OceanMaterial.h"

#include "GLProgram.h"
#include "RenderMesh.h"
#include "GLDevice.h"
#include "GLVertexSource.h"
#include "glsl_binding_defines.h"

namespace yare {

OceanMaterial::OceanMaterial()
{
   _program = createProgramFromFile("OceanMaterial.glsl");
}

OceanMaterial::~OceanMaterial()
{

}

void OceanMaterial::render(const RenderResources& resources, const GLVertexSource& mesh_source)
{
   GLDevice::bindProgram(program());
   GLDevice::bindVertexSource(mesh_source);
   glDrawArrays(GL_PATCHES, 0, mesh_source.vertexCount());
}

int OceanMaterial::requiredMeshFields()
{
   return int(MeshFieldName::Position) | int(MeshFieldName::Normal);
}



}