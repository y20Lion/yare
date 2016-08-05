#include "GLDevice.h"

#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>

#include "GLFramebuffer.h"
#include "GLProgram.h"
#include "GLVertexSource.h"
#include "GLTexture.h"
#include "GLSampler.h"

namespace yare { namespace GLDevice {

void bindUniformMatrix4(GLint location, const mat4& matrix)
{
   glUniformMatrix4fv(location, 1, false, value_ptr(matrix));
}

void bindFramebuffer(const GLFramebuffer* framebuffer, int color_attachment)
{
   if (framebuffer)
   {
      glNamedFramebufferDrawBuffer(framebuffer->id(), GL_COLOR_ATTACHMENT0 + color_attachment);
      glBindFramebuffer(GL_FRAMEBUFFER_EXT, framebuffer->id());
   }
   else
   {
      glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
   }
}

void bindProgram(const GLProgram& program)
{
    glUseProgram(program.id());
}

void bindVertexSource(const GLVertexSource& vertex_source)
{
    glBindVertexArray(vertex_source.id()); 
}

void bindTexture(int texture_unit, const GLTexture& texture, const GLSampler& sampler)
{
   GLuint texture_id = texture.id();
   GLuint sampler_id = sampler.id();

   glBindTextures(texture_unit, 1, &texture_id);
   glBindSamplers(texture_unit, 1, &sampler_id);
}

void bindImage(int image_unit, const GLTexture2D& texture, GLenum access)
{
   glBindImageTexture(image_unit, texture.id(), 0, false, 0, access, texture.internalFormat());
}

void bindDepthStencilState(const GLDepthStencilState& state)
{
   if (state.depth_testing)
   {
      glEnable(GL_DEPTH_TEST);
      glDepthFunc(state.depth_function);
   }
   else
      glDisable(GL_DEPTH_TEST);
}

void bindDefaultDepthStencilState()
{
   glEnable(GL_DEPTH_TEST);
   glDepthFunc(GL_LEQUAL);
}

void bindColorBlendState(const GLColorBlendState& state)
{
   if (state.blending_mode == GLBlendingMode::ModulateAdd)
   {
      glEnable(GL_BLEND);
      glBlendFunc(GL_ONE, GL_SRC1_COLOR);
      glBlendEquation(GL_FUNC_ADD);
   }
   else
   {
      glDisable(GL_BLEND);
   }
}

void bindDefaultColorBlendState()
{
   glDisable(GL_BLEND);
}

void bindRasterizationState(const GLRasterizationState& state)
{
   glPolygonMode(GL_FRONT_AND_BACK, state.polygon_mode);
   glCullFace(state.cull_face);
}

void bindDefaultRasterizationState()
{
   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
   glCullFace(GL_BACK);
}

void draw(int vertex_start, int vertex_count)
{    
    glDrawArrays(GL_TRIANGLES, vertex_start, vertex_count);
}

void draw(const GLVertexSource& vertex_source)
{
   glBindVertexArray(vertex_source.id());
   glDrawArrays(GL_TRIANGLES, 0, vertex_source.vertexCount());
}

}}
