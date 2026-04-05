#pragma once

#include "Camera/Camera.hpp"
#include "Image/Image.hpp"
#include "Shaders/Shader.hpp"
#include "Utils/ProgressBar.hpp"

namespace VI
{
class Scene;
class Camera;

class DummyRenderer final
{
public:
  template <Shader S> Image Render(const Scene& scene, const Camera& camera, const S& shader)
  {
    auto [width, height] = camera.GetResolution();

    ProgressBar bar{static_cast<int>(width * height)};
    Image image{static_cast<int>(width), static_cast<int>(height)};

    for (int y = 0; y < height; ++y)
    {
      for (int x = 0; x < width; ++x)
      {
        Ray ray = camera.GenerateRay(x, y);
        ray.Direction.x = x;
        ray.Direction.y = y;

        RGB color = shader.Execute(ray, scene);
        image.Set(x, y, color);
        bar.Increment();
      }
    }

    bar.Finish();

    return image;
  }
};
} // namespace VI
