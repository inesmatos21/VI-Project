#pragma once

#include "Scene/Scene.hpp"

#include <filesystem>

namespace VI
{
Scene CreateCornellBox();
Scene CreateImportanceSamplingCornellBox();
Scene WhittedCornellBox();
Scene SphereScene();
Scene CreateVeachScene();
Scene CreateVeachScene2();
Scene CreateGltfScene(const std::filesystem::path& path);
} // namespace VI
