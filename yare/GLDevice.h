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

enum class GLBlendingMode { Disabled, ModulateAdd };
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

    /*void blit(const GLRenderTarget& src_render_target, int attachment, GLRenderTarget& dst_render_target, int attachment);
    void blitDepth(const GLRenderTarget& src_render_target, GLRenderTarget& dst_render_target);
    void read(const GLRenderTarget& render_target, int attachment);*/
   void bindUniformMatrix4(GLint location, const mat4& matrix);
      

   // state setting
   void bindFramebuffer(const GLFramebuffer* framebuffer, int color_attachment);
   void bindProgram(const GLProgram& program);
   void bindVertexSource(const GLVertexSource& vertex_source);
   void bindTexture(int texture_unit, const GLTexture& texture, const GLSampler& sampler);
   void bindImage(int image_unit, const GLTexture2D& texture, GLenum access);

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

