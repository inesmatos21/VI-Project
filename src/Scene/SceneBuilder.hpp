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

// Motion Blur
Scene CreateMotionBlurScene();

Scene CreateGltfScene(const std::filesystem::path& path, int camera_width = 800, int camera_height = 600);
} // namespace VI
