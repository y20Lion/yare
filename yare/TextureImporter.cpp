#include "TextureImporter.h"

#include "FreeImage.h"

#include "GLTexture.h"
#include "GLSampler.h"
#include "GLDevice.h"
#include "GLProgram.h"
#include "GLVertexSource.h"
#include "GLBuffer.h"
#include "GLFramebuffer.h"
#include "glsl_binding_defines.h"
#include "GLTexture.h"
#include "LatlongToCubemapConverter.h"

namespace yare { namespace TextureImporter {

Uptr<GLTexture2D> importTextureFromFile(const char* filename, bool float_pixels)
{
   FREE_IMAGE_FORMAT format = FreeImage_GetFileType(filename, 0);
   FIBITMAP* image = FreeImage_Load(format, filename);
   FIBITMAP* prepared_image = nullptr;
   if (float_pixels)
      prepared_image = FreeImage_ConvertToType(image, FIT_RGBF);
   else
      prepared_image = FreeImage_ConvertTo32Bits(image);
   FreeImage_Unload(image);

   int width = FreeImage_GetWidth(prepared_image);
   int height = FreeImage_GetHeight(prepared_image);

   char* pixels = (char*)FreeImage_GetBits(prepared_image);
   bool is_bgr_image = !float_pixels;
   GLenum internal_format = float_pixels ? GL_RGB16F : GL_RGBA8;
   Uptr<GLTexture2D> texture = createMipmappedTexture2D(width, height, internal_format, pixels, is_bgr_image);

   FreeImage_Unload(prepared_image);

   return texture;
}

float quad_vertices[] = {1.0f,1.0f,   -1.0f,1.0f,   1.0f,-1.0f,  -1.0f,1.0f,   -1.0f,-1.0f,  1.0f,-1.0f};

Uptr<GLTextureCubemap> TextureImporter::importCubemapFromFile(const char* filename, const LatlongToCubemapConverter& latlong_converter)
{
   Uptr<GLTexture2D> latlong_texture = importTextureFromFile(filename, true);
   return latlong_converter.convert(*latlong_texture);
}

}}




