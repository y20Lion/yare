#pragma once

#include <GL/glew.h>
#include <cstdint>

#include "tools.h"

namespace yare {

struct GLBufferDesc
{
   GLBufferDesc(){}
   GLBufferDesc(std::int64_t size_bytes, void* data, GLbitfield flags)
      : size_bytes(size_bytes), data(data), flags(flags) {}
   
   std::int64_t size_bytes;
   void* data;
   GLbitfield flags;
};

class GLBuffer
{
public:
    GLBuffer(const GLBufferDesc& desc);
    virtual ~GLBuffer();
    GLuint id() const { return _buffer_id; }

    void* map(GLenum access);
	void* mapRange(std::int64_t offset, std::int64_t length, GLenum access);
    void unmap();

	std::int64_t size() const { return _size_bytes; }

protected:
    DISALLOW_COPY_AND_ASSIGN(GLBuffer)
    GLuint _buffer_id;
	std::int64_t _size_bytes;
};

struct GLDynamicBufferDesc
{
   std::int64_t segment_size_bytes;
};

// Deriving directly from GLBuffer is awkward, there should be a GLBufferBase
class GLDynamicBuffer : public GLBuffer
{
public:
   GLDynamicBuffer(const GLDynamicBufferDesc& desc);
   virtual ~GLDynamicBuffer();

   void* getUpdateSegmentPtr(); 
   const void* getRenderSegmentPtr() const;
   std::int64_t getUpdateSegmentOffset();
   std::int64_t getRenderSegmentOffset() const;

   std::int64_t segmentSize() const { return _segment_size; }
   static void moveActiveSegments();

private:
   char* _head_ptr;
   std::int64_t _segment_size;
   static int _update_segment_index;
   static int _render_segment_index;
   DISALLOW_COPY_AND_ASSIGN(GLDynamicBuffer)
};

Uptr<GLBuffer> createBuffer(std::int64_t size_bytes, GLenum flags = 0, void* data=nullptr);
Uptr<GLDynamicBuffer> createDynamicBuffer(std::int64_t requested_size_bytes);

}