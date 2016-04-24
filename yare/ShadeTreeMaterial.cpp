#include "ShadeTreeMaterial.h"

#include <algorithm>

#include "stl_helpers.h"
#include "GLProgram.h"
#include "ShadeTreeNode.h"

ShadeTreeMaterial::ShadeTreeMaterial()
{

}

ShadeTreeMaterial::~ShadeTreeMaterial()
{

}

const char* _material_vert_shader =
" #version 450 \n"
" layout(location=3) uniform mat4 t_view_local; \n"
" layout(location=1) in vec3 position; \n"
" layout(location=2) in vec3 normal; \n"
" out vec3 attr_normal; \n"

" void main() \n"
" { \n"
"   gl_Position =  t_view_local * vec4(position, 1.0); \n"
"   attr_normal =  normal; \n"
" }\n";

static const char* _material_frag_shader_begin =
"#version 450 \n"
"vec3 light_direction = vec3(0.0, 0.0, 1.0);\n "
"vec3 light_color = vec3(1.0, 1.0, 1.0);\n "
"in vec3 attr_normal; \n"
"\n "
"vec3 evalDiffuseBSDF(vec3 color, vec3 normal)\n "
"{\n "
"	return dot(normal, light_direction) * color * light_color;\n "
"}\n "
"\n "
" void main() \n"
" { \n";
static const char* _material_frag_shader_end =
"\n "
"   gl_FragColor =  vec4(shading_result, 1.0); \n"
" }\n";


void ShadeTreeMaterial::compile()
{
    auto is_output_node = [](const auto& name_node_pair)
    {
        return name_node_pair.second->type == "OUTPUT_MATERIAL";
    };
    auto node_it = std::find_if(RANGE(tree_nodes), is_output_node);
    ShadeTreeNode* output_node = node_it->second.get();

    ShadeTreeEvaluation evaluation;
    output_node->evaluate(tree_nodes, "shading_result", evaluation);

    std::string fragment_shader = _material_frag_shader_begin;
    for (const std::string& glsl : evaluation.glsl_code)
        fragment_shader.append(glsl);
    fragment_shader.append(_material_frag_shader_end);

    _program = createProgram(_material_vert_shader, fragment_shader);
}