#include "GLFormats.h"


#include <assert.h>
#include <algorithm>

namespace yare {

int GLFormats::mipmapLevelCount(int width, int height)
{
   int dimension = std::max(width, height);
   int i = 0;
   while (dimension > 0)
   {
      dimension >>= 1;
      i++;
   }
   return i;
}

int GLFormats::componentCount(GLenum components)
{
   switch (components)
   {
   case GL_BGRA:
   case GL_RGBA:
      return 4;
   case GL_BGR:
   case GL_RGB:
      return 3;
   case GL_RG:
      return 2;
   case GL_RED:
      return 1;
   }
   assert(false);
   return 0;
}

int GLFormats::sizeOfType(GLuint type)
{
   switch (type)
   {
   case GL_BYTE:
   case GL_UNSIGNED_BYTE:
      return 1;
   case GL_SHORT:
   case GL_UNSIGNED_SHORT:
   case GL_HALF_FLOAT:
      return 2;
   case GL_INT:
   case GL_UNSIGNED_INT:
   case GL_FLOAT:
      return 4;
   case GL_DOUBLE:
      return 8;
   default:
      assert(false);
      return 0;
   }
}

}