#pragma once

#include <GL/glew.h>

namespace yare {  

   namespace GLFormats
   {
      int mipmapLevelCount(int width, int height);
      int sizeOfType(GLuint type);
      int componentCount(GLenum components);
   }

}
