#pragma once

#include <GL/glew.h>
#include "tools.h"

struct GLRenderTargetAttachmentDesc
{
    std::vector<GLTexture> texture;
    type;
}

struct GLRenderTargetDesc
{
    std::vector<GLRenderTargetAttachmentDesc> attachments;    
};

class GLRenderTarget
{
public:
    GLRenderTarget(const GLRenderTargetDesc& desc);
    virtual ~GLRenderTarget();
    GLuint id() const { return _render_target_id; }



private:
    DISALLOW_COPY_AND_ASSIGN(GLRenderTarget)
    GLuint _render_target_id;
};
DECLARE_STD_PTR(GLRenderTarget)

class GLDefaultRT : private GLRenderTarget