#include "stdafx.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <nanogui/nanogui.h>


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





using namespace nanogui;

enum test_enum {
   Item1 = 0,
   Item2,
   Item3
};

bool bvar = true;
int ivar = 12345678;
double dvar = 3.1415926;
float fvar = (float)dvar;
std::string strval = "A string";
test_enum enumval = Item2;
Color colval(0.5f, 0.5f, 0.7f, 1.f);

Screen *screen = nullptr;

bool nanogui_mouse_event = false;

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




   // Create a nanogui screen and pass the glfw pointer to initialize
   screen = new Screen();
   screen->initialize(window, true);

   int width, height;
   glfwGetFramebufferSize(window, &width, &height);
   glViewport(0, 0, width, height);
   //glfwSwapInterval(0);
   //glfwSwapBuffers(window);

   // Create nanogui gui
   bool enabled = true;
   FormHelper *gui = new FormHelper(screen);
   ref<Window> nanoguiWindow = gui->addWindow(Eigen::Vector2i(10, 10), "Form helper example");
   gui->addGroup("Basic types");
   gui->addVariable("bool", bvar)->setTooltip("Test tooltip.");
   gui->addVariable("string", strval);

   gui->addGroup("Validating fields");
   gui->addVariable("int", ivar)->setSpinnable(true);
   gui->addVariable("float", fvar)->setTooltip("Test.");
   gui->addVariable("double", dvar)->setSpinnable(true);

   gui->addGroup("Complex types");
   gui->addVariable("Enumeration", enumval, enabled)->setItems({ "Item 1", "Item 2", "Item 3" });
   gui->addVariable("Color", colval);

   gui->addGroup("Other widgets");
   gui->addButton("A button", []() { std::cout << "Button pressed." << std::endl; })->setTooltip("Testing a much longer tooltip, that will wrap around to new lines multiple times.");;

   nanoguiWindow->setPosition(Vector2i(width - nanoguiWindow->preferredSize(screen->nvgContext())[0] - 5, 5));

   
   ref<TextBox> hud = new TextBox(screen, "lol");
   hud->setPosition(Vector2i(500, 100));

   screen->setVisible(true);
   screen->performLayout();



   glfwSetCursorPosCallback(window,
                            [](GLFWwindow *, double x, double y) {
      screen->cursorPosCallbackEvent(x, y);
   }
   );

   
   glfwSetMouseButtonCallback(window,
      [](GLFWwindow *, int button, int action, int modifiers)
      {
         nanogui_mouse_event = screen->mouseButtonCallbackEvent(button, action, modifiers);

      }
   );

   glfwSetKeyCallback(window,
                      [](GLFWwindow *, int key, int scancode, int action, int mods) {
      screen->keyCallbackEvent(key, scancode, action, mods);
   }
   );

   glfwSetCharCallback(window,
                       [](GLFWwindow *, unsigned int codepoint) {
      screen->charCallbackEvent(codepoint);
   }
   );

   glfwSetDropCallback(window,
                       [](GLFWwindow *, int count, const char **filenames) {
      screen->dropCallbackEvent(count, filenames);
   }
   );

   glfwSetScrollCallback(window,
                         [](GLFWwindow *, double x, double y) {
      screen->scrollCallbackEvent(x, y);
   }
   );

   glfwSetFramebufferSizeCallback(window,
                                  [](GLFWwindow *, int width, int height) {
      screen->resizeCallbackEvent(width, height);
   }
   );











   GLDevice::bindDefaultDepthStencilState();
   GLDevice::bindDefaultColorBlendState();
   GLDevice::bindDefaultRasterizationState();
   //glEnable(GL_CULL_FACE);

   RenderEngine render_engine(ImageSize(1500, 1000));
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
   updateScene(&render_engine, render_index); // bootstrap the update/render multithreaded cycle
   int i = 0;
   while (!glfwWindowShouldClose(window))
   {
      glfwPollEvents();
      if (!nanogui_mouse_event)
      {
         handleInputs(window, &camera_manipulator);
         if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
            render_engine.clustered_light_culler->debugUpdateClusteredGrid(render_engine.scene()->render_data[render_index]);
      }
      

      GLDynamicBuffer::moveActiveSegments();
      // we run at the same time the render of the current frame and the update of the next one
      auto next_frame_scene_update_fence = std::async(updateScene, &render_engine, update_index);

      renderScene(&render_engine, render_index);
      //render_engine.presentDebugTexture();   

      // Draw nanogui
      hud->setValue(std::to_string(i));
      ++i;
      //hud->performLayout(screen->nvgContext());

      Vector2i pref = hud->preferredSize(screen->nvgContext());
      hud->setSize(pref);
      hud->performLayout(screen->nvgContext());
      //hud->upd();
      screen->drawContents();
      screen->drawWidgets();
      
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


