#include "Importer3DY.h"

#include <memory>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <json/json.h>
#include <iostream>
#include <random>
#include <functional>
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
#include "TransformHierarchy.h"
#include "matrix_math.h"
#include "stl_helpers.h"

namespace yare {

using namespace std;
using namespace glm;

static void readDataBlock(const Json::Value& json_object, uint64_t* address, uint64_t* size)
{
	const auto& datablock = json_object["DataBlock"];
	*address = datablock["Address"].asUInt64();
	*size = datablock["Size"].asUInt64();
}

static std::unique_ptr<char[]> readDataBlock(const Json::Value& json_object, std::ifstream& data_file)
{
   uint64_t address, size;
   readDataBlock(json_object, &address, &size);
   auto buffer = std::make_unique<char[]>(size);
   data_file.seekg(address, std::ios::beg);
   data_file.read(buffer.get(), size);
   return buffer;
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

   render_mesh->commitToGPU();

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

static ivec3 readIVec3(const Json::Value& json_vec)
{
   return ivec3(json_vec[0].asInt(), json_vec[1].asInt(), json_vec[2].asInt());
}

static vec4 readVec4(const Json::Value& json_vec)
{
   return vec4(json_vec[0].asFloat(), json_vec[1].asFloat(), json_vec[2].asFloat(), json_vec[3].asFloat());
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
        std::replace(RANGE(slot.name), '.', '_');
        slot.default_value = vec4(0.0);

        if (json_slot.isMember("DefaultValue"))
        {

           if (json_slot["DefaultValue"].isArray())
           {
              int value_components = json_slot["DefaultValue"].size();
              for (int i = 0; i < value_components; ++i)
                 slot.default_value[i] = json_slot["DefaultValue"][i].asFloat();
           }
           else
           {
              slot.default_value.x = json_slot["DefaultValue"].asFloat();
           }
        }
     

        for (const auto& json_link : json_slot["Links"])
        {
            Link link;
            link.node_name = json_link["Node"].asString();
            link.slot_name = json_link["Slot"].asString();
            std::replace(RANGE(link.slot_name), '.', '_');
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
    auto mat4x3 = readMatrix4x3(json_node["TransformMatrix"]);
    node.texture_transform[0] = mat4x3[0];
    node.texture_transform[1] = mat4x3[1];
    node.texture_transform[2] = mat4x3[3];
}

static void readMathNodeProperties(const Json::Value& json_node, MathNode& node)
{
   node.clamp = json_node["Clamp"].asBool();
   node.operation = json_node["Operation"].asString();
}

static void readMixRgbNodeProperties(const Json::Value& json_node, MixRGBNode& node)
{
   node.clamp = json_node["Clamp"].asBool();
   node.operation = json_node["Operation"].asString();
}

static void readVectMathNodeProperties(const Json::Value& json_node, VectorMathNode& node)
{
   node.operation = json_node["Operation"].asString();
}

static void readColorRampNodeProperties(const Json::Value& json_node, ColorRampNode& node, std::ifstream& data_file)
{
   const auto& json_colors = json_node["Colors"];
   auto temp_buf_ptr = readDataBlock(json_colors, data_file);
   node.ramp_texture = createMipmappedTexture1D(256, GL_RGB16, temp_buf_ptr.get());
}

static auto readCurve(const Json::Value& json_curve_node, std::ifstream& data_file)
{
   auto temp_buf_ptr = readDataBlock(json_curve_node, data_file);
   return createTexture1D(256, GL_R16, temp_buf_ptr.get());
}

static void readCurveRgbNodeProperties(const Json::Value& json_node, CurveRgbNode& node, std::ifstream& data_file)
{
   node.red_curve = readCurve(json_node["RedCurve"], data_file);
   node.green_curve = readCurve(json_node["GreenCurve"], data_file);
   node.blue_curve = readCurve(json_node["BlueCurve"], data_file);
}

static Uptr<ShadeTreeMaterial> readMaterial(const RenderEngine& render_engine, const Json::Value& json_material, const TextureMap& textures, std::ifstream& data_file)
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
        else if (node->type == "MIX_RGB")
           readMixRgbNodeProperties(json_node, (MixRGBNode&)*node);
        else if (node->type == "VECT_MATH")
           readVectMathNodeProperties(json_node, (VectorMathNode&)*node);
        else if (node->type == "VALTORGB")
           readColorRampNodeProperties(json_node, (ColorRampNode&) *node, data_file);
        else if (node->type == "CURVE_RGB")
           readCurveRgbNodeProperties(json_node, (CurveRgbNode&)*node, data_file);

        material->tree_nodes[json_node["Name"].asString()] = std::move(node);
    }

    return material;
}


static MaterialMap readMaterials(const RenderEngine& render_engine, const Json::Value& json_materials, const TextureMap& textures, std::ifstream& data_file)
{
   MaterialMap materials;
   for (const auto& json_material : json_materials)
   {
      Uptr<IMaterial> material;
      if (json_material["Name"].asString() == "Ocean")
         material = std::make_unique<OceanMaterial>();
      else       
         material = readMaterial(render_engine, json_material, textures, data_file);

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

static RotationType convertToRotationType(const std::string& rotation_type)
{
   if (rotation_type == "XYZ")
      return RotationType::EulerXYZ;
   else if (rotation_type == "XZY")
      return RotationType::EulerXZY;

   else if (rotation_type == "YXZ")
      return RotationType::EulerYXZ;
   else if (rotation_type == "YZX")
      return RotationType::EulerYZX;

   else if (rotation_type == "ZXY")
      return RotationType::EulerZXY;
   else if (rotation_type == "ZYX")
      return RotationType::EulerZYX;

   else if (rotation_type == "QUATERNION")
      return RotationType::Quaternion;

   else if (rotation_type == "AXIS_ANGLE")
      return RotationType::AxisAngle;

   else
      assert(false);
   return RotationType::Quaternion;
}

static Transform readTransform(const Json::Value& json_transform)
{
   Transform transform;
   transform.location = readVec3(json_transform["Location"]);
   transform.scale = readVec3(json_transform["Scale"]);
   transform.rotation = readVec4(json_transform["Rotation"]);
   transform.rotation_type = convertToRotationType(json_transform["RotationType"].asString());

   return transform;
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
         bone.local_transform = readTransform(json_bone["Pose"]);
         //bone.local_transform.pose_matrix = readMatrix4x3(json_bone["Pose"]["PoseMatrix"]);

         skeleton->skeleton_to_bone_bind_pose_matrices[i] = readMatrix4x3(json_bone["SkeletonToBoneMatrix"]);
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

         mat4x3 skeleton_to_parent_in_bind_pose_matrix = bone.parent == -1 ? mat4x3(1.0) : skeleton->skeleton_to_bone_bind_pose_matrices[bone.parent];
         bone.parent_to_bone_matrix = composeAS(inverseAS(skeleton_to_parent_in_bind_pose_matrix), skeleton->skeleton_to_bone_bind_pose_matrices[i]);

         ++i;
      }

      skeletons[skeleton->name] = skeleton;
      scene->skeletons.push_back(skeleton);
   }

   int i = 0;
   for (const auto& json_skeleton : json_skeletons)
   {
      scene->name_to_skeleton[json_skeleton["Name"].asString()] = scene->skeletons[i++].get();
   }

   return skeletons;
}

static void readAction(const Json::Value& json_action, Scene* scene)
{
   scene->animation_player->actions.push_back(Action());
   Action& action = scene->animation_player->actions.back();
   action.target_object = json_action["TargetObject"].asString();
   for (const auto& json_curve : json_action["Curves"])
   {
      action.curves.push_back(AnimationCurve());
      AnimationCurve& curve = action.curves.back();

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

static void readActions(const Json::Value& json_actions, Scene* scene)
{
   for (const auto& json_action : json_actions)
   {
      readAction(json_action, scene);
   }
}

void readTransformHierarchy(const Json::Value& json_hierarchy, Scene* scene)
{   
   std::vector<TransformHierarchyNode> nodes(json_hierarchy["Nodes"].size());

   int i = 0;
   int child_counter = 1;
   for (const auto& json_node : json_hierarchy["Nodes"])
   {
      auto& node = nodes[i];
      node.local_transform = readTransform(json_node["Transform"]);
      node.parent_to_node_matrix = readMatrix4x3(json_node["ParentToNodeMatrix"]);
      node.children_count = json_node["Children"].size();
      node.first_child = node.children_count ? child_counter : -1;
      child_counter += node.children_count;
      node.object_name = json_node["ObjectName"].asString();
      scene->object_name_to_transform_node_index[json_node["ObjectName"].asString()] = i;
      
      i++;
   }

   scene->transform_hierarchy = std::make_unique<TransformHierarchy>(std::move(nodes));
}

static void readAOVolume(const Json::Value& json_ao_volume, const std::string& filename, Scene* scene)
{
   scene->ao_volume = std::make_unique<AOVolume>();
   AOVolume& ao_volume = *scene->ao_volume;
   ao_volume.transform_node_index = scene->object_name_to_transform_node_index.at(json_ao_volume["Name"].asString());
   ao_volume.size = readVec3(json_ao_volume["Size"]);
   ao_volume.position = readVec3(json_ao_volume["Position"]);
   ao_volume.resolution = readIVec3(json_ao_volume["Resolution"]);  
   if (json_ao_volume.isMember("Path"))
   {
      std::ifstream ao_volume_file(filename + "\\ao_volume.bin", std::ifstream::binary);
      if (!ao_volume_file.is_open())
      {         
         return;
      }

      int volume_size_in_mem = ao_volume.resolution.x*ao_volume.resolution.y*ao_volume.resolution.z;
      auto buffer = std::make_unique<char[]>(volume_size_in_mem);
      ao_volume_file.read(buffer.get(), volume_size_in_mem);
      ao_volume_file.close();
      ao_volume.ao_texture = createTexture3D(ao_volume.resolution.x, ao_volume.resolution.y, ao_volume.resolution.z,
                                          GL_R8, buffer.get());
   }
}

static void readSDFVolume(const Json::Value& json_sdf_volume, const std::string& filename, Scene* scene)
{
   scene->sdf_volume = std::make_unique<SDFVolume>();
   SDFVolume& sdf_volume = *scene->sdf_volume;
   sdf_volume.transform_node_index = scene->object_name_to_transform_node_index.at(json_sdf_volume["Name"].asString());
   sdf_volume.size = readVec3(json_sdf_volume["Size"]);
   sdf_volume.position = readVec3(json_sdf_volume["Position"]);
   sdf_volume.resolution = readIVec3(json_sdf_volume["Resolution"]);
   if (json_sdf_volume.isMember("Path"))
   {
      std::ifstream sdf_volume_file(filename + "\\sdf_volume.bin", std::ifstream::binary);
      if (!sdf_volume_file.is_open())
      {         
         return;
      }

      int volume_size_in_mem = sdf_volume.resolution.x*sdf_volume.resolution.y*sdf_volume.resolution.z*4;
      auto buffer = std::make_unique<char[]>(volume_size_in_mem);
      sdf_volume_file.read(buffer.get(), volume_size_in_mem);
      sdf_volume_file.close();
      sdf_volume.sdf_texture = createTexture3D(sdf_volume.resolution.x, sdf_volume.resolution.y, sdf_volume.resolution.z,
                                             GL_R16F, buffer.get());
   }
}

void addRandomLights(Scene* scene)
{
   auto real_rand = std::bind(std::uniform_real_distribution<float>(0.0, 1.0), std::mt19937());

   for (int i = 0; i < 100; ++i)
   {
      Light light;      
      light.color = vec3(real_rand(), real_rand(), real_rand())+ vec3(0.05f);
      
      light.strength = real_rand()*5.0f + 10.0f;
      light.world_to_local_matrix = mat4x3(1.0f);
      light.world_to_local_matrix[3] = vec3(real_rand()*10.0f-5.0f, real_rand()*10.0f-5.0f, real_rand());
      
      light.type = LightType::Sphere;
      light.sphere.size = 0.02f+ real_rand()*0.02f;
      /*light.spot.angle = 1.0f;
      light.spot.angle = 0.5f;*/
      scene->lights.push_back(light);
   }
}



void import3DY(const std::string& filename, const RenderEngine& render_engine, Scene* scene)
{
	std::ifstream data_file(filename+"\\data.bin", std::ifstream::binary);

	Json::Value root;
	std::ifstream json_file(filename+"\\structure.json");
	json_file >> root;
	json_file.close();

   readTransformHierarchy(root["TransformHierarchy"], scene);

   if (!root["AOVolume"].isNull())
      readAOVolume(root["AOVolume"], filename, scene);

   if (!root["SDFVolume"].isNull())
      readSDFVolume(root["SDFVolume"], filename, scene);

   readLights(root["Lights"], scene);
   addRandomLights(scene);

   readEnvironment(render_engine, root["Environment"], scene);
   readActions(root["Actions"], scene);
   auto skeletons = readSkeletons(root["Skeletons"], scene);
   auto textures = readTextures(root["Textures"]);
   auto materials = readMaterials(render_engine, root["Materials"], textures, data_file);        
   

	const auto& json_surfaces = root["Surfaces"];
	
   auto default_material = std::make_shared<DefaultMaterial>();
   for (const auto& json_surface : json_surfaces)
	{			
		auto render_mesh = readMesh(json_surface["Mesh"], data_file);
		if (!render_mesh)
			continue;
		//const auto& json_matrix = json_surface["WorldToLocalMatrix"];
		
		SurfaceInstance surface_instance;
      surface_instance.center_in_local_space = readVec3(json_surface["CenterInLocal"]);
		surface_instance.mesh = std::move(render_mesh);
      surface_instance.transform_node_index = scene->object_name_to_transform_node_index.at(json_surface["Name"].asString());

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

      if (scene->ao_volume)
      {
         ((int&)surface_instance.material_variant) |= int(MaterialVariant::EnableAOVolume);
      }

      if (scene->sdf_volume)
      {
         ((int&)surface_instance.material_variant) |= int(MaterialVariant::EnableSDFVolume);
      }

      surface_instance.material_program = &surface_instance.material->compile(surface_instance.material_variant);
		scene->surfaces.push_back(surface_instance);      
	}
}

static std::string _toUppercase(const std::string& input)
{
   std::string output = input;
   for (auto & c : output) c = std::toupper(c);
   return output;
}

static void saveBakedVolumeTo3DY(const std::string& filename, const GLTexture3D& volume_texture, const std::string& volume_type, const Scene& scene)
{
   Json::Value root;
   std::ifstream json_file_as_read(filename + "\\structure.json");
   json_file_as_read >> root;
   json_file_as_read.close();

   int voxels_size = volume_texture.readbackBufferSize();
   auto voxels = std::make_unique<char[]>(voxels_size);
   volume_texture.readbackTexels(voxels.get());

   std::ofstream volume_file(filename + "\\"+volume_type+"_volume.bin", std::ofstream::binary);
   volume_file.write(voxels.get(), voxels_size);
   volume_file.close();

   root[_toUppercase(volume_type)+"Volume"]["Path"] = volume_type + "_volume.bin";

   std::ofstream json_file_as_write(filename + "\\structure.json");
   json_file_as_write << root;
   json_file_as_write.close();
}

void saveBakedAmbientOcclusionVolumeTo3DY(const std::string& filename, const Scene& scene)
{
   saveBakedVolumeTo3DY(filename, *scene.ao_volume->ao_texture, "ao", scene);
}

void saveBakedSignedDistanceFieldVolumeTo3DY(const std::string& filename, const Scene& scene)
{
   saveBakedVolumeTo3DY(filename, *scene.sdf_volume->sdf_texture, "sdf", scene);
}

}