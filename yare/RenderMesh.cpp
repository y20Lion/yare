#include "RenderMesh.h"

#include <assert.h>

#include "GLVertexSource.h"

static int _sizeOfType(GLuint type)
{
	switch (type)
	{
		case GL_BYTE: 
		case GL_UNSIGNED_BYTE: 
			return 1;
		case GL_SHORT:
		case GL_UNSIGNED_SHORT:
		case GL_HALF_FLOAT:
			return 2;
		case GL_INT:
		case GL_UNSIGNED_INT:
		case GL_FLOAT:
			return 4;
		case GL_DOUBLE: 
			return 8;
		default: 
			assert(false);
			return 0;
	}	
}


RenderMesh::RenderMesh(int triangle_count, int vertex_count, const std::vector<VertexField>& input_fields)
: _triangle_count(triangle_count)
, _vertex_count(vertex_count)
{
	std::int64_t vertex_buffer_size = 0;
	for (const auto& input_field : input_fields)
	{
		auto& field = _fields[input_field.name];
		field.components = input_field.components;
		field.component_type = input_field.component_type;
		field.offset = vertex_buffer_size;
		field.size = input_field.components*_sizeOfType(input_field.component_type)*vertex_count;
		vertex_buffer_size += field.size;
	}

	GLBufferDesc desc;
	desc.flags = GL_MAP_WRITE_BIT;
	desc.size_bytes = vertex_buffer_size;
	_vertex_buffer = std::make_unique<GLBuffer>(desc);
}

RenderMesh::~RenderMesh()
{

}

void* RenderMesh::mapVertices(const std::string& vertex_field)
{
	const auto& field = _fields.at(vertex_field);
	return _vertex_buffer->mapRange(field.offset, field.size, GL_MAP_WRITE_BIT);
}

void RenderMesh::unmapVertices()
{
	_vertex_buffer->unmap();
}

void* RenderMesh::mapTrianglesIndices()
{
	return nullptr;
}

void RenderMesh::unmapTriangleIndices()
{

}

Uptr<GLVertexSource> createVertexSource(const RenderMesh& mesh)
{
	auto vertex_source = std::make_unique<GLVertexSource>();
	vertex_source->setVertexBuffer(mesh.vertexBuffer());
	vertex_source->setVertexAttribute(1, 3, GL_FLOAT, 0, 0);
	vertex_source->setVertexCount(mesh.vertexCount());

	return vertex_source;
}