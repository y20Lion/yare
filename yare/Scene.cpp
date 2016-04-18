#include "Scene.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include "GLBuffer.h"
#include <iostream>
static glm::mat4 _toMat4(const glm::mat4x3& matrix)
{
    glm::mat4 result;
    result[0] = glm::vec4(matrix[0], 0.0);
    result[1] = glm::vec4(matrix[1], 0.0);
    result[2] = glm::vec4(matrix[2], 0.0);
    result[3] = glm::vec4(matrix[3], 1.0);
    return result;
}

static glm::mat4x3 _toMat4x3(const glm::mat4& matrix)
{
    glm::mat4x3 result;
    result[0] = matrix[0].xyz;
    result[1] = matrix[1].xyz;
    result[2] = matrix[2].xyz;
    result[3] = matrix[3].xyz;
    return result;
}

void Scene::update()
{
    auto mat = glm::lookAt(camera.point_of_view.from, camera.point_of_view.to, camera.point_of_view.up);
    _matrix_view_world = glm::perspective(3.14f/2.0f, 1.0f, 0.5f, 1000.0f) * mat;
        /** glm::frustum(camera.frustum.left, camera.frustum.right,
                       camera.frustum.bottom, camera.frustum.top,
                       camera.frustum.near, camera.frustum.far);*/
            
    surfaces_render_data.resize(surfaces.size());
    for (int i = 0; i < surfaces_render_data.size(); ++i)
    {
        surfaces_render_data[i].matrix_view_local = _matrix_view_world * _toMat4(surfaces[i].matrix_world_local);
    }
}

glm::vec3 quad_vertices[6] = { glm::vec3(1.0, 1.0, -1.0), glm::vec3(-1.0, 1.0, -1.0), glm::vec3(-1.0, -1.0, -1.0),
                          glm::vec3(1.0, 1.0, -1.0), glm::vec3(-1.0, -1.0, -1.0), glm::vec3( 1.0f, -1.0, -1.0) };

Scene createBasicScene()
{
    auto positions_buffer = createVertexBuffer(sizeof(glm::vec3)*6);
    auto ptr = positions_buffer->map(GL_WRITE_ONLY);
    memcpy(ptr, quad_vertices, sizeof(glm::vec3)*6);
    positions_buffer->unmap();
    
    GLVertexSourceSptr quad_source = GLVertexSourceSptr(new GLVertexSource());
    quad_source->setVertexBuffer(*positions_buffer);
    quad_source->setVertexAttribute(1, 3, GL_FLOAT, 0, 0);
    quad_source->setVertexCount(6);
    
    SurfaceInstance surface_instance;
    Scene scene;

    surface_instance.matrix_world_local = glm::mat4x3(1.0);
    surface_instance.mesh = quad_source;
    scene.surfaces.push_back(surface_instance);

    surface_instance.matrix_world_local = _toMat4x3(glm::rotate(float(M_PI/2.0), glm::vec3(0.0f,1.0f,0.0f)));
    scene.surfaces.push_back(surface_instance);

    surface_instance.matrix_world_local = _toMat4x3(glm::rotate(-float(M_PI/2.0), glm::vec3(0.0f,1.0f,0.0f)));
    scene.surfaces.push_back(surface_instance);

    surface_instance.matrix_world_local = _toMat4x3(glm::rotate(float(M_PI), glm::vec3(0.0f,1.0f,0.0f)));
    scene.surfaces.push_back(surface_instance);

    return scene;
}