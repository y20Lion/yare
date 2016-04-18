#pragma once

#include "tools.h"
#include "Scene.h"
#include "GLProgram.h"

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
    GLProgramUptr _draw_mesh;
};