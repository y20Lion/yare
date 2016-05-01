#pragma once

#include <glm/vec2.hpp>
#include "Camera.h"

namespace yare {

enum class CameraAction {None, Zoom, Pan, Orbit};

class CameraManipulator
{
public:
    CameraManipulator(PointOfView* manipulated_pov);

    void motionStart(CameraAction action, double x, double y);
    void motionEnd();
    void mouseMotion(double x, double y);    

    bool isInMotion() { return  _action != CameraAction::None; }

private:
    void _motionZoom(const glm::vec2& mouse_current_pos);
    void _motionPan(const glm::vec2& mouse_current_pos);
    void _motionOrbit(const glm::vec2& mouse_current_pos);

private:
    PointOfView* _pov;
    CameraAction _action;
    glm::vec2 _mouse_start_pos;
    PointOfView _start_pov;
};

}