#include "ShadeTreeMaterial.h"

#include <algorithm>

#include "stl_helpers.h"
#include "GLProgram.h"
#include "GLTexture.h"
#include "ShadeTreeNode.h"

namespace yare {

ShadeTreeMaterial::ShadeTreeMaterial()
 : _first_texture_binding(5)
{

}

ShadeTreeMaterial::~ShadeTreeMaterial()
{

}

const char* _material_vert_shader =
" layout(location=1) in vec3 position; \n"
" layout(location=2) in vec3 normal; \n"
" layout(location=3) in vec2 uv; \n"
" out vec3 attr_normal; \n"
" #ifdef USE_UV \n"
" out vec2 attr_uv; \n"
" #endif \n"
" layout(std140, binding=3) uniform MatUniform \n"
" { \n"
"   mat4 matrix_view_local; \n"
"   mat3 normal_matrix_world_local; \n"
" }; \n"

" void main() \n"
" { \n"
"   gl_Position =  matrix_view_local * vec4(position, 1.0); \n"
"   attr_normal =  normal_matrix_world_local*normal; \n"
" #ifdef USE_UV \n"
"   attr_uv =  uv; \n"
" #endif \n"
" }\n";

static const char* _material_frag_shader_begin =
"vec3 light_direction = vec3(0.0, 0.2, 1.0);\n "
"vec3 light_color = vec3(1.0, 1.0, 1.0);\n "
"in vec3 attr_normal; \n"
" #ifdef USE_UV \n"
"in vec2 attr_uv; \n"
" #endif \n";
static const char* _material_frag_shader_middle =
"\n "
"vec3 evalDiffuseBSDF(vec3 color, vec3 normal)\n "
"{\n "
"	return max(dot(normalize(normal), light_direction),0.02) * color * light_color;\n " // boohoo an ambient
"}\n "
"\n "
"void sampleTexture(sampler2D tex, vec3 uvw, mat3x2 transform, out vec3 color, out float alpha)\n "
"{\n "
"	vec4 tex_sample=texture(tex, transform*vec3(uvw.xy,1.0));\n " 
"	color=tex_sample.rgb;\n "
"	alpha=tex_sample.a;\n "
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

    ShadeTreeParams eval_params;
    eval_params.tree_nodes = &tree_nodes;
    eval_params.texture_binding_slot_start = _first_texture_binding;
    
    ShadeTreeEvaluation evaluation;
    output_node->evaluate(eval_params, "shading_result", evaluation);

    std::string fragment_shader = "#version 450 \n";
    if (evaluation.uv_needed)
        fragment_shader += "#define USE_UV \n";
    fragment_shader += _material_frag_shader_begin;

    _used_textures.clear();
    for (const auto& glsl_texture : evaluation.glsl_textures)
    {
        _used_textures.push_back(glsl_texture.first->id());
        fragment_shader.append(glsl_texture.second);
    }    

    fragment_shader.append(_material_frag_shader_middle);

    for (const std::string& glsl : evaluation.glsl_code)
        fragment_shader.append(glsl);

    fragment_shader.append(_material_frag_shader_end);

    std::string vertex_shader = "#version 450 \n";
    if (evaluation.uv_needed)
        vertex_shader += "#define USE_UV \n";
    vertex_shader += _material_vert_shader;

    _program = createProgram(vertex_shader, fragment_shader);
}

void ShadeTreeMaterial::bindTextures()
{
    glBindTextures(_first_texture_binding, (GLuint)_used_textures.size(), _used_textures.data());
}

}