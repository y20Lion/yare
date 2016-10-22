#pragma once

#include <string>

namespace yare {

class Scene;
class RenderEngine;

void import3DY(const std::string& filename, const RenderEngine& render_engine, Scene* scene);

void saveBakedAmbientOcclusionVolumeTo3DY(const std::string& filename, const Scene& scene);
void saveBakedSignedDistanceFieldVolumeTo3DY(const std::string& filename, const Scene& scene);

}