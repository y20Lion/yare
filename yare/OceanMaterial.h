#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <GL/glew.h>

#include "tools.h"

namespace yare {

class GLProgram;
class ShadeTreeNode;
struct ShadeTreeEvaluation;
struct RenderResources;

class OceanMaterial
{
public:
   ShadeTreeMaterial();
   ~ShadeTreeMaterial();
   std::string name;


   void compile(const RenderResources& resources);

   std::map<std::string, std::unique_ptr<ShadeTreeNode>> tree_nodes;

   const GLProgram& program() const { return *_program; }

   void bindTextures();
   bool isTransparent() const { return _is_transparent; }

private:
   std::string _createVertexShaderCode(const ShadeTreeEvaluation& evaluation, const std::string& fragment_template);
   std::string _createFragmentShaderCode(const ShadeTreeEvaluation& evaluation, const std::string& vertex_template);

private:

   Uptr<GLProgram> _program;
   std::vector<GLuint> _used_textures;
   int _first_texture_binding;
   bool _is_transparent;
};

}