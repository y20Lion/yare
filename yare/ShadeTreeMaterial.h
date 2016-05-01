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

class ShadeTreeMaterial
{
public:
    ShadeTreeMaterial();
    ~ShadeTreeMaterial();
    std::string name;


    void compile();

    std::map<std::string, std::unique_ptr<ShadeTreeNode>> tree_nodes;
    
    const GLProgram& program() const { return *_program; }
    
    void bindTextures();

private:
    Uptr<GLProgram> _program;
    std::vector<GLuint> _used_textures;
    int _first_texture_binding;
};

}