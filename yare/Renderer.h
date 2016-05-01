#pragma once

#include "tools.h"
#include "Scene.h"
#include "GLProgram.h"

namespace yare {

class Renderer
{
public:
    Renderer();
    void render();
    Scene* scene() { return &_scene; }

    glm::vec3 pickPosition(int x, int y);

private:
    DISALLOW_COPY_AND_ASSIGN(Renderer)
    Scene _scene;
    Uptr<GLProgram> _draw_mesh;
    Uptr<GLBuffer> _uniforms_buffer;
};

}