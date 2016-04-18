#pragma once

#include <GL/glew.h>
#include <cstdint>

#include "tools.h"

struct GLBufferDesc
{
    std::int64_t size_bytes;
    GLbitfield flags;
};

class GLBuffer
{
public:
    GLBuffer(const GLBufferDesc& desc);
    virtual ~GLBuffer();
    GLuint id() const { return _buffer_id; }

    void* map(GLenum access);
    void unmap();

private:
    DISALLOW_COPY_AND_ASSIGN(GLBuffer)
    GLuint _buffer_id;
};
DECLARE_STD_PTR(GLBuffer)

GLBufferUptr createVertexBuffer(std::int64_t size_bytes);