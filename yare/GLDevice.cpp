#include "GLDevice.h"

#include <GL/glew.h>

#include "GLProgram.h"
#include "GLProgramResources.h"
#include "GLVertexSource.h"




namespace yare { namespace GLDevice {

static const GLVertexSource* _current_vertex_source = nullptr;

void setCurrentProgram(const GLProgram& program)
{
    glUseProgram(program.id());
}

void setCurrentProgramResources(const GLProgramResources& program_resources)
{
    program_resources.bindResources();
}

void setCurrentVertexSource(const GLVertexSource& vertex_source)
{
    glBindVertexArray(vertex_source.id());
    _current_vertex_source = &vertex_source;
}

void draw(int vertex_start, int vertex_count)
{    
    glDrawArrays(GL_TRIANGLES, vertex_start, vertex_count);
}

}}
