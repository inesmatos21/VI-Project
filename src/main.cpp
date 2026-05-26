#include <chrono>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

#include "Camera/Camera.hpp"
#include "Image/FileImages.hpp"
#include "Image/Image.hpp"
#include "Math/Vector.hpp"
#include "Renderer/Renderer.hpp"
#include "Scene/Scene.hpp"
#include "Scene/SceneBuilder.hpp"
#include "Shaders/AmbientShader.hpp"
#include "Shaders/DirectIllumination.hpp"
#include "Shaders/PathTracingShader.hpp"
#include "Shaders/VeachShader.hpp"
#include "Shaders/WhittedShader.hpp"

/*  uncomment the folowiing line to perform
    post-rendering denoising (Intel OIDN)
 */
#define __DENOISE__
#if defined(__DENOISE__)
#include "Image/Denoiser.hpp"
#endif

#include <chrono>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string_view>

using namespace VI;

namespace
{

struct CommandLineOptions
{
  std::optional<std::filesystem::path> ScenePath = std::nullopt;
  int SamplesPerPixel = 128;
  bool MotionBlur = false;
};

CommandLineOptions ParseCommandLine(int argc, char** argv)
{
  CommandLineOptions options{};
  for (int i = 1; i < argc; ++i)
  {
    const std::string_view arg{argv[i]};
    if (arg == "--scene")
    {
      if (i + 1 >= argc)
      {
        throw std::invalid_argument("--scene requires a glTF path");
      }
      options.ScenePath = std::filesystem::path{argv[++i]};
      continue;
    }

    if (arg == "--spp")
    {
      if (i + 1 >= argc)
      {
        throw std::invalid_argument("--spp requires a positive sample count");
      }
      options.SamplesPerPixel = std::stoi(argv[++i]);
      if (options.SamplesPerPixel <= 0)
      {
        throw std::invalid_argument("--spp requires a positive sample count");
      }
      continue;
    }

    if (arg == "--motion-blur")
    {
      options.MotionBlur = true;
      continue;
    }

    throw std::invalid_argument("Unknown argument: " + std::string{arg});
  }
  return options;
}

} // namespace

int main(int argc, char** argv)
{
  auto begin = std::chrono::system_clock::now();
  const auto options = ParseCommandLine(argc, argv);

  constexpr int w = 1280;
  constexpr int h = 720;

  Renderer renderer;
  Image image{w, h};

  if (options.MotionBlur)
  {
    // Dedicated motion-blur scene: 
    PathTracingShader shader{{0.5f, 0.7f, 1.0f}, DirectIlluminationMode::Importance};
    Scene scene = CreateMotionBlurScene();
    scene.Build();
    const Camera& cam = *scene.GetCamera();
    image = renderer.Render(scene, cam, shader, options.SamplesPerPixel, true);
  }
  else if (options.ScenePath.has_value())
  {
    // Veach Camera
    constexpr Point Eye = {0, 2, -7};
    constexpr Point At = {0, 1, 2};
    constexpr Vector Up = {0, 1, 0};
    constexpr float fovH = 45.f;
    constexpr float fovHrad = fovH * 3.14f / 180.f;
    Camera camera{Eye, At, Up, w, h, fovHrad};

    PathTracingShader path_tracing_shader{{0.0f, 0.0f, 0.0f}, DirectIlluminationMode::Importance};
    Scene scene = CreateGltfScene(*options.ScenePath, w, h);
    scene.Build();
    const Camera& render_camera = scene.GetCamera() != nullptr ? *scene.GetCamera() : camera;
    image = renderer.Render(scene, render_camera, path_tracing_shader, options.SamplesPerPixel, true);
  }
  else
  {
    // Default: Veach MIS scene
    constexpr Point Eye = {0, 2, -7};
    constexpr Point At = {0, 1, 2};
    constexpr Vector Up = {0, 1, 0};
    constexpr float fovH = 45.f;
    constexpr float fovHrad = fovH * 3.14f / 180.f;
    Camera camera{Eye, At, Up, w, h, fovHrad};

    VeachShader veach_shader{{0.0f, 0.0f, 0.0f}};
    Scene scene = CreateVeachScene();
    scene.Build();
    image = renderer.Render(scene, camera, veach_shader, options.SamplesPerPixel, true);
  }

  ImagePPM::Save(image, "image.ppm");

  auto end = std::chrono::system_clock::now();
  auto duration = std::chrono::duration<double>(end - begin);
  std::cout << "Time to render: " << duration.count() << " sec" << '\n';

#if defined(__DENOISE__)
  begin = std::chrono::system_clock::now();

  Denoiser denoise(w, h);
  const auto denoised_image = denoise.Execute(image);

  ImagePPM::Save(denoised_image, "image-OIDN.ppm");

  end = std::chrono::system_clock::now();
  duration = std::chrono::duration<double>(end - begin);
  std::cout << "Time to denoise: " << duration.count() << " sec" << '\n';
#endif

  return 0;
}