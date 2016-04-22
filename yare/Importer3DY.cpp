#include "Importer3DY.h"

#include <fstream>
#include <json/json.h>
#include <iostream>

#include "RenderMesh.h"
#include "Scene.h"

using namespace std;
using namespace glm;

void readDataBlock(const Json::Value& json_object, uint64_t* address, uint64_t* size)
{
	const auto& datablock = json_object["DataBlock"];
	*address = datablock["Address"].asUInt64();
	*size = datablock["Size"].asUInt64();
}

Uptr<RenderMesh> readMesh(const Json::Value& mesh_object, std::ifstream& data_file)
{
	int vertex_count = mesh_object["VertexCount"].asInt();
	int triangle_count = mesh_object["TriangleCount"].asInt();
	const auto& fields = mesh_object["Fields"];

	std::vector<VertexField> mesh_fields;
	for (const auto& field : fields)
	{
		VertexField mesh_field;
		mesh_field.components = field["Components"].asInt();
		mesh_field.component_type = GL_FLOAT;
		mesh_field.name = field["Name"].asString();
		mesh_fields.push_back(mesh_field);
	}

	auto render_mesh = std::make_unique<RenderMesh>(vertex_count/3, vertex_count, mesh_fields);
	for (const auto& field : fields)
	{
		uint64_t address, size;
		readDataBlock(field, &address, &size);
		data_file.seekg(address, std::ios::beg);
		auto name = field["Name"].asString();
		char* vertex_field = (char*)render_mesh->mapVertices(name);
		auto var = size / 3 / 4 ;
		assert(var == vertex_count);
		data_file.read(vertex_field, size);
		render_mesh->unmapVertices();
	}

	return render_mesh;
}

mat4x3 readMatrix4x3(const Json::Value& json_matrix)
{
	mat4x3 matrix;
	for (int i = 0; i < 4; ++i)
	{
		auto val = vec3(json_matrix[3 * i + 0].asFloat(), json_matrix[3 * i + 1].asFloat(), json_matrix[3 * i + 2].asFloat());
		matrix[i] = val;
	}

	return matrix;
}

void import3DY(const std::string& filename, Scene* scene)
{
	std::ifstream data_file("D:\\test.bin", std::ifstream::binary);

	Json::Value root;
	std::ifstream json_file("D:\\test.json");
	json_file >> root;
	json_file.close();

	const auto& json_surfaces = root["Surfaces"];
	for (const auto& json_surface : json_surfaces)
	{			
		auto render_mesh = readMesh(json_surface["Mesh"], data_file);
		const auto& json_matrix = json_surface["WorldToLocalMatrix"];
		
		SurfaceInstance surface_instance;
		surface_instance.mesh = createVertexSource(*render_mesh);
		surface_instance.matrix_world_local = readMatrix4x3(json_matrix);
		scene->surfaces.push_back(surface_instance);
	}
}