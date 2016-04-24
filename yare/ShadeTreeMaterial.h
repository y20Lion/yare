#pragma once

#include <map>
#include <memory>
#include <vector>

#include "tools.h"

class GLProgram;
class ShadeTreeNode;

class ShadeTreeMaterial
{
public:
    ShadeTreeMaterial();
    ~ShadeTreeMaterial();

    void compile();

    std::map<std::string, std::unique_ptr<ShadeTreeNode>> tree_nodes;

    const GLProgram& program() const { return *_program; }
private:
    Uptr<GLProgram> _program;
};
