#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <glm/vec4.hpp>
#include <glm/mat3x2.hpp>

#include "tools.h"

namespace yare {

class GLTexture;

enum class ShadeTreeNodeSlotType { Float, Vec3, Normal};

struct Link
{
    Link() {}

    std::string node_name;
    std::string slot_name;
};

struct ShadeTreeNodeSlot
{
	std::string name;
	ShadeTreeNodeSlotType type;
	glm::vec4 default_value;
	std::vector<Link> links;
};

enum class ShadeTreeNodeType {DiffuseBSDF, GlossyBSDF, OutputMaterial, MixBSDF, AddBSDF};

struct ShadeTreeEvaluation
{    
    ShadeTreeEvaluation() : uv_needed(false) {}
    
    std::set<std::string> evaluted_nodes;
    std::vector<std::string> glsl_code;    
    std::vector<std::pair<Sptr<GLTexture>, std::string>> glsl_textures;
    bool uv_needed;

    bool isNodeAlreadyEvaluated(const std::string& node_name) const;
    void addNodeCode(const std::string& node_name, const std::string& node_code);
};


class ShadeTreeNode;
typedef std::map<std::string, std::unique_ptr<ShadeTreeNode>> ShadeTreeNodes;
struct ShadeTreeParams
{
    ShadeTreeNodes* tree_nodes;
    int texture_binding_slot_start;
};



class ShadeTreeNode
{
public:
    ShadeTreeNode(const std::string& type) : type(type) {}
    virtual ~ShadeTreeNode() {}
	virtual void evaluate(const ShadeTreeParams& params, const std::string& output_slot, ShadeTreeEvaluation& evaluation) = 0;
	std::string type;
	std::string name;
	std::map<std::string, ShadeTreeNodeSlot> input_slots;
	std::map<std::string, ShadeTreeNodeSlot> output_slots;

protected:
	std::string _evaluateInputSlot(const ShadeTreeParams& params, const std::string& slot_name, ShadeTreeEvaluation& evaluation);
};

Uptr<ShadeTreeNode> createShadeTreeNode(const std::string& node_type);

////////////////// All node types /////////////////

class OutputNode : public ShadeTreeNode
{
public:
    OutputNode() : ShadeTreeNode("OUTPUT_MATERIAL") {}
    virtual void evaluate(const ShadeTreeParams& params, const std::string& output_slot, ShadeTreeEvaluation& evaluation);
};

class DiffuseBSDFNode : public ShadeTreeNode
{
public:
	DiffuseBSDFNode() : ShadeTreeNode("BSDF_DIFFUSE") {}
	virtual void evaluate(const ShadeTreeParams& params, const std::string& output_slot, ShadeTreeEvaluation& evaluation);
};

class MixBSDFNode : public ShadeTreeNode
{
public:
    MixBSDFNode() : ShadeTreeNode("MIX_SHADER") {}
    virtual void evaluate(const ShadeTreeParams& params, const std::string& output_slot, ShadeTreeEvaluation& evaluation);
};

class AddBSDFNode : public ShadeTreeNode
{
public:
    AddBSDFNode() : ShadeTreeNode("ADD_SHADER") {}
    virtual void evaluate(const ShadeTreeParams& params, const std::string& output_slot, ShadeTreeEvaluation& evaluation);
};

class TexImageNode : public ShadeTreeNode
{
public:
    TexImageNode() : ShadeTreeNode("TEX_IMAGE") {}
    glm::mat3x2 texture_transform;
    Sptr<GLTexture> texture;
    virtual void evaluate(const ShadeTreeParams& params, const std::string& output_slot, ShadeTreeEvaluation& evaluation);
};

class UvSourceNode : public ShadeTreeNode
{
public:
    UvSourceNode() : ShadeTreeNode("UV_MAP") {}
    virtual void evaluate(const ShadeTreeNodes& tree_nodes, const std::string& output_slot, ShadeTreeEvaluation& evaluation);
};

}
