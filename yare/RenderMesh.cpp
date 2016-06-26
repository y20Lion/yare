#include "RenderMesh.h"

#include <assert.h>

#include "GLVertexSource.h"
#include "GLFormats.h"

namespace yare {




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
		field.size = input_field.components*GLFormats::sizeOfType(input_field.component_type)*vertex_count;
		vertex_buffer_size += field.size;
	}

	GLBufferDesc desc;
	desc.flags = GL_MAP_WRITE_BIT;
	desc.size_bytes = vertex_buffer_size;
   desc.data = nullptr;
	_vertex_buffer = std::make_unique<GLBuffer>(desc);
}

RenderMesh::~RenderMesh()
{

}

void* RenderMesh::mapVertices(MeshFieldName vertex_field)
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

Uptr<GLVertexSource> createVertexSource(const RenderMesh& mesh, FieldsMask fields_bitmask)
{
    auto vertex_source = std::make_unique<GLVertexSource>();   
    vertex_source->setVertexBuffer(mesh.vertexBuffer());
    for (int i = 0; i < 32; ++i)
    {
        auto mesh_field = (1 << i);
        if (fields_bitmask & mesh_field)
        {
            auto field_info = mesh.fieldInfo(MeshFieldName(mesh_field));
            GLSLVecType vec_type = MeshFieldName(mesh_field) == MeshFieldName::BoneIndices ? GLSLVecType::uvec: GLSLVecType::vec;
            vertex_source->setVertexAttribute(i, field_info.components, field_info.component_type, vec_type, 0, field_info.offset);
        }
    }
    vertex_source->setVertexCount(mesh.vertexCount());
    return vertex_source;
}

Uptr<GLVertexSource> createVertexSource(const RenderMesh& mesh)
{
	auto vertex_source = std::make_unique<GLVertexSource>();
	vertex_source->setVertexBuffer(mesh.vertexBuffer());
	vertex_source->setVertexAttribute(0, 3, GL_FLOAT, GLSLVecType::vec, 0, mesh.fieldInfo(MeshFieldName::Position).offset);
   vertex_source->setVertexAttribute(1, 3, GL_FLOAT, GLSLVecType::vec, 0, mesh.fieldInfo(MeshFieldName::Normal).offset);
   vertex_source->setVertexAttribute(2, 2, GL_FLOAT, GLSLVecType::vec, 0, mesh.fieldInfo(MeshFieldName::Uv0).offset);
	vertex_source->setVertexCount(mesh.vertexCount());

	return vertex_source;
}

}