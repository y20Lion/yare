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

struct GLPersistentlyMappedBufferDesc
{
   std::int64_t window_size_bytes;
};

// I know inheritance sets always the course towards disaster, but for now it's just a bit awkwardly
class GLPersistentlyMappedBuffer : public GLBuffer
{
public:
   GLPersistentlyMappedBuffer(const GLPersistentlyMappedBufferDesc& desc);
   virtual ~GLPersistentlyMappedBuffer();

   void* getCurrentWindowPtr();
   std::int64_t getCurrentWindowOffset();

   std::int64_t windowSize() const { return _window_size; }
   static void moveWindow() { _window_index = (_window_index + 1) % _window_count; }

private:
   char* _head_ptr;
   std::int64_t _window_size;
   static int _window_index;
   static int _window_count;
   DISALLOW_COPY_AND_ASSIGN(GLPersistentlyMappedBuffer)
};

Uptr<GLBuffer> createBuffer(std::int64_t size_bytes, void* data=nullptr);
Uptr<GLPersistentlyMappedBuffer> createPersistentBuffer(std::int64_t size_bytes);

}