#pragma once

namespace yare { 

class GLBuffer;
class GLProgram;
class GLProgramResources;
class GLVertexSource;
class GLFramebuffer;
class GLSampler;
class GLTexture;

namespace GLDevice 
{

    /*void blit(const GLRenderTarget& src_render_target, int attachment, GLRenderTarget& dst_render_target, int attachment);
    void blitDepth(const GLRenderTarget& src_render_target, GLRenderTarget& dst_render_target);
    void read(const GLRenderTarget& render_target, int attachment);*/

    // state setting
    void bindFramebuffer(const GLFramebuffer* framebuffer, int color_attachment);
    void bindProgram(const GLProgram& program);
    void bindVertexSource(const GLVertexSource& vertex_source);
    void bindTexture(int texture_unit, const GLTexture& texture, const GLSampler& sampler);
    void bindBlendingState();
    void bindRasterState();
    // draw calls
    void draw(int vertex_start, int vertex_count);
    void draw(const GLVertexSource& vertex_source);
}

}

