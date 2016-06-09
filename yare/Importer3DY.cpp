#include "Importer3DY.h"

#include <memory>
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
#include "RenderEngine.h"
#include "CubemapFiltering.h"
#include "DefaultMaterial.h"
#include "OceanMaterial.h"

namespace yare {

using namespace std;
using namespace glm;

void readDataBlock(const Json::Value& json_object, uint64_t* address, uint64_t* size)
{
	const auto& datablock = json_object["DataBlock"];
	*address = datablock["Address"].asUInt64();
	*size = datablock["Size"].asUInt64();
}

MeshFieldName _fieldName(const std::string& field_name)
{
    if (field_name == "position")
        return MeshFieldName::Position;
    else if (field_name == "normal")
        return MeshFieldName::Normal;
    else if (field_name == "uv")
        return MeshFieldName::Uv0;
    else if (field_name == "tangent")
       return MeshFieldName::Tangent0;
    else
    {
        assert(false);
        return MeshFieldName::Position;
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

           if (json_slot["DefaultValue"].isArray())
           {
              int value_components = json_slot["DefaultValue"].size();
              for (int i = 0; i < value_components; ++i)
                 slot.default_value[i] = json_slot["DefaultValue"][i].asFloat();

              if (slot.name == "Normal")
                 slot.type = ShadeTreeNodeSlotType::Normal;
              else
                 slot.type = ShadeTreeNodeSlotType::Vec3;
           }
           else
           {
              slot.default_value.x = json_slot["DefaultValue"].asFloat();
              slot.type = ShadeTreeNodeSlotType::Float;
           }
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
typedef std::map<std::string, Sptr<IMaterial>> MaterialMap;

void readTexImageNodeProperties(const Json::Value& json_node, const TextureMap& textures, TexImageNode& node)
{
    node.texture = textures.at(json_node["Image"].asString());
    node.texture_transform = readMatrix4x3(json_node["TransformMatrix"]);
}

void readMathNodeProperties(const Json::Value& json_node, MathNode& node)
{
   node.clamp = json_node["Clamp"].asBool();
   node.operation = json_node["Operation"].asString();
}

void readVectMathNodeProperties(const Json::Value& json_node, VectorMathNode& node)
{
   node.operation = json_node["Operation"].asString();
}

Uptr<ShadeTreeMaterial> readMaterial(const RenderEngine& render_engine, const Json::Value& json_material, const TextureMap& textures)
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
           readTexImageNodeProperties(json_node, textures, (TexImageNode&)*node);
        else if (node->type == "MATH")
           readMathNodeProperties(json_node, (MathNode&)*node);
        else if (node->type == "VECT_MATH")
           readVectMathNodeProperties(json_node, (VectorMathNode&)*node);

        material->tree_nodes[json_node["Name"].asString()] = std::move(node);
    }

    material->compile(*render_engine.render_resources);
    
    return material;
}


MaterialMap readMaterials(const RenderEngine& render_engine, const Json::Value& json_materials, const TextureMap& textures)
{
   MaterialMap materials;
   for (const auto& json_material : json_materials)
   {
      Uptr<IMaterial> material;
      if (json_material["Name"].asString() == "Ocean")
         material = std::make_unique<OceanMaterial>();
      else       
         material = readMaterial(render_engine, json_material, textures);

      if (material)
      {
         materials[json_material["Name"].asString()] = std::move(material);
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

void readEnvironment(const RenderEngine& render_engine, const Json::Value& json_env, Scene* scene)
{
   const auto& texture_name = json_env["Name"].asString();
   const auto& texture_path = json_env["Path"].asString();
   scene->sky_cubemap = TextureImporter::importCubemapFromFile(texture_path.c_str(), *render_engine.cubemap_converter);
   scene->sky_diffuse_cubemap = render_engine.cubemap_converter->createDiffuseCubemap(*scene->sky_cubemap, DiffuseFilteringMethod::BruteForce);
   scene->sky_diffuse_cubemap_sh = render_engine.cubemap_converter->createDiffuseCubemap(*scene->sky_cubemap, DiffuseFilteringMethod::SphericalHarmonics);
}

void import3DY(const std::string& filename, const RenderEngine& render_engine, Scene* scene)
{
	std::ifstream data_file(filename+"\\data.bin", std::ifstream::binary);

	Json::Value root;
	std::ifstream json_file(filename+"\\structure.json");
	json_file >> root;
	json_file.close();

    auto textures = readTextures(root["Textures"]);
    readEnvironment(render_engine, root["Environment"], scene);
    auto materials = readMaterials(render_engine, root["Materials"], textures);    

	const auto& json_surfaces = root["Surfaces"];
	
   auto default_material = std::make_shared<DefaultMaterial>();
   for (const auto& json_surface : json_surfaces)
	{			
		auto render_mesh = readMesh(json_surface["Mesh"], data_file);
		if (!render_mesh)
			continue;
		const auto& json_matrix = json_surface["WorldToLocalMatrix"];
		
		SurfaceInstance surface_instance;
		surface_instance.mesh = std::move(render_mesh);
		surface_instance.matrix_world_local = readMatrix4x3(json_matrix);

      auto it = materials.find(json_surface["Material"].asString());
      if (it != materials.end())
         surface_instance.material = it->second;
      else
         surface_instance.material = default_material;

		scene->surfaces.push_back(surface_instance);
	}   
}

}