#include "AppGui.h"

#include "RenderEngine.h"
#include "RenderResources.h"
#include "GLGPUTimer.h"

using namespace nanogui;

namespace yare {


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

static bool nanogui_mouse_event = false;

Slider * slider;
Label* lol;
Graph* graph;
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
   ref<Window> nanoguiWindow = gui->addWindow(Eigen::Vector2i(10, 10), "Form helper example");
   gui->addGroup("Basic types");
   gui->addVariable("bool", bvar)->setTooltip("Test tooltip.");
   gui->addVariable("string", strval);

   gui->addGroup("Validating; fields");
   gui->addVariable("int", ivar)->setSpinnable(true);
   gui->addVariable("float", fvar)->setTooltip("Test.");
   gui->addVariable("double", dvar)->setSpinnable(true);

   gui->addGroup("Complex types");
   gui->addVariable("Enumeration", enumval, enabled)->setItems({ "Item 1", "Item 2", "Item 3" });
   gui->addVariable("Color", colval);

   //gui->addSmallGroup("Stuff1");
   //gui->addButton("A button", []() { std::cout << "Button pressed." << std::endl; })->setTooltip("Testing a much longer tooltip, that will wrap around to new lines multiple times.");
   gui->addSpace();
   Label* test4 = new Label(nanoguiWindow, "3.0");
   gui->addWidget("Stuff:", test4);
   Slider* slider = new Slider(nanoguiWindow);
   
   gui->addWidget("", slider);
   slider->setCallback([test4](float value) { test4->setCaption(string_format("%.2f", value)); });

   gui->addSpace();
   gui->addWidget("other Stuff:", new Label(nanoguiWindow, "5.0"));
   gui->addWidget("", new Slider(nanoguiWindow));


   //nanoguiWindow->setFixedWidth(400);

   nanoguiWindow->setPosition(Vector2i(width - nanoguiWindow->preferredSize(screen->nvgContext())[0] - 10, 5));
 

   ref<Window> hudWindow = new Window(screen, "");//hud_helper->addWindow(Eigen::Vector2i(10, 10), "");
   hudWindow->setLayout(new BoxLayout(Orientation::Horizontal));

   _hud = new Label(hudWindow, "lol");
   _hud->setFontSize(20);
   _hud->setFixedSize(Vector2i(200, 0));
   _hud->setColor(Color(1.0f, 1.0f, 0.0f, 1.0f));

   ref<Window> graph_window = new Window(screen, "");//hud_helper->addWindow(Eigen::Vector2i(10, 10), "");
   graph_window->setLayout(new BoxLayout(Orientation::Horizontal));

   graph = new Graph(graph_window, "lol");
   graph_window->setPosition(Vector2i(200, 200));
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

   std::string hud_text = string_format("Z PREPASS: %.2fms\n SSAO: %.2fms\n MATERIALS: %.2fms BACKGROUND: %.2fms\n CPU Render:%.2fms\n CPU Update:%.2fms",
                                        render_resources->z_pass_timer->elapsedTimeInMs(),
                                        render_resources->ssao_timer->elapsedTimeInMs(),
                                        render_resources->material_pass_timer->elapsedTimeInMs(),
                                        render_resources->background_timer->elapsedTimeInMs(),
                                        _render_time_ms,
                                       _update_time_ms);
   //std::cout << slider->value() << std::endl;
   _hud->setCaption(hud_text);

   for (int i = 0; i < 20; ++i)
      graph->values()[i] = _render_times[i]/10.0;
  /* static int i = 0;
   ++i;
   lol->setCaption(std::to_string(i));*/
   /*Vector2i pref = _hud->preferredSize(screen->nvgContext());
   _hud->setSize(pref);
   _hud->performLayout(screen->nvgContext()); */  

   screen->performLayout();
}

}