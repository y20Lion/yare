#pragma once

#include <string>
#include <vector>

#include "GLBuffer.h"

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

private:
    DISALLOW_COPY_AND_ASSIGN(RenderMesh)
    GLBuffer _triangle_indices;
    GLBuffer _vertex_fields;
};

Uptr<GLVertexSource> createVertexSource(const RenderMesh& mesh, const std::vector<std::pair<std::string, int>> field_to_attribute_binding);
Uptr<GLVertexSource> createVertexSource(const RenderMesh& mesh, const GLProgram& program);