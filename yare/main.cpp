#include "stdafx.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <assert.h>
#include <iostream>

#include "GLProgram.h"
#include "GLProgramResources.h"
#include "GLBuffer.h"
#include "GLDevice.h"
#include "GLTexture.h"
#include "GLVertexSource.h"
#include "Renderer.h"
#include "CameraManipulator.h"
#include "Importer3DY.h"

void __stdcall printGLDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity,
                                   GLsizei length, const GLchar* message, const void* userparam)
{
    std::cout << message << std::endl;
    assert(severity == GL_DEBUG_SEVERITY_NOTIFICATION);
}

bool initGlewWithDummyWindow()
{
    glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
    GLFWwindow* window = glfwCreateWindow(640, 480, "MyWindow", NULL, NULL);
    glfwMakeContextCurrent(window);

    auto glew_result = glewInit();
    
    glfwWindowHint(GLFW_VISIBLE, GL_TRUE);
    glfwDestroyWindow(window);

    return glew_result == GLEW_OK;
}

/////////////////////////////// test //////////////////////

void handleInputs(GLFWwindow* window, CameraManipulator* camera_manipulator)
{
    static int previous_state = GLFW_RELEASE;

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    
    int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);

    if (state == GLFW_PRESS && state != previous_state)
    {
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
            camera_manipulator->motionStart(CameraAction::Pan, xpos, ypos);
        else if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            camera_manipulator->motionStart(CameraAction::Zoom, xpos, ypos);
        else
            camera_manipulator->motionStart(CameraAction::Orbit, xpos, ypos);
    }

    if (state == GLFW_RELEASE && state != previous_state)
        camera_manipulator->motionEnd();

    camera_manipulator->mouseMotion(xpos, ypos);
    previous_state = state;
}

int main()
{
    if (!glfwInit())
    {
        fprintf(stderr, "glfw: failed to initialize\n");
        return -1;
    }

    if (!initGlewWithDummyWindow())
    {
        fprintf(stderr, "glew: failed to initialize\n");
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);   
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    GLFWwindow* window = glfwCreateWindow(640, 480, "MyWindow", NULL, NULL);
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(&printGLDebugMessage, nullptr);
    
    const char* vertex_source = 
        " #version 450 \n"
        " layout(location=1) in vec3 position; \n"
        " out vec2 uv; \n"
        " void main() \n"
        " { \n"
        "   gl_Position = vec4(position, 1.0); \n"
        "   uv = position.xy; \n"
        " }\n";
    const char* fragment_source = 
        " #version 450 \n"
        " in vec2 uv; \n"
        //" layout(binding = 3) uniform sampler2D checkerboard; \n"
        " void main() \n"
        " { \n"
        "   gl_FragColor = vec4(1.0, 1.0, 0.0, 1.0);//vec4(texture(checkerboard, uv).rgb, 1.0); \n"
        " }\n";

    import("D:\\test.bin");

    /*auto program = createProgram(vertex_source, fragment_source);

    auto positions_buffer = createVertexBuffer(sizeof(Vec3)*6);
    auto ptr = positions_buffer->map(GL_WRITE_ONLY);
    memcpy(ptr, quad_vertices, sizeof(Vec3)*6);
    positions_buffer->unmap();*/

    /*GLTexture2DDesc tex_desc;
    tex_desc.width = 2;
    tex_desc.height = 2;
    tex_desc.levels = 1;
    tex_desc.internal_format = GL_RGBA8;
    GLTexture2D texture(tex_desc);
    texture.setData(0, GL_RGB, GL_FLOAT, pix_data);

    GLProgramResources prog_resources;
    prog_resources.setTexture(3, texture);*/
    
    /*GLVertexSource quad_source;
    quad_source.setVertexBuffer(*positions_buffer);
    quad_source.setVertexAttribute(1, 3, GL_FLOAT, 0, 0);*/
        
    
    Renderer renderer;
    CameraManipulator camera_manipulator(&renderer.scene()->camera.point_of_view);
    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        renderer.render();

        handleInputs(window, &camera_manipulator);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
	return 0;
}

