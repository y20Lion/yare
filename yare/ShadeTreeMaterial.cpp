#include "ShadeTreeMaterial.h"

#include <algorithm>

#include "stl_helpers.h"
#include "GLProgram.h"
#include "GLTexture.h"
#include "ShadeTreeNode.h"
#include "RenderResources.h"
#include "RenderMesh.h"
#include "GLDevice.h"
#include "GLVertexSource.h"

namespace yare {

ShadeTreeMaterial::ShadeTreeMaterial()
   : _first_texture_binding(5)
   , _is_transparent(false)
   , _uses_uv(false)
{
}

ShadeTreeMaterial::~ShadeTreeMaterial()
{

}

void ShadeTreeMaterial::render(const RenderResources& resources, const GLVertexSource& mesh_source)
{   
   bindTextures();
   GLDevice::bindProgram(program());   
   GLDevice::bindVertexSource(mesh_source);

   if (isTransparent())
   {
      GLDevice::bindColorBlendState({ GLBlendingMode::ModulateAdd });
   }

   GLDevice::draw(0, mesh_source.vertexCount());

   if (isTransparent())
   {
      GLDevice::bindDefaultColorBlendState();
   }
}

int ShadeTreeMaterial::requiredMeshFields()
{
   int fields = int(MeshFieldName::Position) | int(MeshFieldName::Normal);
   if (_uses_uv)
      fields |= int(MeshFieldName::Uv0);

   if (_uses_normal_mapping)
      fields |= int(MeshFieldName::Tangent0);

   //if (_uses_skinning)
      fields |= int(MeshFieldName::BoneIndices) | int(MeshFieldName::BoneWeights);
   
   return fields;   
}

void ShadeTreeMaterial::compile(const RenderResources& resources)
{
   auto is_output_node = [](const auto& name_node_pair)
   {
      return name_node_pair.second->type == "OUTPUT_MATERIAL";
   };
   auto node_it = std::find_if(RANGE(tree_nodes), is_output_node);
   ShadeTreeNode* output_node = node_it->second.get();

   ShadeTreeParams eval_params;
   eval_params.tree_nodes = &tree_nodes;
   eval_params.texture_binding_slot_start = _first_texture_binding;

   _markNodesThatNeedPixelDifferentials(*output_node, false);

   ShadeTreeEvaluation evaluation;
   output_node->evaluate(eval_params, evaluation);
   _is_transparent = evaluation.is_transparent;
   _uses_normal_mapping = evaluation.normal_mapping_needed;
   _uses_uv = evaluation.uv_needed;   
   _buildProgramDefinesString();
   
   std::string fragment_shader = _createFragmentShaderCode(evaluation, resources.shade_tree_material_fragment);
   std::string vertex_shader = _createVertexShaderCode(evaluation, resources.shade_tree_material_vertex);

   _program = createProgram(vertex_shader, fragment_shader);
   
}

void ShadeTreeMaterial::bindTextures()
{
   glBindTextures(_first_texture_binding, (GLuint)_used_textures.size(), _used_textures.data());   
}

std::string ShadeTreeMaterial::_createFragmentShaderCode(const ShadeTreeEvaluation& evaluation, const std::string& fragment_template)
{
   std::string texture_bindings;
   _used_textures.clear();
   for (const auto& glsl_texture : evaluation.glsl_textures)
   {
      _used_textures.push_back(glsl_texture.first->id());
      texture_bindings.append(glsl_texture.second);
   }

   std::string nodes_shading;
   for (const std::string& glsl : evaluation.glsl_code)
      nodes_shading.append(glsl);

   return string_format(fragment_template, _defines, texture_bindings, nodes_shading);
}

std::string ShadeTreeMaterial::_createVertexShaderCode(const ShadeTreeEvaluation& evaluation, const std::string& vertex_template)
{
   return string_format(vertex_template, _defines);
}

void ShadeTreeMaterial::_buildProgramDefinesString()
{
   _defines.clear();
   if (_uses_uv)
      _defines += "#define USE_UV \n";

   if (_uses_normal_mapping)
      _defines += "#define USE_NORMAL_MAPPING \n";
}

void ShadeTreeMaterial::_markNodesThatNeedPixelDifferentials(ShadeTreeNode& node, bool parent_needs_pixels_differentials)
{
   if (parent_needs_pixels_differentials)
   {
      node.compute_pixel_differentials = true;
   }

   bool child_need_to_compute_differentials = parent_needs_pixels_differentials || node.type == "BUMP";
   for (const auto& input : node.input_slots)
   {
      if (!input.second.links.empty())
      {
         auto& child_node = tree_nodes.at(input.second.links[0].node_name);
         _markNodesThatNeedPixelDifferentials(*child_node, child_need_to_compute_differentials);
      }
   }
}

}