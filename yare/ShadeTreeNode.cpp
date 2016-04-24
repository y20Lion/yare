#include "ShadeTreeNode.h"

#include <algorithm>
#include <assert.h>
#include "stl_helpers.h"

static std::string _toGLSLVarName(std::string node_name, std::string slot_name)
{
    std::replace(RANGE(node_name), ' ', '_');
    std::replace(RANGE(node_name), '.', '_');

    std::replace(RANGE(slot_name), ' ', '_');
    std::replace(RANGE(slot_name), '.', '_');

    return node_name + "_Node_" + slot_name;
}

bool ShadeTreeEvaluation::isNodeAlreadyEvaluated(const std::string& node_name) const
{
    return (evaluted_nodes.find(node_name) != evaluted_nodes.end());
}

void ShadeTreeEvaluation::addNodeCode(const std::string& node_name, const std::string& node_code)
{
    glsl_code.push_back(node_code);
    evaluted_nodes.insert(node_name);
}

std::string ShadeTreeNode::_evaluateInputSlot(const ShadeTreeNodes& tree_nodes,
                                              const std::string& slot_name,
                                              ShadeTreeEvaluation& evaluation)
{
    const auto& input_slot = input_slots.at(slot_name);
    const auto& input_links = input_slot.links;
    bool no_link = input_links.empty();
    if (no_link)
	{
        switch (input_slot.type)
        {
            case ShadeTreeNodeSlotType::Normal:
                return "attr_normal";
            case ShadeTreeNodeSlotType::Vec3: 
                return "vec3(" + std::to_string(input_slot.default_value.x) + ", "
                               + std::to_string(input_slot.default_value.y) + ", "
                               + std::to_string(input_slot.default_value.z) + ")";
            case ShadeTreeNodeSlotType::Float: 
                return std::to_string(input_slot.default_value.x);
            default: assert(false); return "";
        }            
    }		
    else
    {
        tree_nodes.at(input_links[0].node_name)->evaluate(tree_nodes, input_links[0].slot_name, evaluation);		
        return _toGLSLVarName(input_links[0].node_name, input_links[0].slot_name);
	}
}

Uptr<ShadeTreeNode> createShadeTreeNode(const std::string& node_type)
{
    if (node_type == "OUTPUT_MATERIAL")
        return std::make_unique<OutputNode>();
    else if (node_type == "BSDF_DIFFUSE")
        return std::make_unique<DiffuseBSDFNode>();
    else if (node_type == "MIX_SHADER")
        return std::make_unique<MixBSDFNode>();
    else if (node_type == "ADD_SHADER")
        return std::make_unique<AddBSDFNode>();
    else
        return nullptr;
}


void OutputNode::evaluate(const ShadeTreeNodes& tree_nodes, const std::string & output_slot, ShadeTreeEvaluation & evaluation)
{
    if (evaluation.isNodeAlreadyEvaluated(name))
        return;

    std::string shading = _evaluateInputSlot(tree_nodes, "Surface", evaluation);
    std::string node_glsl_code = "vec3 "+ output_slot +" = " + shading + ";\n";
    evaluation.addNodeCode(name, node_glsl_code);
}

void DiffuseBSDFNode::evaluate(const ShadeTreeNodes& tree_nodes, const std::string& output_slot, ShadeTreeEvaluation& evaluation)
{
	if (evaluation.isNodeAlreadyEvaluated(name))
		return;

	std::string color = _evaluateInputSlot(tree_nodes, "Color", evaluation);
	std::string normal = _evaluateInputSlot(tree_nodes, "Normal", evaluation);
    std::string node_glsl_code = "vec3 "+ _toGLSLVarName(name, output_slot) +" = evalDiffuseBSDF("+ color +", "+ normal +");\n";

    evaluation.addNodeCode(name, node_glsl_code);
}

void MixBSDFNode::evaluate(const ShadeTreeNodes& tree_nodes, const std::string& output_slot, ShadeTreeEvaluation& evaluation)
{
    if (evaluation.isNodeAlreadyEvaluated(name))
        return;

    std::string shading0 = _evaluateInputSlot(tree_nodes, "Shader", evaluation);
    std::string shading1 = _evaluateInputSlot(tree_nodes, "Shader_001", evaluation);
    std::string mix_factor = _evaluateInputSlot(tree_nodes, "Fac", evaluation);
    std::string node_glsl_code = "vec3 " + _toGLSLVarName(name, output_slot) +" = mix("+  shading0 +", "+ shading1 +", "+ mix_factor +");\n";

    evaluation.addNodeCode(name, node_glsl_code);
}

void AddBSDFNode::evaluate(const ShadeTreeNodes& tree_nodes, const std::string& output_slot, ShadeTreeEvaluation& evaluation)
{
    if (evaluation.isNodeAlreadyEvaluated(name))
        return;

    std::string shading0 = _evaluateInputSlot(tree_nodes, "Shader", evaluation);
    std::string shading1 = _evaluateInputSlot(tree_nodes, "Shader_001", evaluation);
    std::string node_glsl_code = "vec3 " + _toGLSLVarName(name, output_slot) + " = " + shading0 + " + " + shading1 +";\n";

    evaluation.addNodeCode(name, node_glsl_code);
}

