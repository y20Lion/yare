#pragma once

#include <GL/glew.h>
#include <glm/vec4.hpp>
#include "tools.h"

namespace yare {

using glm::vec4;

struct GLSamplerDesc
{
   GLSamplerDesc();
   
   GLenum min_filter;
   GLenum mag_filter;   
   int anisotropy;
   GLenum wrap_mode_u;
   GLenum wrap_mode_v;
   GLenum wrap_mode_w;
   vec4 border_color;
};

class GLSampler
{
public:
   GLSampler(const GLSamplerDesc& desc);
   ~GLSampler();
   GLuint id() const { return _sampler_id; }

private:
   GLuint _sampler_id;
   DISALLOW_COPY_AND_ASSIGN(GLSampler)
};

Uptr<GLSampler> createSampler(const GLSamplerDesc& desc);

}