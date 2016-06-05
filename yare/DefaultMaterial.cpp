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

void DefaultMaterial::render(const RenderResources& resources, const GLVertexSource& mesh_source)
{
   GLDevice::bindProgram(program());
   GLDevice::bindVertexSource(mesh_source);
   GLDevice::draw(0, mesh_source.vertexCount());
}

int DefaultMaterial::requiredMeshFields()
{
   return int(MeshFieldName::Position) | int(MeshFieldName::Normal);
}


}