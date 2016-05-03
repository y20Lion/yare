#include "Importer3DY.h"

#include <fstream>
#include <json/json.h>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

#include "GLTexture.h"
#include "RenderMesh.h"
#include "Scene.h"
#include "ShadeTreeNode.h"
#include "ShadeTreeMaterial.h"
#include "TextureImporter.h"

namespace yare {

using namespace std;
using namespace glm;

void readDataBlock(const Json::Value& json_object, uint64_t* address, uint64_t* size)
{
	const auto& datablock = json_object["DataBlock"];
	*address = datablock["Address"].asUInt64();
	*size = datablock["Size"].asUInt64();
}

FieldName _fieldName(const std::string& field_name)
{
    if (field_name == "position")
        return FieldName::Position;
    else if (field_name == "normal")
        return FieldName::Normal;
    else if (field_name == "uv")
        return FieldName::Uv0;
    else
    {
        assert(false);
        return FieldName::Position;
    }
}

Uptr<RenderMesh> readMesh(const Json::Value& mesh_object, std::ifstream& data_file)
{
	int vertex_count = mesh_object["VertexCount"].asInt();
	int triangle_count = mesh_object["TriangleCount"].asInt();
	const auto& fields = mesh_object["Fields"];
	if (vertex_count == 0)
		return nullptr;

	std::vector<VertexField> mesh_fields;
	for (const auto& field : fields)
	{
		VertexField mesh_field;
		mesh_field.components = field["Components"].asInt();
		mesh_field.component_type = GL_FLOAT;
		mesh_field.name = _fieldName(field["Name"].asString());
		mesh_fields.push_back(mesh_field);
	}

	auto render_mesh = std::make_unique<RenderMesh>(vertex_count/3, vertex_count, mesh_fields);
	for (const auto& field : fields)
	{
		uint64_t address, size;
		readDataBlock(field, &address, &size);
		data_file.seekg(address, std::ios::beg);
		auto name = _fieldName(field["Name"].asString());
		char* vertex_field = (char*)render_mesh->mapVertices(name);
		//auto var = size / 3 / 4 ;
		//assert(var == vertex_count);
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

void readNodeSlots(const Json::Value& json_slots, std::map<std::string, ShadeTreeNodeSlot>& slots)
{
    for (const auto& json_slot : json_slots)
    {
        ShadeTreeNodeSlot slot;
        slot.name = json_slot["Name"].asString();
        slot.default_value = vec4(0.0);
        slot.type = ShadeTreeNodeSlotType::Float;
        if (json_slot.isMember("DefaultValue"))
        {
            int value_components = json_slot["DefaultValue"].size();
            for (int i = 0; i < value_components; ++i)
                slot.default_value[i] = json_slot["DefaultValue"][i].asFloat();

            if (value_components == 1)
                slot.type = ShadeTreeNodeSlotType::Float;
            else if (slot.name == "Normal")
                slot.type = ShadeTreeNodeSlotType::Normal;
            else
                slot.type = ShadeTreeNodeSlotType::Vec3;
        }        

        for (const auto& json_link : json_slot["Links"])
        {
            Link link;
            link.node_name = json_link["Node"].asString();
            link.slot_name = json_link["Slot"].asString();
            slot.links.push_back(link);
        }
        
        slots[slot.name] = slot;
    }    
}


typedef std::map<std::string, Sptr<GLTexture>> TextureMap;
typedef std::map<std::string, Sptr<ShadeTreeMaterial>> MaterialMap;

void readTexImageNodeProperties(const Json::Value& json_node, const TextureMap& textures, TexImageNode* node)
{
    node->texture = textures.at(json_node["Image"].asString());
    node->texture_transform = readMatrix4x3(json_node["TransformMatrix"]);
}

Uptr<ShadeTreeMaterial> readMaterial(const Json::Value& json_material, const TextureMap& textures)
{
    Uptr<ShadeTreeMaterial> material = std::make_unique<ShadeTreeMaterial>();
    material->name = json_material["Name"].asString();
    for (const auto& json_node : json_material["Nodes"])
    {
        auto node = createShadeTreeNode(json_node["Type"].asString());
        if (node == nullptr)
            return nullptr;
        node->name = json_node["Name"].asString();
        readNodeSlots(json_node["InputSlots"], node->input_slots);
        readNodeSlots(json_node["OutputSlots"], node->output_slots); 
        if (node->type == "TEX_IMAGE")
            readTexImageNodeProperties(json_node, textures, (TexImageNode*)node.get());

        material->tree_nodes[json_node["Name"].asString()] = std::move(node);
    }
    return material;
}


MaterialMap readMaterials(const Json::Value& json_materials, const TextureMap& textures)
{
    MaterialMap materials;
    for (const auto& json_material : json_materials)
    {
        auto material = readMaterial(json_material, textures);
        if (material)
        {
            material->compile();
            materials[json_material["Name"].asString()] = std::move(material);
        }
        else
        {
            materials[json_material["Name"].asString()] = (*materials.begin()).second;
        }

    }
    return materials;
}

std::map<std::string, Sptr<GLTexture>> readTextures(const Json::Value& json_textures)
{
    std::map<std::string, Sptr<GLTexture>> textures;
    for (const auto& json_texture : json_textures)
    {
        const auto& texture_name = json_texture["Name"].asString();
        const auto& texture_path = json_texture["Path"].asString();
        textures[texture_name] = TextureImporter::importTextureFromFile(texture_path.c_str());
    }
    return textures;
}

void import3DY(const std::string& filename, Scene* scene)
{
	std::ifstream data_file(filename+"\\data.bin", std::ifstream::binary);

	Json::Value root;
	std::ifstream json_file(filename+"\\structure.json");
	json_file >> root;
	json_file.close();

    auto textures = readTextures(root["Textures"]);
    auto materials = readMaterials(root["Materials"], textures);    

	const auto& json_surfaces = root["Surfaces"];
	for (const auto& json_surface : json_surfaces)
	{			
		auto render_mesh = readMesh(json_surface["Mesh"], data_file);
		if (!render_mesh)
			continue;
		const auto& json_matrix = json_surface["WorldToLocalMatrix"];
		
		SurfaceInstance surface_instance;
		surface_instance.mesh = std::move(render_mesh);
		surface_instance.matrix_world_local = readMatrix4x3(json_matrix);
        surface_instance.material = materials.at(json_surface["Material"].asString());
		scene->surfaces.push_back(surface_instance);
	}   
}

}