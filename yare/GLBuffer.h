#pragma once

#include <GL/glew.h>
#include <cstdint>

#include "tools.h"

namespace yare {

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
	void* mapRange(std::int64_t offset, std::int64_t length, GLenum access);
    void unmap();

	std::int64_t size() const { return _size_bytes; }

private:
    DISALLOW_COPY_AND_ASSIGN(GLBuffer)
    GLuint _buffer_id;
	std::int64_t _size_bytes;
};

Uptr<GLBuffer> createBuffer(std::int64_t size_bytes);

}