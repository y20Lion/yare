#pragma once

#include <GL/glew.h>
#include <cstdint>

#include "tools.h"

namespace yare {

class GLBuffer;

class GLVertexSource
{
public:
    GLVertexSource();
    ~GLVertexSource();
    GLuint id() const { return _vao_id; }

    void setVertexAttribute(int attribute_slot, int components, GLenum component_type, int attribute_stride = 0, std::int64_t attribute_offset = 0);
    void setIndexBuffer(const GLBuffer& index_buffer);
    void setVertexBuffer(const GLBuffer& vertex_buffer);

    void setPrimitiveType(GLenum primitive_type) {_primitive_type = primitive_type; }
    GLenum primitiveType() const { return _primitive_type; }

    void setVertexCount(int vertex_count) {_vertex_count = vertex_count; }
    int vertexCount() const { return _vertex_count; }

private:
    DISALLOW_COPY_AND_ASSIGN(GLVertexSource)
    GLuint _vao_id;
    GLenum _primitive_type;
    int _vertex_count;
};

}