#include "TextureImporter.h"

#include "FreeImage.h"

#include "GLTexture.h"

namespace yare { namespace TextureImporter {

Uptr<GLTexture> importTextureFromFile(const char* filename)
{
    FREE_IMAGE_FORMAT format = FreeImage_GetFileType(filename, 0);
    FIBITMAP* image = FreeImage_Load(format, filename);
    FIBITMAP* image_rgba8 = FreeImage_ConvertTo32Bits(image);
    FreeImage_Unload(image);

    int width = FreeImage_GetWidth(image_rgba8);
    int height = FreeImage_GetHeight(image_rgba8);

    char* pixels = (char*)FreeImage_GetBits(image_rgba8);
    bool is_bgr_image = true;
    Uptr<GLTexture> texture = createMipmappedTexture2D(width, height, GL_RGBA8, pixels, is_bgr_image);

    FreeImage_Unload(image_rgba8);

    return texture;
}

}}




