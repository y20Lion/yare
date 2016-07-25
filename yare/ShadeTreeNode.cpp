#include "ShadeTreeNode.h"

#include <algorithm>
#include <sstream>
#include <assert.h>
#include "stl_helpers.h"
#include "GLTexture.h"
#include "GLSampler.h"
#include "RenderResources.h"

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

void ShadeTreeEvaluation::addTexture(int binding_slot_start, const std::string& texture_type, const std::string& texture_name, const GLTexture* texture, const GLSampler* sampler)
{
   int bind_slot = int(glsl_textures.size()) + binding_slot_start;
   std::string sampler_definition = "layout(binding=" + std::to_string(bind_slot) + ")uniform "+ texture_type+ " " + texture_name + ";\n";
   glsl_textures.push_back(std::make_tuple(texture, sampler, sampler_definition));
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
   const std::string& output_slot = "Shader";
    
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
    else if (node_type == "EMISSION")
       return std::make_unique<EmissionBSDFNode>();
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
    else if (node_type == "FRESNEL")
       return std::make_unique<FresnelNode>();
    else if (node_type == "MIX_RGB")
       return std::make_unique<MixRGBNode>();
    else if (node_type == "VALTORGB")
       return std::make_unique<ColorRampNode>();
    else if (node_type == "CURVE_RGB")
       return std::make_unique<CurveRgbNode>();
    else if (node_type == "RGBTOBW")
       return std::make_unique<RgbToBWNode>();
    else if (node_type == "LAYER_WEIGHT")
       return std::make_unique<LayerWeightNode>();
    else if (node_type == "TEX_COORD")
       return std::make_unique<TextureCoordinateNode>();
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
   std::string roughness = _evaluateInputSlot<Float>(params, "Roughness", evaluation).expression;
   std::string glsl_output_name = _toGLSLVarName(name, "BSDF");
   std::string node_glsl_code = "vec3 " + glsl_output_name + " = evalGlossyBSDF(" + color + ", " + normal + ", "+ roughness  +");\n";

   NodeEvaluatedOutputs& result = evaluation.addNodeCode(name, node_glsl_code);
   result["BSDF"] = std::make_unique<Shading>(glsl_output_name);
   return result;
}

const NodeEvaluatedOutputs& EmissionBSDFNode::evaluate(const ShadeTreeParams& params, ShadeTreeEvaluation& evaluation)
{
   RETURN_IF_ALREADY_EVALUATED

   std::string color = _evaluateInputSlot<Color>(params, "Color", evaluation).expression;
   std::string strength = _evaluateInputSlot<Float>(params, "Strength", evaluation).expression;
   std::string glsl_output_name = _toGLSLVarName(name, "Emission");
   std::string node_glsl_code = "vec3 " + glsl_output_name + " = evalEmissionBSDF(" + color + ", " + strength + ");\n";

   NodeEvaluatedOutputs& result = evaluation.addNodeCode(name, node_glsl_code);
   result["Emission"] = std::make_unique<Shading>(glsl_output_name);
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
       node_glsl_code += string_format("sampleTexture(%s, vec3(attr_uv, 1.0), %s, %s, %s);\n",
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

    evaluation.addTexture(params.texture_binding_slot_start, "sampler2D", glsl_texture, texture.get(), params.samplers->mipmap_repeat.get());
        
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
   
   //TODO add math node code
   std::string node_glsl_code;
   NodeEvaluatedOutputs& result = evaluation.addNodeCode(name, node_glsl_code);
   result["Value"] = std::make_unique<Vector>(output);
   return result;
}

const NodeEvaluatedOutputs& FresnelNode::evaluate(const ShadeTreeParams& params, ShadeTreeEvaluation& evaluation)
{
   RETURN_IF_ALREADY_EVALUATED

   std::string ior = _evaluateInputSlot<Float>(params, "IOR", evaluation).expression;
   std::string value2 = _evaluateInputSlot<Normal>(params, "Normal", evaluation).expression;
   std::string output = _toGLSLVarName(name, "Fac");

   std::string node_glsl_code = string_format("float %s = evalFresnel(%s, %s);\n", output, ior, value2);
   NodeEvaluatedOutputs& result = evaluation.addNodeCode(name, node_glsl_code);
   result["Fac"] = std::make_unique<Float>(output);
   return result;
}

const NodeEvaluatedOutputs& MixRGBNode::evaluate(const ShadeTreeParams& params, ShadeTreeEvaluation& evaluation)
{
   RETURN_IF_ALREADY_EVALUATED

   std::string fac = _evaluateInputSlot<Float>(params, "Fac", evaluation).expression;
   std::string color1 = _evaluateInputSlot<Color>(params, "Color1", evaluation).expression;
   std::string color2 = _evaluateInputSlot<Color>(params, "Color2", evaluation).expression;
   std::string output = _toGLSLVarName(name, "Color");

   std::string node_glsl_code;
   if (operation == "MIX")
      node_glsl_code += string_format("vec3 %s = node_mix_blend(clamp(%s, 0.0, 1.0), %s, %s);\n", output, fac, color1, color2);
   else if (operation == "ADD")
      node_glsl_code += string_format("vec3 %s = node_mix_add(clamp(%s, 0.0, 1.0), %s, %s);\n", output, fac, color1, color2);
   else if (operation == "MULTIPLY")
      node_glsl_code += string_format("vec3 %s = node_mix_mul(clamp(%s, 0.0, 1.0), %s, %s);\n", output, fac, color1, color2);
   else if (operation == "SCREEN")
      node_glsl_code += string_format("vec3 %s = node_mix_screen(clamp(%s, 0.0, 1.0), %s, %s);\n", output, fac, color1, color2);
   else if (operation == "OVERLAY")
      node_glsl_code += string_format("vec3 %s = node_mix_overlay(clamp(%s, 0.0, 1.0), %s, %s);\n", output, fac, color1, color2);
   else if (operation == "SUBTRACT")
      node_glsl_code += string_format("vec3 %s = node_mix_sub(clamp(%s, 0.0, 1.0), %s, %s);\n", output, fac, color1, color2);
   else if (operation == "DIVIDE")
      node_glsl_code += string_format("vec3 %s = node_mix_div(clamp(%s, 0.0, 1.0), %s, %s);\n", output, fac, color1, color2);
   else if (operation == "DIFFERENCE")
      node_glsl_code += string_format("vec3 %s = node_mix_diff(clamp(%s, 0.0, 1.0), %s, %s);\n", output, fac, color1, color2);
   else if (operation == "DARKEN")
      node_glsl_code += string_format("vec3 %s = node_mix_dark(clamp(%s, 0.0, 1.0), %s, %s);\n", output, fac, color1, color2);
   else if (operation == "LIGHTEN")
      node_glsl_code += string_format("vec3 %s = node_mix_light(clamp(%s, 0.0, 1.0), %s, %s);\n", output, fac, color1, color2);
   else if (operation == "DODGE")
      node_glsl_code += string_format("vec3 %s = node_mix_dodge(clamp(%s, 0.0, 1.0), %s, %s);\n", output, fac, color1, color2);
   else if (operation == "BURN")
      node_glsl_code += string_format("vec3 %s = node_mix_burn(clamp(%s, 0.0, 1.0), %s, %s);\n", output, fac, color1, color2);
   else if (operation == "HUE")
      node_glsl_code += string_format("vec3 %s = node_mix_hue(clamp(%s, 0.0, 1.0), %s, %s);\n", output, fac, color1, color2);
   else if (operation == "SATURATION")
      node_glsl_code += string_format("vec3 %s = node_mix_sat(clamp(%s, 0.0, 1.0), %s, %s);\n", output, fac, color1, color2);
   else if (operation == "VALUE")
      node_glsl_code += string_format("vec3 %s = node_mix_val(clamp(%s, 0.0, 1.0), %s, %s);\n", output, fac, color1, color2);
   else if (operation == "COLOR")
      node_glsl_code += string_format("vec3 %s = node_mix_color(clamp(%s, 0.0, 1.0), %s, %s);\n", output, fac, color1, color2);
   else if (operation == "SOFT_LIGHT")
      node_glsl_code += string_format("vec3 %s = node_mix_soft(clamp(%s, 0.0, 1.0), %s, %s);\n", output, fac, color1, color2);
   else if (operation == "LINEAR_LIGHT")
      node_glsl_code += string_format("vec3 %s = node_mix_linear(clamp(%s, 0.0, 1.0), %s, %s);\n", output, fac, color1, color2);

   if (clamp)
      node_glsl_code += string_format("%s = clamp(%s, vec3(0.0), vec3(1.0));\n", output);

   NodeEvaluatedOutputs& result = evaluation.addNodeCode(name, node_glsl_code);
   result["Color"] = std::make_unique<Color>(output);
   return result;
}

const NodeEvaluatedOutputs& ColorRampNode::evaluate(const ShadeTreeParams& params, ShadeTreeEvaluation& evaluation)
{
   RETURN_IF_ALREADY_EVALUATED

   std::string glsl_fac = _evaluateInputSlot<Float>(params, "Fac", evaluation).expression;
   std::string glsl_ramp = _toGLSLVarName(name, "RampTexture");
   std::string glsl_color = _toGLSLVarName(name, "Color");
   std::string glsl_alpha = _toGLSLVarName(name, "Alpha");
   
   evaluation.addTexture(params.texture_binding_slot_start, "sampler1D", glsl_ramp, ramp_texture.get(), params.samplers->mipmap_clampToEdge.get());
   std::string node_glsl_code = string_format("vec3 %s = texture(%s, %s).rgb;\n", glsl_color, glsl_ramp, glsl_fac);

   NodeEvaluatedOutputs& result = evaluation.addNodeCode(name, node_glsl_code);
   result["Color"] = std::make_unique<Color>(glsl_color);
   result["Alpha"] = std::make_unique<Float>(glsl_alpha);
   return result;
}

const NodeEvaluatedOutputs& CurveRgbNode::evaluate(const ShadeTreeParams& params, ShadeTreeEvaluation& evaluation)
{
   RETURN_IF_ALREADY_EVALUATED

   std::string glsl_fac = _evaluateInputSlot<Float>(params, "Fac", evaluation).expression;
   std::string glsl_color = _evaluateInputSlot<Color>(params, "Color", evaluation).expression;
   std::string glsl_out_color = _toGLSLVarName(name, "Color");
   std::string glsl_red_curve = _toGLSLVarName(name, "RedCurve");
   std::string glsl_green_curve = _toGLSLVarName(name, "GreenCurve");
   std::string glsl_blue_curve = _toGLSLVarName(name, "BlueCurve");

   evaluation.addTexture(params.texture_binding_slot_start, "sampler1D", glsl_red_curve, red_curve.get(), params.samplers->bilinear_clampToEdge.get());
   evaluation.addTexture(params.texture_binding_slot_start, "sampler1D", glsl_green_curve, green_curve.get(), params.samplers->bilinear_clampToEdge.get());
   evaluation.addTexture(params.texture_binding_slot_start, "sampler1D", glsl_blue_curve, blue_curve.get(), params.samplers->bilinear_clampToEdge.get());
   std::string node_glsl_code = string_format("vec3 %s = evalCurveRgb(%s, %s, %s, %s, %s);\n",
                                              glsl_out_color, glsl_color, glsl_fac, glsl_red_curve, glsl_green_curve, glsl_blue_curve);

   NodeEvaluatedOutputs& result = evaluation.addNodeCode(name, node_glsl_code);
   result["Color"] = std::make_unique<Color>(glsl_out_color);
   return result;
}

const NodeEvaluatedOutputs& RgbToBWNode::evaluate(const ShadeTreeParams& params, ShadeTreeEvaluation& evaluation)
{
   RETURN_IF_ALREADY_EVALUATED

   std::string glsl_color = _evaluateInputSlot<Color>(params, "Color", evaluation).expression;
   std::string glsl_out_val = _toGLSLVarName(name, "Val");

   std::string node_glsl_code = string_format("float %s = linearRgbToGray(%s);\n", glsl_out_val, glsl_color);

   NodeEvaluatedOutputs& result = evaluation.addNodeCode(name, node_glsl_code);
   result["Val"] = std::make_unique<Float>(glsl_out_val);
   return result;
}

const NodeEvaluatedOutputs& LayerWeightNode::evaluate(const ShadeTreeParams& params, ShadeTreeEvaluation& evaluation)
{
   RETURN_IF_ALREADY_EVALUATED

   std::string glsl_normal = _evaluateInputSlot<Normal>(params, "Normal", evaluation).expression;
   std::string glsl_blend = _evaluateInputSlot<Float>(params, "Blend", evaluation).expression;
   std::string glsl_out_fresnel = _toGLSLVarName(name, "Fresnel");
   std::string glsl_out_facing = _toGLSLVarName(name, "Facing");   

   std::string node_glsl_code;
   node_glsl_code += "float " + glsl_out_fresnel + ", "+ glsl_out_facing + ";\n";
   node_glsl_code += string_format("evalLayerWeight(%s, %s, %s, %s);\n", glsl_blend, glsl_normal, glsl_out_fresnel, glsl_out_facing);

   NodeEvaluatedOutputs& result = evaluation.addNodeCode(name, node_glsl_code);
   result["Fresnel"] = std::make_unique<Float>(glsl_out_fresnel);
   result["Facing"] = std::make_unique<Float>(glsl_out_facing);
   return result;
}

const NodeEvaluatedOutputs& TextureCoordinateNode::evaluate(const ShadeTreeParams& params, ShadeTreeEvaluation& evaluation)
{
   RETURN_IF_ALREADY_EVALUATED

   std::string glsl_uv = _toGLSLVarName(name, "UV");

   std::string node_glsl_code = "vec3 " + glsl_uv + "= vec3(attr_uv, 0.0);\n";
   
   NodeEvaluatedOutputs& result = evaluation.addNodeCode(name, node_glsl_code);
   result["UV"] = std::make_unique<Vector>(glsl_uv);   
   return result;
}

}