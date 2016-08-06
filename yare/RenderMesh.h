#pragma once

#include <map>
#include <string>
#include <vector>

#include "GLBuffer.h"

namespace yare {

class GLProgram;
class GLVertexSource;

enum class MeshFieldName {
    Position = 1 << 0, Normal = 1 << 1,
    Uv0 = 1 << 2, Tangent0 = 1 << 3,
    BoneIndices = 1 << 4, BoneWeights = 1 << 5
};

typedef int FieldsMask;

struct VertexField
{
   MeshFieldName name;
   int components;
   GLenum component_type;
};

class RenderMesh // TODO Yvain handle indexed meshes
{
public:
    RenderMesh(int triangle_count, int vertex_count, const std::vector<VertexField>& fields);
    ~RenderMesh();

    void* mapVertices(MeshFieldName vertex_field);
    void unmapVertices();

    void* mapTrianglesIndices();
    void unmapTriangleIndices();

	const GLBuffer& vertexBuffer() const { return *_vertex_buffer;  }
	int triangleCount() const { return _triangle_count; }
	int vertexCount() const { return _vertex_count; }

    struct Field
    {
        int components;
        GLenum component_type;
        std::int64_t offset;
        std::int64_t size;
    };
    const Field& fieldInfo(MeshFieldName vertex_field) const { return _fields.at(vertex_field); }

private:
    DISALLOW_COPY_AND_ASSIGN(RenderMesh)	

	std::map<MeshFieldName, Field> _fields;
	int _triangle_count;
	int _vertex_count;
    //GLBuffer _index_buffer;
    Uptr<GLBuffer> _vertex_buffer;
};

Uptr<GLVertexSource> createVertexSource(const RenderMesh& mesh, FieldsMask fields_bitmask, bool tessellation);
Uptr<GLVertexSource> createVertexSource(const RenderMesh& mesh);

}