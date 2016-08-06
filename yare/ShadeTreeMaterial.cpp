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
#include "GLSampler.h"

namespace yare {

ShadeTreeMaterial::ShadeTreeMaterial(const RenderResources& render_resources)
   : _first_texture_binding(5)
   , _is_transparent(false)
   , _uses_uv(false)
   , _render_resources(render_resources)
{
}

ShadeTreeMaterial::~ShadeTreeMaterial()
{

}

int ShadeTreeMaterial::requiredMeshFields(MaterialVariant material_variant)
{
   int fields = int(MeshFieldName::Position) | int(MeshFieldName::Normal);
   if (_uses_uv)
      fields |= int(MeshFieldName::Uv0);

   if (_uses_normal_mapping)
      fields |= int(MeshFieldName::Tangent0);

   if (int(material_variant) & int(MaterialVariant::WithSkinning))
      fields |= int(MeshFieldName::BoneIndices) | int(MeshFieldName::BoneWeights);

   return fields;
}

const GLProgram& ShadeTreeMaterial::compile(MaterialVariant material_variant)
{
   auto prog_it =_program_variants.find(material_variant);
   if (prog_it != _program_variants.end())
      return *prog_it->second;

   auto is_output_node = [](const auto& name_node_pair)
   {
      return name_node_pair.second->type == "OUTPUT_MATERIAL";
   };
   auto node_it = std::find_if(RANGE(tree_nodes), is_output_node);
   ShadeTreeNode* output_node = node_it->second.get();

   ShadeTreeParams eval_params;
   eval_params.tree_nodes = &tree_nodes;
   eval_params.texture_binding_slot_start = _first_texture_binding;
   eval_params.samplers = &_render_resources.samplers;
   _markNodesThatNeedPixelDifferentials(*output_node, false);

   ShadeTreeEvaluation evaluation;
   output_node->evaluate(eval_params, evaluation);
   _is_transparent = evaluation.is_transparent;
   _uses_normal_mapping = evaluation.normal_mapping_needed;
   _uses_uv = evaluation.uv_needed;   

   std::string program_defines = _buildProgramDefinesString(material_variant);   
   std::string fragment_shader = _createFragmentShaderCode(evaluation, _render_resources.shade_tree_material_fragment, program_defines);
   std::string vertex_shader = _createVertexShaderCode(evaluation, _render_resources.shade_tree_material_vertex, program_defines);

   _program_variants[material_variant] = createProgram(vertex_shader, fragment_shader);
   return *_program_variants[material_variant];
}

void ShadeTreeMaterial::bindTextures()
{
   glBindTextures(_first_texture_binding, (GLuint)_used_textures.size(), _used_textures.data()); 
   glBindSamplers(_first_texture_binding, (GLuint)_used_samplers.size(), _used_samplers.data());
}

std::string ShadeTreeMaterial::_createFragmentShaderCode(const ShadeTreeEvaluation& evaluation, const std::string& fragment_template, const std::string& defines)
{
   std::string texture_bindings;
   _used_textures.clear();
   _used_samplers.clear();
   for (const auto& glsl_texture : evaluation.glsl_textures)
   {
      _used_textures.push_back(std::get<0>(glsl_texture)->id());
      _used_samplers.push_back(std::get<1>(glsl_texture)->id());
      texture_bindings.append(std::get<2>(glsl_texture));
   }

   std::string nodes_shading;
   for (const std::string& glsl : evaluation.glsl_code)
      nodes_shading.append(glsl);

   return string_format(fragment_template, defines, texture_bindings, nodes_shading);
}

std::string ShadeTreeMaterial::_createVertexShaderCode(const ShadeTreeEvaluation& evaluation, const std::string& vertex_template, const std::string& defines)
{
   return string_format(vertex_template, defines );
}

std::string ShadeTreeMaterial::_buildProgramDefinesString(MaterialVariant material_variant)
{
   std::string defines;
   if (_uses_uv)
      defines += "#define USE_UV \n";

   if (_uses_normal_mapping)
      defines += "#define USE_NORMAL_MAPPING \n";

   if (int(material_variant) & int(MaterialVariant::WithSkinning))
      defines += "#define USE_SKINNING \n";

   return defines;
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