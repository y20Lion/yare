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
#include "Skeleton.h"
#include "AnimationPlayer.h"

namespace yare {

using namespace std;
using namespace glm;

static void readDataBlock(const Json::Value& json_object, uint64_t* address, uint64_t* size)
{
	const auto& datablock = json_object["DataBlock"];
	*address = datablock["Address"].asUInt64();
	*size = datablock["Size"].asUInt64();
}

static MeshFieldName _fieldName(const std::string& field_name)
{
    if (field_name == "position")
        return MeshFieldName::Position;
    else if (field_name == "normal")
        return MeshFieldName::Normal;
    else if (field_name == "uv")
        return MeshFieldName::Uv0;
    else if (field_name == "tangent")
       return MeshFieldName::Tangent0;
    else if (field_name == "bone_indices")
       return MeshFieldName::BoneIndices;
    else if (field_name == "bone_weights")
       return MeshFieldName::BoneWeights;
    else
    {
        assert(false);
        return MeshFieldName::Position;
    }
}

GLenum _componentType(const std::string& component_type)
{
   if (component_type == "Float")
      return GL_FLOAT;
   else if (component_type == "UnsignedByte")
      return GL_UNSIGNED_BYTE;
   else if (component_type == "UnsignedShort")
      return GL_UNSIGNED_SHORT;
   else
      assert(false);
   return 0;
}

static Uptr<RenderMesh> readMesh(const Json::Value& mesh_object, std::ifstream& data_file)
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
		mesh_field.component_type = _componentType(field["Type"].asString());
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

static mat4x3 readMatrix4x3(const Json::Value& json_matrix)
{
	mat4x3 matrix;
	for (int i = 0; i < 4; ++i)
	{
		auto val = vec3(json_matrix[3 * i + 0].asFloat(), json_matrix[3 * i + 1].asFloat(), json_matrix[3 * i + 2].asFloat());
		matrix[i] = val;
	}

	return matrix;
}

static vec3 readVec3(const Json::Value& json_vec)
{
   return vec3(json_vec[0].asFloat(), json_vec[1].asFloat(), json_vec[2].asFloat());
}

static quat readQuaternion(const Json::Value& json_vec)
{
   return quat(json_vec[0].asFloat(), json_vec[1].asFloat(), json_vec[2].asFloat(), json_vec[3].asFloat());
}

static void readNodeSlots(const Json::Value& json_slots, std::map<std::string, ShadeTreeNodeSlot>& slots)
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
                 slot.type = ShadeTreeNodeSlotType::Normal; // TODO remove slot type
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
typedef std::map<std::string, Sptr<Skeleton>> SkeletonMap;

static void readTexImageNodeProperties(const Json::Value& json_node, const TextureMap& textures, TexImageNode& node)
{
    node.texture = textures.at(json_node["Image"].asString());
    node.texture_transform = readMatrix4x3(json_node["TransformMatrix"]);
}

static void readMathNodeProperties(const Json::Value& json_node, MathNode& node)
{
   node.clamp = json_node["Clamp"].asBool();
   node.operation = json_node["Operation"].asString();
}

static void readVectMathNodeProperties(const Json::Value& json_node, VectorMathNode& node)
{
   node.operation = json_node["Operation"].asString();
}

static Uptr<ShadeTreeMaterial> readMaterial(const RenderEngine& render_engine, const Json::Value& json_material, const TextureMap& textures)
{   
   Uptr<ShadeTreeMaterial> material = std::make_unique<ShadeTreeMaterial>(*render_engine.render_resources);
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

    //material->compile(*render_engine.render_resources);
    
    return material;
}


static MaterialMap readMaterials(const RenderEngine& render_engine, const Json::Value& json_materials, const TextureMap& textures)
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

static std::map<std::string, Sptr<GLTexture>> readTextures(const Json::Value& json_textures)
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

static mat4x3 sunMatFromDirection(const vec3& dir)
{
   // degenerate matrix but only z dir will be used
   return mat4x3(vec3(0), vec3(0), dir, vec3(0));
}

void readEnvironment(const RenderEngine& render_engine, const Json::Value& json_env, Scene* scene)
{
   const auto& texture_name = json_env["Name"].asString();
   const auto& texture_path = json_env["Path"].asString();

   Uptr<GLTexture2D> latlong_texture = TextureImporter::importTextureFromFile(texture_path.c_str(), true);
   scene->sky_cubemap = render_engine.cubemap_converter->createCubemapFromLatlong(*latlong_texture);
   scene->sky_diffuse_cubemap = render_engine.cubemap_converter->createDiffuseCubemap(*scene->sky_cubemap, DiffuseFilteringMethod::BruteForce);
   scene->sky_diffuse_cubemap_sh = render_engine.cubemap_converter->createDiffuseCubemap(*scene->sky_cubemap, DiffuseFilteringMethod::SphericalHarmonics);
   auto lights_from_env = render_engine.cubemap_converter->extractDirectionalLightSourcesFromLatlong(*latlong_texture);
   scene->sky_latlong = std::move(latlong_texture);
   for (const auto& env_light : lights_from_env)
   {
      Light light; 
      light.color = env_light.color;
      light.type = LightType::Sun;
      light.strength = 1.0f;
      light.world_to_local_matrix = sunMatFromDirection(env_light.direction);
      light.sun.size = 1.0;
      scene->lights.push_back(light);
   }
}

static LightType convertToLightType(const std::string& name)
{
   if (name == "AREA")
      return LightType::Rectangle;
   else if (name == "POINT")
      return LightType::Sphere;
   else if (name == "SPOT")
      return LightType::Spot;
   else
      return LightType::Sun;
}

static void readLights(const Json::Value& json_lights, Scene* scene)
{
   for (const auto& json_light : json_lights)
   {
      Light light;
      const auto& json_color = json_light["Color"];
      light.color = glm::vec3(json_color[0].asFloat(), json_color[1].asFloat(), json_color[2].asFloat());
      light.type = convertToLightType(json_light["Type"].asString());
      light.strength = json_light["Strength"].asFloat();
      light.world_to_local_matrix = readMatrix4x3(json_light["WorldToLocalMatrix"]);

      switch (light.type)
      {
         case LightType::Sphere:
            light.sphere.size = json_light["Size"].asFloat();
            break;
         case LightType::Rectangle:
            light.rectangle.size_x = json_light["SizeX"].asFloat();
            light.rectangle.size_y = json_light["SizeY"].asFloat();
            break;
         case LightType::Spot:
            light.spot.angle = json_light["Angle"].asFloat();
            light.spot.angle_blend = json_light["AngleBlend"].asFloat();
            break;
         case LightType::Sun:
            light.sun.size = json_light["Size"].asFloat();
            break;
      }
      scene->lights.push_back(light);
   }
}

static SkeletonMap readSkeletons(const Json::Value& json_skeletons, Scene* scene)
{
   SkeletonMap skeletons;
   for (const auto& json_skeleton : json_skeletons)
   {
      const auto& json_bones = json_skeleton["Bones"];      
      auto skeleton = std::make_shared<Skeleton>(json_bones.size());
      skeleton->name = json_skeleton["Name"].asString();
      skeleton->world_to_skeleton_matrix = readMatrix4x3(json_skeleton["WorldToSkeletonMatrix"]);
      
      int i = 0;
      for (const auto& json_bone : json_bones)
      {
         auto& bone = skeleton->bones[i];
         bone.name = json_bone["Name"].asString();
         bone.parent_to_bone_transform.location = readVec3(json_bone["Pose"]["Location"]);
         bone.parent_to_bone_transform.scale = readVec3(json_bone["Pose"]["Scale"]);
         bone.parent_to_bone_transform.quaternion = readQuaternion(json_bone["Pose"]["Quaternion"]); // TODO fix quaternion order
         bone.parent_to_bone_transform.pose_matrix = readMatrix4x3(json_bone["Pose"]["PoseMatrix"]);         
         // TODO bone parent
         skeleton->skeleton_to_bone_bind_pose_matrices[i] = readMatrix4x3(json_bone["SkeletonToBoneMatrix"]);//TODO remove this        
         skeleton->bone_name_to_index[bone.name] = i;
         ++i;
      }

      i = 0;
      for (const auto& json_bone : json_bones)
      {
         auto& bone = skeleton->bones[i];
         if (!json_bone["Parent"].isString())
            skeleton->root_bone_index = i;

         bone.parent = json_bone["Parent"].isString() ? skeleton->bone_name_to_index.at(json_bone["Parent"].asString()) : -1;
         for (const auto& json_child : json_bone["Children"])
         {
            bone.chilren.push_back(skeleton->bone_name_to_index.at(json_child.asString()));
         }
         ++i;
      }

      skeletons[skeleton->name] = skeleton;
      scene->skeletons.push_back(skeleton);
   }
   return skeletons;
}

static void readAnimations(const Json::Value& json_animations, Scene* scene)
{
   for (const auto& json_animation : json_animations)
   {
      for (const auto& json_curve : json_animation["Curves"])
      {
         scene->animation_player->curves.push_back(AnimationCurve());
         AnimationCurve& curve = scene->animation_player->curves.back();

         curve.target_path = json_curve["TargetPropertyPath"].asString();
         for (const auto& json_keyframe : json_curve["Keyframes"])
         {
            Keyframe keyframe;
            keyframe.x = json_keyframe["X"].asFloat();
            keyframe.y = json_keyframe["Y"].asFloat();
            curve.keyframes.push_back(keyframe);
         }
      }
   }
}

void import3DY(const std::string& filename, const RenderEngine& render_engine, Scene* scene)
{
	std::ifstream data_file(filename+"\\data.bin", std::ifstream::binary);

	Json::Value root;
	std::ifstream json_file(filename+"\\structure.json");
	json_file >> root;
	json_file.close();

   readLights(root["Lights"], scene);
   readEnvironment(render_engine, root["Environment"], scene);
   readAnimations(root["Animations"], scene);
   auto skeletons = readSkeletons(root["Skeletons"], scene);
   auto textures = readTextures(root["Textures"]);
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

      auto mat_it = materials.find(json_surface["Material"].asString());
      if (mat_it != materials.end())
         surface_instance.material = mat_it->second;
      else
         surface_instance.material = default_material;

      auto sk_it = skeletons.find(json_surface["Skeleton"].asString());
      if (sk_it != skeletons.end())
      {
         surface_instance.skeleton = sk_it->second;
         surface_instance.material_variant = MaterialVariant::WithSkinning;
      }
      else
      {
         surface_instance.skeleton = nullptr;
         surface_instance.material_variant = MaterialVariant::Normal;
      }
      surface_instance.material_program = &surface_instance.material->compile(surface_instance.material_variant);
		scene->surfaces.push_back(surface_instance);
	}
}

}