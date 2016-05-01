#include "GLVertexSource.h"

#include "GLBuffer.h"

namespace yare {

GLVertexSource::GLVertexSource()
    : _primitive_type(GL_TRIANGLES)
    , _vertex_count(0)
{
    glGenVertexArrays(1, &_vao_id);
}

GLVertexSource::~GLVertexSource()
{
    glDeleteVertexArrays(1, &_vao_id);
}

void GLVertexSource::setVertexAttribute(int attribute_slot, int components, GLenum component_type, int attribute_stride, std::int64_t attribute_offset)
{
    glBindVertexArray(_vao_id);
    glEnableVertexAttribArray(attribute_slot);
    glVertexAttribPointer(attribute_slot, components, component_type, GL_FALSE, attribute_stride, (const void*)attribute_offset);
    glBindVertexArray(0);
}

void GLVertexSource::setIndexBuffer(const GLBuffer& index_buffer)
{
    glBindVertexArray(_vao_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer.id());
    glBindVertexArray(0);
}

void GLVertexSource::setVertexBuffer(const GLBuffer& vertex_buffer)
{
    glBindVertexArray(_vao_id);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer.id());
    glBindVertexArray(0);
}

}
