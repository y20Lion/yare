#include "GLBuffer.h"

namespace yare {

GLBuffer::GLBuffer(const GLBufferDesc& desc)
: _size_bytes(desc.size_bytes)
{
    glCreateBuffers(1, &_buffer_id);
    glNamedBufferStorage(_buffer_id, desc.size_bytes, desc.data, desc.flags);
}

GLBuffer::~GLBuffer()
{
    glDeleteBuffers(1, &_buffer_id);
}

void* GLBuffer::map(GLenum access)
{
    return glMapNamedBuffer(_buffer_id, access);
}

void* GLBuffer::mapRange(std::int64_t offset, std::int64_t length, GLenum access)
{
	return glMapNamedBufferRange(_buffer_id, offset, length, access);
}

void GLBuffer::unmap()
{
    glUnmapNamedBuffer(_buffer_id);
}

Uptr<GLBuffer> createBuffer(std::int64_t size_bytes, void* data)
{
    GLBufferDesc desc;
    desc.flags = GL_MAP_WRITE_BIT;
    desc.size_bytes = size_bytes;
    desc.data = data;
    return std::make_unique<GLBuffer>(desc);
}

}