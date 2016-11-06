#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <nanogui/nanogui.h>
#include <deque>

struct GLFWwindow;

namespace yare
{

class RenderEngine;

class AppGui
{
public:
   AppGui(GLFWwindow* window, RenderEngine* render_engine);

   bool hasMouseFocus();
   void drawWidgets();
   void reportCPUTimings(float render, float update);

private:
   void _updateHUDText();

private:
   nanogui::ref<nanogui::Label> _hud;
   RenderEngine* _render_engine;

   float _render_time_ms, _update_time_ms;
   std::deque<float> _render_times;
};

}