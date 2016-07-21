#include "GLBuffer.h"

#include "GLFormats.h"

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
    return glMapNamedBufferRange(_buffer_id, 0, _size_bytes, access);
}

void* GLBuffer::mapRange(std::int64_t offset, std::int64_t length, GLenum access)
{
	return glMapNamedBufferRange(_buffer_id, offset, length, access);
}

void GLBuffer::unmap()
{
    glUnmapNamedBuffer(_buffer_id);
}

static const int _segment_count = 4;
int GLDynamicBuffer::_update_segment_index = 1;
int GLDynamicBuffer::_render_segment_index = 0;

static const int _persistent_map_flags = GL_MAP_WRITE_BIT | GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT;

GLDynamicBuffer::GLDynamicBuffer(const GLDynamicBufferDesc& desc)
 : GLBuffer(GLBufferDesc(desc.segment_size_bytes*_segment_count, nullptr, _persistent_map_flags | GL_DYNAMIC_STORAGE_BIT))
{
   _head_ptr = (char*)glMapNamedBufferRange(_buffer_id, 0, desc.segment_size_bytes*_segment_count, _persistent_map_flags);
   _segment_size = desc.segment_size_bytes;
}

GLDynamicBuffer::~GLDynamicBuffer()
{

}

void* GLDynamicBuffer::getUpdateSegmentPtr()
{
   return _head_ptr + (_update_segment_index*segmentSize());
}

const void* GLDynamicBuffer::getRenderSegmentPtr() const
{
   return _head_ptr + (_render_segment_index*segmentSize());
}

std::int64_t GLDynamicBuffer::getUpdateSegmentOffset()
{
   return _update_segment_index*segmentSize();
}

std::int64_t GLDynamicBuffer::getRenderSegmentOffset() const
{
   return _render_segment_index*segmentSize();
}

void GLDynamicBuffer::moveActiveSegments()
{ 
   _render_segment_index = (_render_segment_index + 1) % _segment_count;
   _update_segment_index = (_render_segment_index + 1) % _segment_count;
   
}


Uptr<GLBuffer> createBuffer(std::int64_t size_bytes, GLenum flags, void* data)
{
    GLBufferDesc desc;
    desc.flags = flags;
    desc.size_bytes = size_bytes;
    desc.data = data;
    return std::make_unique<GLBuffer>(desc);
}

Uptr<GLDynamicBuffer> createDynamicBuffer(std::int64_t requested_size_bytes)
{
   int uniform_buffer_align_size;
   glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uniform_buffer_align_size);

   GLDynamicBufferDesc desc;
   desc.segment_size_bytes = GLFormats::alignSize(requested_size_bytes, uniform_buffer_align_size);
   return std::make_unique<GLDynamicBuffer>(desc);
}

}