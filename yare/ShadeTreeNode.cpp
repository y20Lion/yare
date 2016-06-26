#include "ShadeTreeNode.h"

#include <algorithm>
#include <sstream>
#include <assert.h>
#include "stl_helpers.h"

namespace yare {

static const std::string _dx = "_atDx";
static const std::string _dy = "_atDy";

using namespace glm;

static std::string _toGLSLVarName(std::string node_name, std::string slot_name, bool transparency_factor = false)
{
    std::replace(RANGE(node_name), ' ', '_');
    std::replace(RANGE(node_name), '.', '_');

    std::replace(RANGE(slot_name), ' ', '_');
    std::replace(RANGE(slot_name), '.', '_');

    if (transparency_factor)
        return node_name + "_NodeTransFact_" + slot_name;
    else
        return node_name + "_Node_" + slot_name;
}

NodeEvaluatedOutputs& ShadeTreeEvaluation::addNodeCode(const std::string& node_name, const std::string& node_code)
{
    glsl_code.push_back(node_code);
    return evaluted_nodes[node_name];
}

template <typename TValue>
TValue _defaultValue(const glm::vec4& default_value)
{
    return TValue("vec3(" + std::to_string(default_value.x) + ", "
        + std::to_string(default_value.y) + ", "
        + std::to_string(default_value.z) + ")");
}
template <>
Normal _defaultValue<Normal>(const glm::vec4& value)
{
    return Normal("normal");
}

template <>
Float _defaultValue<Float>(const glm::vec4& value)
{
    return Float(std::to_string(value.x));
}

template <typename TValue>
TValue _convertTo(const NodeOutputValue& evaluation)
{
    switch (evaluation.type)
    {
        case NodeOutputType::Shading:
            return TValue(((const Shading&)evaluation).additive);
        case NodeOutputType::Normal:
        case NodeOutputType::Color:
        case NodeOutputType::Vector:
            return TValue(evaluation.expression);
        case NodeOutputType::Float:
            return TValue("vec3("+evaluation.expression+")");
        default:
            assert(false);  return TValue();
    }
}

template <>
Float _convertTo<Float>(const NodeOutputValue& evaluation)
{
    switch (evaluation.type)
    {
    case NodeOutputType::Shading:
        return Float("vec3ToFloat("+ ((const Shading&)evaluation).additive +")");
    case NodeOutputType::Normal:        
    case NodeOutputType::Color:
    case NodeOutputType::Vector:
        return Float("vec3ToFloat("+ evaluation.expression +")");
    case NodeOutputType::Float:
        return evaluation.expression;
    default:
        assert(false);  return Float();
    }
}

template <>
Shading _convertTo<Shading>(const NodeOutputValue& evaluation)
{
    switch (evaluation.type)
    {
    case NodeOutputType::Shading:
        return Shading((const Shading&)evaluation);
    case NodeOutputType::Normal:
    case NodeOutputType::Color:
    case NodeOutputType::Vector:
        return Shading(evaluation.expression);
    case NodeOutputType::Float:
        return Shading("vec3(" + evaluation.expression + ")");
    default:
        assert(false);  return Shading();
    }
}

template <typename TValue>
TValue ShadeTreeNode::_evaluateInputSlot(const ShadeTreeParams& params,
                                              const std::string& slot_name,
                                              ShadeTreeEvaluation& evaluation)
{
    const auto& input_slot = input_slots.at(slot_name);
    const auto& input_links = input_slot.links;
    bool no_link = input_links.empty();
    if (no_link)
	{
        return _defaultValue<TValue>(input_slot.default_value);
    }		
    else
    {
        const NodeEvaluatedOutputs& node_outputs_eval = params.tree_nodes->at(input_links[0].node_name)->evaluate(params, evaluation);        
        return _convertTo<TValue>(*node_outputs_eval.at(input_links[0].slot_name));
	}
}

template <typename TValue>
void ShadeTreeNode::_evaluateInputSlot(const ShadeTreeParams& params,
   const std::string& slot_name,
   ShadeTreeEvaluation& evaluation, std::string& v, std::string& v_atDx, std::string& v_atDy)
{
   const auto& input_slot = input_slots.at(slot_name);
   const auto& input_links = input_slot.links;
   bool no_link = input_links.empty();
   if (no_link)
   {
      v =  v_atDx = v_atDy = _defaultValue<TValue>(input_slot.default_value).expression;
   }
   else
   {
      NodeEvaluatedOutputs& node_outputs_eval = const_cast<NodeEvaluatedOutputs&>(params.tree_nodes->at(input_links[0].node_name)->evaluate(params, evaluation));
      auto& slot = *node_outputs_eval.at(input_links[0].slot_name);
      auto save_expresssion = slot.expression;
      v = _convertTo<TValue>(slot).expression;
      slot.expression = save_expresssion + _dx;
      v_atDx = _convertTo<TValue>(slot).expression;
      slot.expression = save_expresssion + _dy;
      v_atDy = _convertTo<TValue>(slot).expression;
      slot.expression = save_expresssion;
   }
}


void ShadeTreeNode::_evaluateShading(const Shading& shading0, const Shading& shading1, const std::string& expression, Shading* result, std::string* node_glsl_code)
{
    const std::string& output_slot = output_slots.begin()->first;
    
    if (shading0.has_additive || shading1.has_additive)
    {
        *node_glsl_code += "vec3 " + _toGLSLVarName(name, output_slot) + " = " + string_format(expression, shading0.additive, shading1.additive) + ";\n";
        result->additive = _toGLSLVarName(name, output_slot);
        result->has_additive = true;
    }

    if (shading0.has_transparency_factor || shading1.has_transparency_factor)
    {
        *node_glsl_code += "vec3 " + _toGLSLVarName(name, output_slot, true) + " = "
            + string_format(expression, shading0.transparency_factor, shading1.transparency_factor) + ";\n";
        result->transparency_factor = _toGLSLVarName(name, output_slot, true);
        result->has_transparency_factor = true;
    }
}

Uptr<ShadeTreeNode> createShadeTreeNode(const std::string& node_type)
{
    if (node_type == "OUTPUT_MATERIAL")
        return std::make_unique<OutputNode>();
    else if (node_type == "BSDF_DIFFUSE")
        return std::make_unique<DiffuseBSDFNode>();
    else if (node_type == "BSDF_GLOSSY")
       return std::make_unique<GlossyBSDFNode>();
    else if (node_type == "BSDF_TRANSPARENT")
        return std::make_unique<TransparentBSDFNode>();
    else if (node_type == "MIX_SHADER")
        return std::make_unique<MixBSDFNode>();
    else if (node_type == "ADD_SHADER")
        return std::make_unique<AddBSDFNode>();
    else if (node_type == "TEX_IMAGE")
        return std::make_unique<TexImageNode>();
    else if (node_type == "NORMAL_MAP")
       return std::make_unique<NormalMapNode>();
    else if (node_type == "BUMP")
       return std::make_unique<BumpNode>();
    else if (node_type == "MATH")
       return std::make_unique<MathNode>();
    else if (node_type == "VECT_MATH")
       return std::make_unique<VectorMathNode>();
    else
    {
        //assert(false);
        return nullptr;
    }        
}

#define RETURN_IF_ALREADY_EVALUATED auto it = evaluation.evaluted_nodes.find(name);\
    if (it != evaluation.evaluted_nodes.end())\
    return it->second;


const NodeEvaluatedOutputs& OutputNode::evaluate(const ShadeTreeParams& params, ShadeTreeEvaluation & evaluation)
{
    RETURN_IF_ALREADY_EVALUATED

    Shading shading = _evaluateInputSlot<Shading>(params, "Surface", evaluation);
    std::string node_glsl_code = "shading_result = vec4(" + shading.additive + ", 1.0);\n";
    node_glsl_code += "shading_result_transp_factor = vec4(" + shading.transparency_factor + ", 1.0);\n";
    evaluation.is_transparent = shading.has_transparency_factor;

    NodeEvaluatedOutputs& result = evaluation.addNodeCode(name, node_glsl_code);
    result["Output"] = std::make_unique<Shading>(shading);
    return result;
}

const NodeEvaluatedOutputs& DiffuseBSDFNode::evaluate(const ShadeTreeParams& params, ShadeTreeEvaluation& evaluation)
{
    RETURN_IF_ALREADY_EVALUATED

    std::string color = _evaluateInputSlot<Color>(params, "Color", evaluation).expression;
    std::string normal = _evaluateInputSlot<Normal>(params, "Normal", evaluation).expression;
    std::string glsl_output_name = _toGLSLVarName(name, "BSDF");
    std::string node_glsl_code = "vec3 "+ glsl_output_name +" = evalDiffuseBSDF("+ color +", "+ normal +");\n";

    NodeEvaluatedOutputs& result = evaluation.addNodeCode(name, node_glsl_code);
    result["BSDF"] = std::make_unique<Shading>(glsl_output_name);
    return result;
}

const NodeEvaluatedOutputs& GlossyBSDFNode::evaluate(const ShadeTreeParams& params, ShadeTreeEvaluation& evaluation)
{
   RETURN_IF_ALREADY_EVALUATED

   std::string color = _evaluateInputSlot<Color>(params, "Color", evaluation).expression;
   std::string normal = _evaluateInputSlot<Normal>(params, "Normal", evaluation).expression;
   std::string glsl_output_name = _toGLSLVarName(name, "BSDF");
   std::string node_glsl_code = "vec3 " + glsl_output_name + " = evalGlossyBSDF(" + color + ", " + normal + ");\n";

   NodeEvaluatedOutputs& result = evaluation.addNodeCode(name, node_glsl_code);
   result["BSDF"] = std::make_unique<Shading>(glsl_output_name);
   return result;
}

const NodeEvaluatedOutputs& TransparentBSDFNode::evaluate(const ShadeTreeParams& params, ShadeTreeEvaluation& evaluation)
{
    RETURN_IF_ALREADY_EVALUATED

    std::string color = _evaluateInputSlot<Color>(params, "Color", evaluation).expression;
    std::string glsl_output_name = _toGLSLVarName(name, "BSDF", true);
    std::string node_glsl_code = "vec3 " + glsl_output_name + " = " + color + ";\n";
    Shading shading;
    shading.has_transparency_factor = true;
    shading.transparency_factor = glsl_output_name;

    NodeEvaluatedOutputs& result = evaluation.addNodeCode(name, node_glsl_code);
    result["BSDF"] = std::make_unique<Shading>(shading);
    return result;
}

const NodeEvaluatedOutputs& MixBSDFNode::evaluate(const ShadeTreeParams& params, ShadeTreeEvaluation& evaluation)
{
    RETURN_IF_ALREADY_EVALUATED

    Shading shading0 = _evaluateInputSlot<Shading>(params, "Shader", evaluation);
    Shading shading1 = _evaluateInputSlot<Shading>(params, "Shader_001", evaluation);
    std::string mix_factor = _evaluateInputSlot<Float>(params, "Fac", evaluation).expression;

    std::string node_glsl_code;
    Shading shading;
    _evaluateShading(shading0, shading1, "mix(%s, %s,"+ mix_factor +")", &shading, &node_glsl_code);
    
        
    NodeEvaluatedOutputs& result = evaluation.addNodeCode(name, node_glsl_code);
    result["Shader"] = std::make_unique<Shading>(shading);    
    return result;
}

const NodeEvaluatedOutputs& AddBSDFNode::evaluate(const ShadeTreeParams& params, ShadeTreeEvaluation& evaluation)
{
    RETURN_IF_ALREADY_EVALUATED

    Shading shading0 = _evaluateInputSlot<Shading>(params, "Shader", evaluation);
    Shading shading1 = _evaluateInputSlot<Shading>(params, "Shader_001", evaluation);

    std::string node_glsl_code;    
    Shading shading;
    _evaluateShading(shading0, shading1, "%s + %s", &shading, &node_glsl_code);
       
    NodeEvaluatedOutputs& result = evaluation.addNodeCode(name, node_glsl_code);
    result["Shader"] = std::make_unique<Shading>(shading);    
    return result;
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

const NodeEvaluatedOutputs& TexImageNode::evaluate(const ShadeTreeParams& params, ShadeTreeEvaluation& evaluation)
{
    RETURN_IF_ALREADY_EVALUATED

    evaluation.uv_needed = true;

    std::string glsl_color = _toGLSLVarName(name, "Color");
    std::string glsl_alpha = _toGLSLVarName(name, "Alpha");
    std::string glsl_texture = _toGLSLVarName(name, "Texture");
    std::string glsl_matrix = _toGLSLMatrix(texture_transform);
    
    std::string node_glsl_code;

    if (!compute_pixel_differentials)
    {
       node_glsl_code = "vec3 " + glsl_color + ";\n";
       node_glsl_code += "float " + glsl_alpha + ";\n";
       node_glsl_code += string_format("sampleTexture(%s, vec3(attr_uv, 1.0), %s, %s, %s)\n",
          glsl_texture, glsl_matrix, glsl_color, glsl_alpha);
    }
    else
    {
       node_glsl_code = "vec3 " + glsl_color + ";\n";
       node_glsl_code += "vec3 " + glsl_color+ _dx + ";\n";
       node_glsl_code += "vec3 " + glsl_color + _dy + ";\n";
       node_glsl_code += "float " + glsl_alpha + ";\n";
       node_glsl_code += "float " + glsl_alpha + _dx + ";\n";
       node_glsl_code += "float " + glsl_alpha + _dy + ";\n";
       node_glsl_code += string_format("sampleTextureDifferentials(%s, vec3(attr_uv, 1.0), %s, %s, %s, %s, %s, %s, %s);\n",
                           glsl_texture, glsl_matrix, glsl_color, glsl_color+_dx, glsl_color + _dy, glsl_alpha, glsl_alpha+ _dx, glsl_alpha+_dy);
    }
    
    int bind_slot = int(evaluation.glsl_textures.size()) + params.texture_binding_slot_start;
    std::string sampler_definition = "layout(binding="+ std::to_string(bind_slot) +")uniform sampler2D " + _toGLSLVarName(name, "Texture") +";\n";
    evaluation.glsl_textures.push_back(std::make_pair(texture, sampler_definition));
        
    NodeEvaluatedOutputs& result = evaluation.addNodeCode(name, node_glsl_code);
    result["Color"] = std::make_unique<Color>(glsl_color);
    result["Alpha"] = std::make_unique<Float>(glsl_alpha);
    return result;
}

const NodeEvaluatedOutputs& NormalMapNode::evaluate(const ShadeTreeParams& params, ShadeTreeEvaluation& evaluation)
{
   RETURN_IF_ALREADY_EVALUATED
   evaluation.normal_mapping_needed = true;
   
   std::string glsl_normal = _toGLSLVarName(name, "Normal");
   std::string glsl_color = _evaluateInputSlot<Color>(params, "Color", evaluation).expression;   
   std::string glsl_strength = _evaluateInputSlot<Float>(params, "Strength", evaluation).expression;
   std::string node_glsl_code = string_format("vec3 %s = evalNormalMap(%s, %s);\n", glsl_normal, glsl_color, glsl_strength);

   NodeEvaluatedOutputs& result = evaluation.addNodeCode(name, node_glsl_code);
   result["Normal"] = std::make_unique<Normal>(glsl_normal);
   return result;
}

const NodeEvaluatedOutputs& BumpNode::evaluate(const ShadeTreeParams& params, ShadeTreeEvaluation& evaluation)
{
   RETURN_IF_ALREADY_EVALUATED

   std::string glsl_invert = invert ? "true" : "false";   
   std::string glsl_distance = _evaluateInputSlot<Float>(params, "Distance", evaluation).expression;
   std::string glsl_strength = _evaluateInputSlot<Float>(params, "Strength", evaluation).expression;
   std::string glsl_height, glsl_height_dx, glsl_height_dy;
   _evaluateInputSlot<Float>(params, "Height", evaluation, glsl_height, glsl_height_dx, glsl_height_dy);
   std::string glsl_normal = _evaluateInputSlot<Normal>(params, "Normal", evaluation).expression;

   std::string glsl_output_normal = _toGLSLVarName(name, "Normal");
   std::string node_glsl_code = string_format("vec3 %s = evalBump(%s, %s, %s, %s, vec2(%s, %s), %s);\n",
                                 glsl_output_normal, glsl_invert, glsl_distance, glsl_strength, glsl_height, glsl_height_dx, glsl_height_dy, glsl_normal);

   NodeEvaluatedOutputs& result = evaluation.addNodeCode(name, node_glsl_code);
   result["Normal"] = std::make_unique<Normal>(glsl_output_normal);
   return result;
}

static std::string _createMathNodeCode(const std::string& operation,
                                       bool clamp,
                                       const std::string& type,
                                       const std::string& output,
                                       const std::string& value1,
                                       const std::string& value2)
{
   std::string node_glsl_code = type;
   if (operation == "ADD")
      node_glsl_code += string_format(" %s = %s + %s;\n", output, value1, value2);
   else if (operation == "SUBSTRACT")
      node_glsl_code += string_format(" %s = %s - %s;\n", output, value1, value2);
   else if (operation == "MULTIPLY")
      node_glsl_code += string_format(" %s = %s * %s;\n", output, value1, value2);
   else if (operation == "DIVIDE")
      node_glsl_code += string_format(" %s = %s / %s;\n", output, value1, value2);
   else if (operation == "SINE")
      node_glsl_code += string_format(" %s = sin(%s);\n", output, value1);
   else if (operation == "COSINE")
      node_glsl_code += string_format(" %s = cos(%s);\n", output, value1);
   else if (operation == "TANGENT")
      node_glsl_code += string_format(" %s = tan(%s);\n", output, value1);
   else if (operation == "ARCSINE")
      node_glsl_code += string_format(" %s = asin(%s);\n", output, value1);
   else if (operation == "ARCCOSINE")
      node_glsl_code += string_format(" %s = acos(%s);\n", output, value1);
   else if (operation == "ARCTANGENT")
      node_glsl_code += string_format(" %s = atan(%s);\n", output, value1);
   else if (operation == "POWER")
      node_glsl_code += string_format(" %s = pow(%s, %s);\n", output, value1, value2);
   else if (operation == "LOGARITHM")
      node_glsl_code += string_format(" %s = log(%s, %s);\n", output, value1, value2);
   else if (operation == "MINIMUM")
      node_glsl_code += string_format(" %s = min(%s, %s);\n", output, value1, value2);
   else if (operation == "MAXIMUM")
      node_glsl_code += string_format(" %s = max(%s, %s);\n", output, value1, value2);
   else if (operation == "ROUND")
      node_glsl_code += string_format(" %s = floor(%s+0.5);\n", output, value1);
   else if (operation == "LESS_THAN")
      node_glsl_code += string_format(" %s = %s < %s ;\n", output, value1, value2);
   else if (operation == "GREATER_THAN")
      node_glsl_code += string_format(" %s = %s > %s ;\n", output, value1, value2);
   else if (operation == "MODULO")
      node_glsl_code += string_format(" %s = %s %% %s ;\n", output, value1, value2);
   else if (operation == "ABSOLUTE")
      node_glsl_code += string_format(" %s = abs(%s);\n", output, value1);

   if (clamp)
      node_glsl_code += string_format("%s = clamp(%s, 0.0, 1.0);\n", output, output);

   return node_glsl_code;
}


const NodeEvaluatedOutputs& MathNode::evaluate(const ShadeTreeParams& params, ShadeTreeEvaluation& evaluation)
{
   RETURN_IF_ALREADY_EVALUATED
   
   std::string output = _toGLSLVarName(name, "Value");
   std::string node_glsl_code;
   if (!compute_pixel_differentials)
   {
      std::string value1 = _evaluateInputSlot<Float>(params, "Value", evaluation).expression;
      std::string value2 = _evaluateInputSlot<Float>(params, "Value_001", evaluation).expression;      
      node_glsl_code = _createMathNodeCode(operation, clamp, "float", output, value1, value2);
   }  
   else
   {
      std::string value1, value1_dx, value1_dy;
      std::string value2, value2_dx, value2_dy;
      _evaluateInputSlot<Float>(params, "Value", evaluation, value1, value1_dx, value1_dy);      
      _evaluateInputSlot<Float>(params, "Value_001", evaluation, value2, value2_dx, value2_dy);      
      node_glsl_code = _createMathNodeCode(operation, clamp, "float", output, value1, value2);
      node_glsl_code += _createMathNodeCode(operation, clamp, "float", output+_dx, value1_dx, value2_dx);
      node_glsl_code += _createMathNodeCode(operation, clamp, "float", output+_dy, value1_dy, value2_dy);
   }

   NodeEvaluatedOutputs& result = evaluation.addNodeCode(name, node_glsl_code);
   result["Value"] = std::make_unique<Float>(output);
   return result;
}

const NodeEvaluatedOutputs& VectorMathNode::evaluate(const ShadeTreeParams& params, ShadeTreeEvaluation& evaluation)
{
   RETURN_IF_ALREADY_EVALUATED

   std::string value1 = _evaluateInputSlot<Vector>(params, "Value", evaluation).expression;
   std::string value2 = _evaluateInputSlot<Vector>(params, "Value_001", evaluation).expression;
   std::string output = _toGLSLVarName(name, "Value");

   //std::string node_glsl_code = _createMathNodeCode(operation, clamp, "vec3", output, value1, value2);
   std::string node_glsl_code;
   NodeEvaluatedOutputs& result = evaluation.addNodeCode(name, node_glsl_code);
   result["Value"] = std::make_unique<Vector>(output);
   return result;
}

}