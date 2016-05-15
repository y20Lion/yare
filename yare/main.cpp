#include "stdafx.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <assert.h>
#include <iostream>

#include "GLBuffer.h"
#include "RenderEngine.h"
#include "CameraManipulator.h"
#include "Importer3DY.h"
#include "GLTexture.h"

using namespace yare;

void __stdcall printGLDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity,
   GLsizei length, const GLchar* message, const void* userparam)
{
   std::cout << message << std::endl;
   assert(severity != GL_DEBUG_SEVERITY_HIGH);
}

bool initGlewWithDummyWindow()
{
   glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
   GLFWwindow* window = glfwCreateWindow(640, 480, "DummyWindow", NULL, NULL);
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
   glfwWindowHint(GLFW_DEPTH_BITS, 24);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
   glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
   glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
   GLFWwindow* window = glfwCreateWindow(1024, 768, "MyWindow", NULL, NULL);

   glfwMakeContextCurrent(window);
   glfwSwapInterval(1);

   glEnable(GL_DEBUG_OUTPUT);
   glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
   glDebugMessageCallback(&printGLDebugMessage, nullptr);

   glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glDisable(GL_DEPTH_TEST);

   RenderEngine render_engine;
   import3DY("D:\\BlenderTests\\testNodeGraph.3dy", render_engine, render_engine.scene());

   CameraManipulator camera_manipulator(&render_engine.scene()->camera.point_of_view);
   while (!glfwWindowShouldClose(window))
   {
      handleInputs(window, &camera_manipulator);
      render_engine.update();
      render_engine.render();

      glfwSwapBuffers(window);
      glfwPollEvents();
   }

   glfwTerminate();
   return 0;
}

