#include "GLSampler.h"

#include <glm/gtc/type_ptr.hpp>

namespace yare {

GLSamplerDesc::GLSamplerDesc()
: mag_filter(GL_NEAREST)
, min_filter(GL_NEAREST)
, anisotropy(1)
, wrap_mode_u(GL_CLAMP_TO_EDGE)
, wrap_mode_v(GL_CLAMP_TO_EDGE)
, wrap_mode_w(GL_CLAMP_TO_EDGE)
, border_color(0.0)
{}

GLSampler::GLSampler(const GLSamplerDesc& desc)
{
   glGenSamplers(1, &_sampler_id);
   glSamplerParameteri(_sampler_id, GL_TEXTURE_MAG_FILTER, desc.mag_filter);
   glSamplerParameteri(_sampler_id, GL_TEXTURE_MIN_FILTER, desc.min_filter);
   glSamplerParameteri(_sampler_id, GL_TEXTURE_MAX_ANISOTROPY_EXT, desc.anisotropy);
   glSamplerParameteri(_sampler_id, GL_TEXTURE_WRAP_S, desc.wrap_mode_u);
   glSamplerParameteri(_sampler_id, GL_TEXTURE_WRAP_T, desc.wrap_mode_v);
   glSamplerParameteri(_sampler_id, GL_TEXTURE_WRAP_R, desc.wrap_mode_w);
   glSamplerParameterfv(_sampler_id, GL_TEXTURE_BORDER_COLOR, glm::value_ptr(desc.border_color));
}
GLSampler::~GLSampler()
{
   glDeleteSamplers(1, &_sampler_id);
}

Uptr<GLSampler> createSampler(const GLSamplerDesc& desc)
{
   return std::make_unique<GLSampler>(desc);
}

}