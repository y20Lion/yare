#pragma once

#include <string>

#include "tools.h"

namespace yare {
   
class GLTexture2D;
class GLTextureCubemap;
class CubemapFiltering;

namespace TextureImporter 
{
    Uptr<GLTexture2D> importTextureFromFile(const char* filename, bool float_pixels = false);
}

}