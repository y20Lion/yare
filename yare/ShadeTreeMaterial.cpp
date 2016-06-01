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

void ShadeTreeMaterial::render(const GLVertexSource& mesh_source)
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

   ShadeTreeEvaluation evaluation;
   output_node->evaluate(eval_params, evaluation);

   std::string fragment_shader = _createFragmentShaderCode(evaluation, resources.shade_tree_material_fragment);
   std::string vertex_shader = _createVertexShaderCode(evaluation, resources.shade_tree_material_vertex);

   _program = createProgram(vertex_shader, fragment_shader);
   _is_transparent = evaluation.is_transparent;
}

void ShadeTreeMaterial::bindTextures()
{
   glBindTextures(_first_texture_binding, (GLuint)_used_textures.size(), _used_textures.data());
}

std::string ShadeTreeMaterial::_createFragmentShaderCode(const ShadeTreeEvaluation& evaluation, const std::string& fragment_template)
{
   std::string defines;
   _uses_uv = evaluation.uv_needed;
   if (_uses_uv)
      defines += "#define USE_UV \n";

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

   return string_format(fragment_template, defines.data(), texture_bindings.data(), nodes_shading.data());
}

std::string ShadeTreeMaterial::_createVertexShaderCode(const ShadeTreeEvaluation& evaluation, const std::string& vertex_template)
{
   std::string defines;
   if (evaluation.uv_needed)
      defines += "#define USE_UV \n";

   return string_format(vertex_template, defines.data());
}

}