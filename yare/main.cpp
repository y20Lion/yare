#include "stdafx.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <assert.h>
#include <iostream>
#include <atomic>
#include <future>

#include "GLBuffer.h"
#include "RenderEngine.h"
#include "CameraManipulator.h"
#include "Importer3DY.h"
#include "GLTexture.h"
#include "Scene.h"
#include "Barrier.h"
#include "ImageSize.h"
#include "GLDevice.h"
#include "Raytracer.h"

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

void updateScene(RenderEngine* render_engine, int data_index);
void renderScene(RenderEngine* render_engine, int data_index);

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
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);/*GLFW_OPENGL_COMPAT_PROFILE*/
   glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
   glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
   GLFWwindow* window = glfwCreateWindow(1500, 1000, "MyWindow", NULL, NULL);

   glfwMakeContextCurrent(window);
   glfwSwapInterval(1);

   glEnable(GL_DEBUG_OUTPUT);
   glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
   glDebugMessageCallback(&printGLDebugMessage, nullptr);
   glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
   //glPixelStorei(GL_PACK_ALIGNMENT, 1);
   //glPixelStorei(GL_UNPACK_ALIGNMENT, 1);


   GLDevice::bindDefaultDepthStencilState();
   GLDevice::bindDefaultColorBlendState();
   GLDevice::bindDefaultRasterizationState();
   //glEnable(GL_CULL_FACE);

   RenderEngine render_engine(ImageSize(1500, 1000));
   //char* file = "D:\\BlenderTests\\Sintel_Lite_Cycles_V2.3dy";
   //char* file = "D:\\BlenderTests\\stanford_bunny.3dy";


   char* file = "D:\\BlenderTests\\town.3dy";
   import3DY(file, render_engine, render_engine.scene());
   render_engine.offlinePrepareScene();

   if (render_engine.scene()->ao_volume && !render_engine.scene()->ao_volume->texture)
   {
      Raytracer raytracer;
      raytracer.init(*render_engine.scene());
      raytracer.bakeAmbiantOcclusionVolume(*render_engine.scene());
      //saveBakedAmbientOcclusionVolumeTo3DY(file, *render_engine.scene());
   }
      
   CameraManipulator camera_manipulator(&render_engine.scene()->camera.point_of_view);

   glfwShowWindow(window);

   int update_index = 0;
   int render_index = 1;
   updateScene(&render_engine, render_index); // bootstrap the update/render multithreaded cycle
      
   while (!glfwWindowShouldClose(window))
   {
      glfwPollEvents();
      handleInputs(window, &camera_manipulator);

      GLDynamicBuffer::moveActiveSegments();
      // we run at the same time the render of the current frame and the update of the next one
      auto next_frame_scene_update_fence = std::async(updateScene, &render_engine, update_index);

      renderScene(&render_engine, render_index);
      //render_engine.presentDebugTexture();      
      glfwSwapBuffers(window);       

      next_frame_scene_update_fence.wait();  
      // end of multithreaded section

      std::swap(update_index, render_index);
   }

   glfwTerminate();
   return 0;
}

void updateScene(RenderEngine* render_engine, int data_index)
{
   render_engine->updateScene(render_engine->scene()->render_data[data_index]);
}

void renderScene(RenderEngine* render_engine, int data_index)
{
   render_engine->renderScene(render_engine->scene()->render_data[data_index]);
}


