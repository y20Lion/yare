#pragma once

#include <GL/glew.h>
#include <glm/fwd.hpp>
namespace yare { 

using namespace glm;

class GLBuffer;
class GLProgram;
class GLProgramResources;
class GLVertexSource;
class GLFramebuffer;
class GLSampler;
class GLTexture;
class GLTexture2D;

struct GLDepthStencilState
{  
   bool depth_testing;
   GLenum depth_function;   
};

enum class GLBlendingMode { Disabled, ModulateAdd, Add };
struct GLColorBlendState
{ 
   GLBlendingMode blending_mode;
};

struct GLRasterizationState
{  
   GLenum polygon_mode;
   GLenum cull_face;
};

namespace GLDevice 
{   
   void bindUniformMatrix4(GLint location, const mat4& matrix);
   void bindUniformMatrix4Array(GLint location, const mat4* matrices, int count);
      
   // state setting
   void bindFramebuffer(const GLFramebuffer* framebuffer, int color_attachment);
   void bindProgram(const GLProgram& program);
   void bindVertexSource(const GLVertexSource& vertex_source);
   void bindTexture(int texture_unit, const GLTexture& texture, const GLSampler& sampler);
   void bindImage(int image_unit, const GLTexture& texture, GLenum access);

   void bindDepthStencilState(const GLDepthStencilState& state);
   void bindDefaultDepthStencilState();

   void bindColorBlendState(const GLColorBlendState& state);
   void bindDefaultColorBlendState();

   void bindRasterizationState(const GLRasterizationState& state);
   void bindDefaultRasterizationState();    
    
   // draw calls
   void draw(int vertex_start, int vertex_count);
   void draw(const GLVertexSource& vertex_source);
}

}

