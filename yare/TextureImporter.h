#pragma once

#include <string>

#include "tools.h"

namespace yare {


class GLTexture;
namespace TextureImporter 
{
    Uptr<GLTexture> importTextureFromFile(const char* filename);

}

}