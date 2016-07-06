#include "Scene.h"

#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "GLBuffer.h"
#include "RenderMesh.h"
#include "GLTexture.h"
#include "AnimationPlayer.h"
#include "TransformHierarchy.h"


namespace yare {

Scene::Scene()
: animation_player(new AnimationPlayer())
{

}

Scene::~Scene()
{
}


}  // namespace yare