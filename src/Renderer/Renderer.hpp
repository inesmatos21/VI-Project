#pragma once

#include "Camera/Camera.hpp"
#include "Image/Image.hpp"
#include "Math/Random.hpp"
#include "Shaders/Shader.hpp"
#include "Utils/ProgressBar.hpp"

#include <future>
#include <vector>

namespace VI
{
class Scene;
class Camera;

class Renderer final
{
public:
  template <Shader S> Image Render(const Scene& scene, const Camera& camera, const S& shader, int samples_per_pixel = 1, bool do_jittering = false)
  {
    auto [width, height] = camera.GetResolution();

    Image image{static_cast<int>(width), static_cast<int>(height)};
    ProgressBar progress{static_cast<int>(width * height)};

    std::vector<std::future<void>> futures;

    float spp_factor = 1.0f / samples_per_pixel;
    for (int y = 0; y < static_cast<int>(height); ++y)
    {
      for (int x = 0; x < static_cast<int>(width); ++x)
      {
        RGB color = RGB{0.0f};
        for (int s = 0; s < samples_per_pixel; s++)
        {
          Vector jitter = {0.5, 0.5, 0.0};
          if (do_jittering)
          {
            jitter = Random::RandomVec3(0, 1);
          }
          const Ray ray = camera.GenerateRay(x, y, jitter);
          color += shader.Execute(ray, scene);
        }
        image.Set(x, y, color * spp_factor);
        progress.Increment();
      }
    }

    progress.Finish();

    return image;
  }
};
} // namespace VI
