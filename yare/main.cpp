#include "stdafx.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <assert.h>
#include <iostream>
#include <atomic>

#include "GLBuffer.h"
#include "RenderEngine.h"
#include "CameraManipulator.h"
#include "Importer3DY.h"
#include "GLTexture.h"
#include "Scene.h"
#include "Barrier.h"
#include "ImageSize.h"
#include "GLDevice.h"

using namespace yare;

void __stdcall printGLDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity,
   GLsizei length, const GLchar* message, const void* userparam)
{
   if (severity == GL_DEBUG_SEVERITY_HIGH)
   {
      std::cout << message << std::endl;
      assert(false);
   }
}

bool initGlewWithDummyWindow()
{
   glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
   GLFWwindow* window = glfwCreateWindow(640, 480, "DummyWindow", NULL, NULL);
   glfwMakeContextCurrent(window);

   auto glew_result = glewInit();

   //glfwWindowHint(GLFW_VISIBLE, GL_TRUE);
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

static Barrier barrier(2);
std::atomic<bool> exit_program = false;
void engineUpdateThread(RenderEngine* render_engine);


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

   glfwWindowHint(GLFW_SAMPLES, 1); // TODO yvain remove
   glfwWindowHint(GLFW_DEPTH_BITS, 0);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
   glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
   glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
   glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
   GLFWwindow* window = glfwCreateWindow(1500, 1000, "MyWindow", NULL, NULL);

   glfwMakeContextCurrent(window);
   glfwSwapInterval(1);

   glEnable(GL_DEBUG_OUTPUT);
   glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
   glDebugMessageCallback(&printGLDebugMessage, nullptr);
   glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);


   GLDevice::bindDefaultDepthStencilState();
   GLDevice::bindDefaultColorBlendState();
   GLDevice::bindDefaultRasterizationState();
   glEnable(GL_CULL_FACE);

   RenderEngine render_engine(ImageSize(1500, 1000));
   //char* file = "D:\\BlenderTests\\Sintel_Lite_Cycles_V2.3dy";
   //char* file = "D:\\BlenderTests\\test_lighting.3dy";
   char* file = "D:\\BlenderTests\\town.3dy";
   import3DY(file, render_engine, render_engine.scene());
   render_engine.offlinePrepareScene();

   CameraManipulator camera_manipulator(&render_engine.scene()->camera.point_of_view);
   
   //render_engine.updateScene(render_engine.scene()->render_data[1]);   
   //std::thread engine_update_thread(engineUpdateThread, &render_engine);

   glfwShowWindow(window);
   int update_index = 0;
   int render_index = 1;
   while (!exit_program)
   {
      glfwPollEvents();
      handleInputs(window, &camera_manipulator);   
      
      render_engine.updateScene(render_engine.scene()->render_data[render_index]);
      render_engine.renderScene(render_engine.scene()->render_data[render_index]);
      exit_program = bool(glfwWindowShouldClose(window));
      //barrier.wait();
      std::swap(update_index, render_index);

      glfwSwapBuffers(window);      
   }

   glfwTerminate();
   //engine_update_thread.join();
   return 0;
}

void engineUpdateThread(RenderEngine* render_engine)
{
   int update_index = 0;
   int render_index = 1;
   while (!exit_program)
   {
      render_engine->updateScene(render_engine->scene()->render_data[update_index]);
      barrier.wait();
      std::swap(update_index, render_index);
   }
}

