#include "AppGui.h"

#include "RenderEngine.h"
#include "RenderResources.h"
#include "GLGPUTimer.h"

using namespace nanogui;

namespace yare {


static bool nanogui_mouse_event = false;
static Graph* graph;
static Screen *screen = nullptr;

static void addSliderVariable(FormHelper* gui, Window* nanogui_window, const std::string& name, float* value, float value_min, float value_max)
{
   gui->addSpace();

   Label* value_label = new Label(nanogui_window, std::to_string(*value));
   gui->addWidget(name +":", value_label);
   Slider* slider = new Slider(nanogui_window);

   gui->addWidget("", slider);
   slider->setRange(std::make_pair(value_min, value_max));
   slider->setCallback([value_label, value](float new_value) 
   { 
      *value = new_value;
      value_label->setCaption(string_format("%.2f", new_value));
   });
}

AppGui::AppGui(GLFWwindow* window, RenderEngine* render_engine)
   : _render_engine(render_engine)
{
   // Create a nanogui screen and pass the glfw pointer to initialize
   screen = new Screen();
   screen->initialize(window, true);

   int width, height;
   glfwGetFramebufferSize(window, &width, &height);

   // Create nanogui gui
   bool enabled = true;
   FormHelper *gui = new FormHelper(screen);
   ref<Window> nanoguiWindow = gui->addWindow(Eigen::Vector2i(10, 10), "Tweaking");
   gui->addGroup("Froxeled shading debug");
   gui->addVariable("x", render_engine->_settings.x)->setSpinnable(true);
   gui->addVariable("y", render_engine->_settings.y)->setSpinnable(true);
   gui->addVariable("z", render_engine->_settings.z)->setSpinnable(true);
   gui->addVariable("z slices", render_engine->_settings.froxel_z_distribution_factor)->setSpinnable(true);
   addSliderVariable(gui, nanoguiWindow, "bias", &render_engine->_settings.bias, -5.0, 5.0);
   addSliderVariable(gui, nanoguiWindow, "bias2", &render_engine->_settings.bias, -5.0, 5.0);

   nanoguiWindow->setPosition(Vector2i(width - nanoguiWindow->preferredSize(screen->nvgContext())[0] - 10, 5));
 
   ref<Window> hudWindow = new Window(screen, "");
   hudWindow->setLayout(new BoxLayout(Orientation::Horizontal));

   _hud = new Label(hudWindow, "lol");
   _hud->setFontSize(20);
   _hud->setFixedSize(Vector2i(200, 0));
   _hud->setColor(Color(1.0f, 1.0f, 0.0f, 1.0f));

   ref<Window> graph_window = new Window(screen, "");
   graph_window->setLayout(new BoxLayout(Orientation::Horizontal));

   graph = new Graph(graph_window, "Render time");
   graph_window->setPosition(Vector2i(0, height - graph_window->preferredSize(screen->nvgContext())[1]));
   graph->setValues(VectorXf(20));
   _render_times.resize(20);
   
   screen->setVisible(true);
   screen->performLayout();


   glfwSetCursorPosCallback(window,
      [](GLFWwindow *, double x, double y){ screen->cursorPosCallbackEvent(x, y); }
   );
   
   glfwSetMouseButtonCallback(window,
      [](GLFWwindow *, int button, int action, int modifiers){ nanogui_mouse_event = screen->mouseButtonCallbackEvent(button, action, modifiers);}
   );

   glfwSetKeyCallback(window,
      [](GLFWwindow *, int key, int scancode, int action, int mods) {screen->keyCallbackEvent(key, scancode, action, mods); }
   );

   glfwSetCharCallback(window,
      [](GLFWwindow *, unsigned int codepoint) {screen->charCallbackEvent(codepoint);}
   );

   glfwSetDropCallback(window,
      [](GLFWwindow *, int count, const char **filenames) {screen->dropCallbackEvent(count, filenames);}
   );

   glfwSetScrollCallback(window,
      [](GLFWwindow *, double x, double y) {screen->scrollCallbackEvent(x, y);}
   );

   glfwSetFramebufferSizeCallback(window,
      [](GLFWwindow *, int width, int height) {screen->resizeCallbackEvent(width, height);}
   );
}

bool AppGui::hasMouseFocus()
{
   return nanogui_mouse_event;
}

void AppGui::drawWidgets()
{
   if (_hud->visible())
      _updateHUDText();
   
   // Draw nanogui
   screen->drawContents();
   screen->drawWidgets();
}

void AppGui::reportCPUTimings(float render, float update)
{
   _render_time_ms = render;
   _update_time_ms = update;

   _render_times.push_back(render);   
   _render_times.pop_front();
}

void AppGui::_updateHUDText()
{
   RenderResources* render_resources = _render_engine->render_resources.get();

   std::string hud_text = string_format("Z PREPASS: %.2fms\n SSAO: %.2fms\n VOLUM FOG:%.2fms\n MATERIALS: %.2fms BACKGROUND: %.2fms\n CPU Render:%.2fms\n CPU Update:%.2fms",
                                        render_resources->z_pass_timer->elapsedTimeInMs(),
                                        render_resources->ssao_timer->elapsedTimeInMs(),
                                        render_resources->volumetric_fog_timer->elapsedTimeInMs(),
                                        render_resources->material_pass_timer->elapsedTimeInMs(),
                                        render_resources->background_timer->elapsedTimeInMs(),                                        
                                        _render_time_ms,
                                       _update_time_ms);
   _hud->setCaption(hud_text);

   for (int i = 0; i < 20; ++i)
      graph->values()[i] = _render_times[i]/10.0f;

   screen->performLayout();
}

}