#include "ShadeTreeNode.h"

#include <algorithm>
#include <sstream>
#include <assert.h>
#include "stl_helpers.h"

namespace yare {

using namespace glm;

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

std::string ShadeTreeNode::_evaluateInputSlot(const ShadeTreeParams& params,
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
        params.tree_nodes->at(input_links[0].node_name)->evaluate(params, input_links[0].slot_name, evaluation);		
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
    else if (node_type == "TEX_IMAGE")
        return std::make_unique<TexImageNode>();
    else
    {
        assert(false);
        return nullptr;
    }        
}


void OutputNode::evaluate(const ShadeTreeParams& params, const std::string & output_slot, ShadeTreeEvaluation & evaluation)
{
    if (evaluation.isNodeAlreadyEvaluated(name))
        return;

    std::string shading = _evaluateInputSlot(params, "Surface", evaluation);
    std::string node_glsl_code = "vec3 "+ output_slot +" = " + shading + ";\n";
    evaluation.addNodeCode(name, node_glsl_code);
}

void DiffuseBSDFNode::evaluate(const ShadeTreeParams& params, const std::string& output_slot, ShadeTreeEvaluation& evaluation)
{
	if (evaluation.isNodeAlreadyEvaluated(name))
		return;

	std::string color = _evaluateInputSlot(params, "Color", evaluation);
	std::string normal = _evaluateInputSlot(params, "Normal", evaluation);
    std::string node_glsl_code = "vec3 "+ _toGLSLVarName(name, output_slot) +" = evalDiffuseBSDF("+ color +", "+ normal +");\n";

    evaluation.addNodeCode(name, node_glsl_code);
}

void MixBSDFNode::evaluate(const ShadeTreeParams& params, const std::string& output_slot, ShadeTreeEvaluation& evaluation)
{
    if (evaluation.isNodeAlreadyEvaluated(name))
        return;

    std::string shading0 = _evaluateInputSlot(params, "Shader", evaluation);
    std::string shading1 = _evaluateInputSlot(params, "Shader_001", evaluation);
    std::string mix_factor = _evaluateInputSlot(params, "Fac", evaluation);
    std::string node_glsl_code = "vec3 " + _toGLSLVarName(name, output_slot) +" = mix("+  shading0 +", "+ shading1 +", "+ mix_factor +");\n";

    evaluation.addNodeCode(name, node_glsl_code);
}

void AddBSDFNode::evaluate(const ShadeTreeParams& params, const std::string& output_slot, ShadeTreeEvaluation& evaluation)
{
    if (evaluation.isNodeAlreadyEvaluated(name))
        return;

    std::string shading0 = _evaluateInputSlot(params, "Shader", evaluation);
    std::string shading1 = _evaluateInputSlot(params, "Shader_001", evaluation);
    std::string node_glsl_code = "vec3 " + _toGLSLVarName(name, output_slot) + " = " + shading0 + " + " + shading1 +";\n";

    evaluation.addNodeCode(name, node_glsl_code);
}

std::string _toGLSLMatrix(const mat3x2& mat)
{
    std::stringstream stream;
    stream <<  std::string("mat3x2(")
        << mat[0].x << ", " << mat[0].y << ", " 
        << mat[1].x << ", " << mat[1].y << ", " 
        << mat[2].x << ", " << mat[2].y << ")";
    return stream.str();
}

void TexImageNode::evaluate(const ShadeTreeParams& params, const std::string& output_slot, ShadeTreeEvaluation& evaluation)
{
    if (evaluation.isNodeAlreadyEvaluated(name))
        return;

    //std::string uv = _evaluateInputSlot(params, "Vector", evaluation);
    evaluation.uv_needed = true;

    std::string glsl_color = _toGLSLVarName(name, "Color");
    std::string glsl_alpha = _toGLSLVarName(name, "Alpha");
    
    std::string node_glsl_code = "vec3 "+ glsl_color +";\n";
    node_glsl_code += "float " + glsl_alpha + ";\n";
    node_glsl_code += "sampleTexture("+ _toGLSLVarName(name, "Texture")
        +","+ /*uv */"vec3(attr_uv, 1.0)" // TODO Yvain handle multi uv
        +", "+ _toGLSLMatrix(texture_transform)
        +","+ _toGLSLVarName(name, "Color")
        +","+ _toGLSLVarName(name, "Alpha") +");\n";
    
    int bind_slot = int(evaluation.glsl_textures.size()) + params.texture_binding_slot_start;
    std::string sampler_definition = "layout(binding="+ std::to_string(bind_slot) +")uniform sampler2D " + _toGLSLVarName(name, "Texture") +";\n";
    evaluation.glsl_textures.push_back(std::make_pair(texture, sampler_definition));
    
    evaluation.addNodeCode(name, node_glsl_code);
}

}