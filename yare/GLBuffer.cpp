#include "GLBuffer.h"

GLBuffer::GLBuffer(const GLBufferDesc& desc)
{
    glCreateBuffers(1, &_buffer_id);
    glNamedBufferStorage(_buffer_id, desc.size_bytes, nullptr, desc.flags);
}

GLBuffer::~GLBuffer()
{
    glDeleteBuffers(1, &_buffer_id);
}

void* GLBuffer::map(GLenum access)
{
    return glMapNamedBuffer(_buffer_id, access);
}

void GLBuffer::unmap()
{
    glUnmapNamedBuffer(_buffer_id);
}

GLBufferUptr createVertexBuffer(std::int64_t size_bytes)
{
    GLBufferDesc desc;
    desc.flags = GL_MAP_WRITE_BIT;
    desc.size_bytes = size_bytes;
    return GLBufferUptr(new GLBuffer(desc));
}