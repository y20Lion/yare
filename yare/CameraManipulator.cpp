#include "CameraManipulator.h"

#include <cmath>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include "Camera.h"

namespace yare {

using namespace glm;

 CameraManipulator::CameraManipulator(PointOfView* manipulated_pov)
     : _pov(manipulated_pov)
     , _action(CameraAction::None)
     , _mouse_start_pos(0.0f)
 {
 }

void CameraManipulator::motionStart(CameraAction action, double x, double y)
{
    _action = action;
    _mouse_start_pos = vec2(x, y);
}

void CameraManipulator::motionEnd()
{
    _action = CameraAction::None;
    _start_pov = *_pov;
}

void CameraManipulator::mouseMotion(double x, double y)
{
    if (_action == CameraAction::None)
        return;

    vec2 current_pos(x, y);    

    if (_action == CameraAction::Zoom)
        _motionZoom(current_pos);
    else if (_action == CameraAction::Pan)
        _motionPan(current_pos);
    else if (_action == CameraAction::Orbit)
        _motionOrbit(current_pos);
}

void CameraManipulator::_motionZoom(const vec2& mouse_current_pos)
{
    float motion_delta = exp((mouse_current_pos.y - _mouse_start_pos.y)*0.01f);     
    _pov->from = _pov->to + (_start_pov.from - _start_pov.to)* motion_delta ;
}

void CameraManipulator::_motionPan(const vec2& mouse_current_pos)
{
    vec2 motion_delta = (mouse_current_pos - _mouse_start_pos) * 0.01f; 
    vec3 right = _pov->rightDirection();
    vec3 motion = right * -motion_delta.x + _pov->up * motion_delta.y;
    _pov->to = _start_pov.to + motion;
    _pov->from = _start_pov.from + motion;
}

vec3 transformPosition(const mat4& transform, const vec3& position)
{
    return (transform * vec4(position, 1.0)).xyz;
}

vec3 transformDirection(const mat4& transform, const vec3& position)
{
    return (transform * vec4(position, 0.0)).xyz;
}

void CameraManipulator::_motionOrbit(const vec2& mouse_current_pos)
{
    vec2 angle_delta = (mouse_current_pos - _mouse_start_pos) * float(M_PI/600.0);

    auto rotate_around_right_vector = translate(_start_pov.to) * rotate(angle_delta.y, _start_pov.rightDirection()) * translate(-_start_pov.to);
    auto up_after_first_rotation = transformDirection(rotate_around_right_vector, _start_pov.up);
    auto from_after_first_rotation = transformPosition(rotate_around_right_vector, _start_pov.from);     

    auto rotate_around_z = translate(_pov->to) * rotate(-angle_delta.x, vec3(0.0f, 0.0f, 1.0f)) * translate(-_pov->to);
    _pov->up = transformDirection(rotate_around_z, up_after_first_rotation);
    _pov->from = transformPosition(rotate_around_z, from_after_first_rotation);
}

}