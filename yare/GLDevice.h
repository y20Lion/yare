#pragma once

namespace yare { 

class GLBuffer;
class GLProgram;
class GLProgramResources;
class GLVertexSource;
class GLRenderTarget;

namespace GLDevice 
{

    /*void blit(const GLRenderTarget& src_render_target, int attachment, GLRenderTarget& dst_render_target, int attachment);
    void blitDepth(const GLRenderTarget& src_render_target, GLRenderTarget& dst_render_target);
    void read(const GLRenderTarget& render_target, int attachment);*/

    // state setting
    void setCurrentRenderTarget();
    void setCurrentProgram(const GLProgram& program);
    void setCurrentProgramResources(const GLProgramResources& program_resources);
    void setCurrentVertexSource(const GLVertexSource& vertex_source);

    // draw calls
    void drawIndexed();
    void draw(int vertex_start, int vertex_count);
}

}

