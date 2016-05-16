#include "OceanMaterial.h"

#include "GLProgram.h"

namespace yare {

OceanMaterial::OceanMaterial()
{
   _program = createProgramFromFile("OceanMaterial.glsl");
}

OceanMaterial::~OceanMaterial()
{

}




}