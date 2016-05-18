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

int GLPersistentlyMappedBuffer::_window_count = 4;
int GLPersistentlyMappedBuffer::_window_index = 0;

static const int _persistent_map_flags = GL_MAP_WRITE_BIT | GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT;

GLPersistentlyMappedBuffer::GLPersistentlyMappedBuffer(const GLPersistentlyMappedBufferDesc& desc)
 : GLBuffer(GLBufferDesc(desc.window_size_bytes*_window_count, nullptr, _persistent_map_flags | GL_DYNAMIC_STORAGE_BIT))
{
   _head_ptr = (char*)glMapNamedBufferRange(_buffer_id, 0, desc.window_size_bytes*_window_count, _persistent_map_flags);
   _window_size = desc.window_size_bytes;
}

GLPersistentlyMappedBuffer::~GLPersistentlyMappedBuffer()
{

}

void* GLPersistentlyMappedBuffer::getCurrentWindowPtr()
{
   return _head_ptr + (_window_index*windowSize());
}

std::int64_t GLPersistentlyMappedBuffer::getCurrentWindowOffset()
{
   return _window_index*windowSize();
}

Uptr<GLBuffer> createBuffer(std::int64_t size_bytes, void* data)
{
    GLBufferDesc desc;
    desc.flags = GL_MAP_WRITE_BIT;
    desc.size_bytes = size_bytes;
    desc.data = data;
    return std::make_unique<GLBuffer>(desc);
}

Uptr<GLPersistentlyMappedBuffer> createPersistentBuffer(std::int64_t size_bytes)
{
   GLPersistentlyMappedBufferDesc desc;
   desc.window_size_bytes = size_bytes;
   return std::make_unique<GLPersistentlyMappedBuffer>(desc);
}

}