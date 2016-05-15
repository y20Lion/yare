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

enum class NodeOutputType {Shading, Color, Normal, Vector, Float};

class NodeOutputValue
{
public:
    NodeOutputValue(NodeOutputType type) : type(type) {}
    virtual ~NodeOutputValue() {}
    NodeOutputType type;
    std::string expression;
};

class Shading : public NodeOutputValue
{
public:
    Shading(const std::string& glsl_name) : NodeOutputValue(NodeOutputType::Shading), has_additive(true), has_transparency_factor(false), additive(glsl_name), transparency_factor("vec3(0.0)") {}
    Shading() : NodeOutputValue(NodeOutputType::Shading), has_additive(false), has_transparency_factor(false), additive("vec3(0.0)"), transparency_factor("vec3(0.0)") {}
    virtual ~Shading(){}
    
    std::string additive;
    bool has_additive;
    std::string transparency_factor;
    bool has_transparency_factor;
};

class Color : public NodeOutputValue
{
public:
    Color() : NodeOutputValue(NodeOutputType::Color) {}
    Color(const std::string& expression_) 
        : NodeOutputValue(NodeOutputType::Color) { expression = expression_; }
    virtual ~Color() {}
};

class Normal : public NodeOutputValue
{
public:
    Normal() : NodeOutputValue(NodeOutputType::Normal) {}
    Normal(const std::string& expression_) : NodeOutputValue(NodeOutputType::Normal) { expression = expression_; }
    virtual ~Normal() {}
};

class Vector : public NodeOutputValue
{
public:
    Vector() : NodeOutputValue(NodeOutputType::Vector) {}
    Vector(const std::string& expression_) : NodeOutputValue(NodeOutputType::Vector) { expression = expression_; }
    virtual ~Vector() {}
};

class Float : public NodeOutputValue
{
public:
    Float() : NodeOutputValue(NodeOutputType::Float) {}
    Float(const std::string& expression_) : NodeOutputValue(NodeOutputType::Float) { expression = expression_; }
    virtual ~Float() {}
};

typedef std::map<std::string, std::shared_ptr<NodeOutputValue>> NodeEvaluatedOutputs;
struct ShadeTreeEvaluation
{    
    ShadeTreeEvaluation() : uv_needed(false), is_transparent(false) {}
    
    std::map<std::string, NodeEvaluatedOutputs> evaluted_nodes;
    std::vector<std::string> glsl_code;    
    std::vector<std::pair<Sptr<GLTexture>, std::string>> glsl_textures;
    bool uv_needed;
    bool is_transparent;

    NodeEvaluatedOutputs& addNodeCode(const std::string& node_name, const std::string& node_code);
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
	virtual const NodeEvaluatedOutputs& evaluate(const ShadeTreeParams& params, ShadeTreeEvaluation& evaluation) = 0;
	std::string type;
	std::string name;
	std::map<std::string, ShadeTreeNodeSlot> input_slots;
	std::map<std::string, ShadeTreeNodeSlot> output_slots;

protected:
    template <typename TValue>
    TValue _evaluateInputSlot(const ShadeTreeParams& params, const std::string& slot_name, ShadeTreeEvaluation& evaluation);

    void _evaluateShading(const Shading& shading0, const Shading& shading1,
        const std::string& expression, Shading* result, std::string* node_glsl_code);
};

Uptr<ShadeTreeNode> createShadeTreeNode(const std::string& node_type);

////////////////// All node types /////////////////

class OutputNode : public ShadeTreeNode
{
public:
    OutputNode() : ShadeTreeNode("OUTPUT_MATERIAL") {}
    virtual const NodeEvaluatedOutputs& evaluate(const ShadeTreeParams& params, ShadeTreeEvaluation& evaluation) override;
};

class DiffuseBSDFNode : public ShadeTreeNode
{
public:
	DiffuseBSDFNode() : ShadeTreeNode("BSDF_DIFFUSE") {}
	virtual const NodeEvaluatedOutputs& evaluate(const ShadeTreeParams& params, ShadeTreeEvaluation& evaluation) override;
};

class GlossyBSDFNode : public ShadeTreeNode
{
public:
   GlossyBSDFNode() : ShadeTreeNode("BSDF_GLOSSY") {}
   virtual const NodeEvaluatedOutputs& evaluate(const ShadeTreeParams& params, ShadeTreeEvaluation& evaluation) override;
};

class TransparentBSDFNode : public ShadeTreeNode
{
public:
    TransparentBSDFNode() : ShadeTreeNode("BSDF_TRANSPARENT") {}
    virtual const NodeEvaluatedOutputs& evaluate(const ShadeTreeParams& params, ShadeTreeEvaluation& evaluation) override;
};

class MixBSDFNode : public ShadeTreeNode
{
public:
    MixBSDFNode() : ShadeTreeNode("MIX_SHADER") {}
    virtual const NodeEvaluatedOutputs& evaluate(const ShadeTreeParams& params, ShadeTreeEvaluation& evaluation) override;
};

class AddBSDFNode : public ShadeTreeNode
{
public:
    AddBSDFNode() : ShadeTreeNode("ADD_SHADER") {}
    virtual const NodeEvaluatedOutputs& evaluate(const ShadeTreeParams& params, ShadeTreeEvaluation& evaluation) override;
};

class TexImageNode : public ShadeTreeNode
{
public:
    TexImageNode() : ShadeTreeNode("TEX_IMAGE") {}
    glm::mat3x2 texture_transform;
    Sptr<GLTexture> texture;
    virtual const NodeEvaluatedOutputs& evaluate(const ShadeTreeParams& params, ShadeTreeEvaluation& evaluation) override;
};

/*class UvSourceNode : public ShadeTreeNode
{
public:
    UvSourceNode() : ShadeTreeNode("UV_MAP") {}
    virtual ShadingResult evaluate(const ShadeTreeNodes& tree_nodes, const std::string& output_slot, ShadeTreeEvaluation& evaluation) override;
};*/

}
