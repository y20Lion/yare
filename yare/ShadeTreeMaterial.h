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
   ShadeTreeMaterial();
   virtual ~ShadeTreeMaterial();
   std::string name;
   
   virtual void render(const RenderResources& resources, const GLVertexSource& mesh_source) override;
   virtual int requiredMeshFields() override;

   void compile(const RenderResources& resources);

   std::map<std::string, std::unique_ptr<ShadeTreeNode>> tree_nodes;

   const GLProgram& program() const { return *_program; }

   void bindTextures();
   bool isTransparent() const { return _is_transparent; }

private:
   std::string _createVertexShaderCode(const ShadeTreeEvaluation& evaluation, const std::string& fragment_template);
   std::string _createFragmentShaderCode(const ShadeTreeEvaluation& evaluation, const std::string& vertex_template);
   void _buildProgramDefinesString();
   void _markNodesThatNeedPixelDifferentials(ShadeTreeNode& node, bool parent_needs_pixels_differentials);

private:

   Uptr<GLProgram> _program;
   std::vector<GLuint> _used_textures;
   int _first_texture_binding;
   bool _is_transparent;
   bool _uses_uv;
   bool _uses_normal_mapping;
   std::string _defines;
};

}