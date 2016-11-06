#include "stdafx.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <assert.h>
#include <iostream>
#include <atomic>
#include <future>

#include "GLBuffer.h"
#include "RenderEngine.h"
#include "ClusteredLightCuller.h"
#include "CameraManipulator.h"
#include "Importer3DY.h"
#include "GLTexture.h"
#include "Scene.h"
#include "Barrier.h"
#include "ImageSize.h"
#include "GLDevice.h"
#include "Raytracer.h"
#include "AppGui.h"

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

float updateScene(RenderEngine* render_engine, int data_index, GLFWwindow* update_context);
float renderScene(RenderEngine* render_engine, int data_index, AppGui* app_gui, GLFWwindow* render_context);

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
   //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
   glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
   glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
   GLFWwindow* window = glfwCreateWindow(1500, 1000, "MyWindow", NULL, NULL);
   GLFWwindow* update_context = glfwCreateWindow(1, 1, "", NULL, window);

   glfwMakeContextCurrent(window);
   glfwSwapInterval(1);

   glEnable(GL_DEBUG_OUTPUT);
   glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
   glDebugMessageCallback(&printGLDebugMessage, nullptr);
   glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
   //glPixelStorei(GL_PACK_ALIGNMENT, 1);
   //glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   
   RenderEngine render_engine(ImageSize(1500, 1000));

   AppGui app_gui(window, &render_engine);

   GLDevice::bindDefaultDepthStencilState();
   GLDevice::bindDefaultColorBlendState();
   GLDevice::bindDefaultRasterizationState();
   //glEnable(GL_CULL_FACE);

   
   //char* file = "D:\\BlenderTests\\Sintel_Lite_Cycles_V2.3dy";
   //char* file = "D:\\BlenderTests\\stanford_bunny.3dy";


   //char* file = "D:\\BlenderTests\\DeLorean.3dy";
   char* file = "D:\\BlenderTests\\test_clustered_shading.3dy";
   import3DY(file, render_engine, render_engine.scene());
   render_engine.offlinePrepareScene();

   bool bake_ao_volume = render_engine.scene()->ao_volume && !render_engine.scene()->ao_volume->ao_texture;
   bool bake_sdf_volume =  render_engine.scene()->sdf_volume && !render_engine.scene()->sdf_volume->sdf_texture;
   if (bake_ao_volume || bake_sdf_volume)
   {
      Raytracer raytracer;
      raytracer.init(*render_engine.scene());
      
      if (bake_ao_volume)
      {
         raytracer.bakeAmbiantOcclusionVolume(*render_engine.scene());
         saveBakedAmbientOcclusionVolumeTo3DY(file, *render_engine.scene());
      }

      if (bake_sdf_volume)
      {
         raytracer.bakeSignedDistanceFieldVolume(*render_engine.scene());
         saveBakedSignedDistanceFieldVolumeTo3DY(file, *render_engine.scene());
      }
   }
      
   CameraManipulator camera_manipulator(&render_engine.scene()->camera.point_of_view);

   glfwShowWindow(window);

   int update_index = 0;
   int render_index = 1;
   updateScene(&render_engine, render_index, update_context); // bootstrap the update/render multithreaded cycle
   int i = 0;
   while (!glfwWindowShouldClose(window))
   {
      glfwPollEvents();
      if (!app_gui.hasMouseFocus())
      {
         handleInputs(window, &camera_manipulator);
         if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
            render_engine.clustered_light_culler->debugUpdateClusteredGrid(render_engine.scene()->render_data[render_index]);
      }
      
          
      GLDynamicBuffer::moveActiveSegments();

      // we run at the same time the render of the current frame and the update of the next one
      auto next_frame_scene_update_fence = std::async(updateScene, &render_engine, update_index, update_context);
      float render_duration = renderScene(&render_engine, render_index, &app_gui, window);      
      float update_duration = next_frame_scene_update_fence.get();
      // end of multithreaded section

      
      if ( (i++) % 60 == 0 )
         app_gui.reportCPUTimings(render_duration*1000.0f, update_duration*1000.0f);

      std::swap(update_index, render_index);
   }

   glfwTerminate();
   return 0;
}

float updateScene(RenderEngine* render_engine, int data_index, GLFWwindow* update_context)
{
   auto start = std::chrono::steady_clock::now();
   
   glfwMakeContextCurrent(update_context);
   
   render_engine->updateScene(render_engine->scene()->render_data[data_index]);

   glfwMakeContextCurrent(nullptr);

   float update_duration = std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count();
   return update_duration;   
}

float renderScene(RenderEngine* render_engine, int data_index, AppGui* app_gui, GLFWwindow* window)
{
   auto start = std::chrono::steady_clock::now();
   
   render_engine->renderScene(render_engine->scene()->render_data[data_index]);
   //render_engine.presentDebugTexture();  
   app_gui->drawWidgets();
  
   glfwSwapBuffers(window);

   float render_duration = std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count();
   return render_duration;
}


