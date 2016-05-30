#pragma once

#include <string>

#include "tools.h"

namespace yare {
   
class GLTexture2D;
class GLTextureCubemap;
class CubemapConverter;

namespace TextureImporter 
{
    Uptr<GLTexture2D> importTextureFromFile(const char* filename, bool float_pixels = false);
    Uptr<GLTextureCubemap> importCubemapFromFile(const char* filename, const CubemapConverter& latlong_converter);
}

}