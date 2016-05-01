#include "GLProgramResources.h"

#include "GLTexture.h"

namespace yare {

GLProgramResources::GLProgramResources()
{

}

GLProgramResources::~GLProgramResources()
{

}

void GLProgramResources::setTexture(int bind_slot, const GLTexture& texture)
{
    _bind_slot_to_texture[bind_slot] = texture.id();
}

void GLProgramResources::bindResources() const
{
    for (const auto& slot_texture_pair : _bind_slot_to_texture)
        glBindTextures(slot_texture_pair.first, 1, &slot_texture_pair.second);
}

}