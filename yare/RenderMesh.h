#pragma once

#include <map>
#include <string>
#include <vector>

#include "GLBuffer.h"

class GLProgram;
class GLVertexSource;

struct VertexField
{
    std::string name;
    int components;
    GLenum component_type;
};

class RenderMesh
{
public:
    RenderMesh(int triangle_count, int vertex_count, const std::vector<VertexField>& fields);
    ~RenderMesh();

    void* mapVertices(const std::string& vertex_field);
    void unmapVertices();

    void* mapTrianglesIndices();
    void unmapTriangleIndices();

	const GLBuffer& vertexBuffer() const { return *_vertex_buffer;  }
	int triangleCount() const { return _triangle_count; }
	int vertexCount() const { return _vertex_count; }

private:
    DISALLOW_COPY_AND_ASSIGN(RenderMesh)
	struct Field
	{
		int components;
		GLenum component_type;
		std::int64_t offset;
		std::int64_t size;
	};

	std::map<std::string, Field> _fields;
	int _triangle_count;
	int _vertex_count;
    //GLBuffer _index_buffer;
    Uptr<GLBuffer> _vertex_buffer;
};

//Uptr<GLVertexSource> createVertexSource(const RenderMesh& mesh, const std::vector<std::pair<std::string, int>> field_to_attribute_binding);
Uptr<GLVertexSource> createVertexSource(const RenderMesh& mesh);