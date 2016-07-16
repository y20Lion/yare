#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <GL/glew.h>

#include "tools.h"
#include "IMaterial.h"

namespace yare {

class GLProgram;
class ShadeTreeNode;
struct ShadeTreeEvaluation;
struct RenderResources;

class ShadeTreeMaterial : public IMaterial
{
public:
   ShadeTreeMaterial(const RenderResources& render_resources);
   virtual ~ShadeTreeMaterial();
   std::string name;

   virtual int requiredMeshFields(MaterialVariant material_variant) override;
   virtual const GLProgram& compile(MaterialVariant material_variant) override;
   virtual void render(const GLVertexSource& mesh_source, const GLProgram& program) override;
   
   std::map<std::string, std::unique_ptr<ShadeTreeNode>> tree_nodes;

   void bindTextures();
   bool isTransparent() const { return _is_transparent; }

private:
   std::string _createVertexShaderCode(const ShadeTreeEvaluation& evaluation, const std::string& fragment_template, const std::string& defines);
   std::string _createFragmentShaderCode(const ShadeTreeEvaluation& evaluation, const std::string& vertex_template, const std::string& defines);
   std::string _buildProgramDefinesString(MaterialVariant material_variant);
   void _markNodesThatNeedPixelDifferentials(ShadeTreeNode& node, bool parent_needs_pixels_differentials);

private:
   const RenderResources& _render_resources;
   std::map<MaterialVariant, Uptr<GLProgram>> _program_variants;
   
   std::vector<GLuint> _used_textures;
   std::vector<GLuint> _used_samplers;
   int _first_texture_binding;
   bool _is_transparent;
   bool _uses_uv;
   bool _uses_normal_mapping;
};

}