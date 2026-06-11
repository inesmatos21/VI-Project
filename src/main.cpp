#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <optional>
#include <sstream>
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
  std::string Theme = "";
  std::optional<std::filesystem::path> ScenePath = std::nullopt;
  int SamplesPerPixel = 128;
  bool MotionBlur = false;
  AccelerationStructureType Accel = AccelerationStructureType::BVH;
  std::optional<Point> Eye = std::nullopt;
  std::optional<Point> At = std::nullopt;
  std::optional<float> FovDegrees = std::nullopt;
};

Point ParsePoint(std::string_view text)
{
  std::string s{text};
  std::replace(s.begin(), s.end(), ',', ' ');
  std::istringstream stream{s};
  Point p{};
  if (!(stream >> p.x >> p.y >> p.z))
  {
    throw std::invalid_argument("expected three comma-separated numbers, got: " + s);
  }
  return p;
}

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

    if (arg == "--accel")
    {
      if (i + 1 >= argc)
      {
        throw std::invalid_argument("--accel requires 'bvh' or 'grid'");
      }
      const std::string_view value{argv[++i]};
      if (value == "bvh")
      {
        options.Accel = AccelerationStructureType::BVH;
      }
      else if (value == "grid")
      {
        options.Accel = AccelerationStructureType::Grid;
      }
      else
      {
        throw std::invalid_argument("--accel requires 'bvh' or 'grid'");
      }
      continue;
    }

    if (arg == "--eye")
    {
      if (i + 1 >= argc)
      {
        throw std::invalid_argument("--eye requires x,y,z");
      }
      options.Eye = ParsePoint(argv[++i]);
      continue;
    }

    if (arg == "--at")
    {
      if (i + 1 >= argc)
      {
        throw std::invalid_argument("--at requires x,y,z");
      }
      options.At = ParsePoint(argv[++i]);
      continue;
    }

    if (arg == "--fov")
    {
      if (i + 1 >= argc)
      {
        throw std::invalid_argument("--fov requires degrees");
      }
      options.FovDegrees = std::stof(argv[++i]);
      continue;
    }

    // Argumento posicional: o tema a renderizar (ex.: VI-RT bvh)
    if (!arg.starts_with("--"))
    {
      if (!options.Theme.empty())
      {
        throw std::invalid_argument("Only one theme may be given, got '" + options.Theme + "' and '" + std::string{arg} + "'");
      }
      options.Theme = std::string{arg};
      continue;
    }

    throw std::invalid_argument("Unknown argument: " + std::string{arg});
  }

  if (options.Theme.empty())
  {
    if (options.MotionBlur)
    {
      options.Theme = "motion-blur";
    }
    else if (options.ScenePath.has_value())
    {
      options.Theme = "gltf";
    }
    else
    {
      options.Theme = "veach";
    }
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

  if (options.Theme == "motion-blur")
  {
    // Dedicated motion-blur scene: bouncing coloured spheres with an ambient
    // light, rendered with the path tracer for correct global illumination.
    PathTracingShader shader{{0.5f, 0.7f, 1.0f}, DirectIlluminationMode::Importance};
    Scene scene = CreateMotionBlurScene();
    scene.SetAccelerationStructureType(options.Accel);
    scene.Build();
    const Camera& cam = *scene.GetCamera();
    image = renderer.Render(scene, cam, shader, options.SamplesPerPixel, true);
  }
  else if (options.Theme == "bvh")
  {
    // Dedicated BVH scene: a large field of spheres, demanding enough that
    // the acceleration structure dominates the render time (compare --accel).
    PathTracingShader shader{{0.5f, 0.7f, 1.0f}, DirectIlluminationMode::Importance};
    Scene scene = CreateBVHScene();
    scene.SetAccelerationStructureType(options.Accel);
    scene.Build();
    const Camera& cam = *scene.GetCamera();
    image = renderer.Render(scene, cam, shader, options.SamplesPerPixel, true);
  }
  else if (options.Theme == "sponza" || options.Theme == "gltf")
  {
    const bool is_sponza = options.Theme == "sponza";
    const std::filesystem::path scene_path = options.ScenePath.value_or(std::filesystem::path{"scenes/Sponza/glTF/Sponza.gltf"});

    // Sponza tem uma vista interior predefinida; outras cenas glTF usam a
    // câmara embebida no ficheiro ou a posição dada por --eye/--at/--fov.
    const Point default_eye = is_sponza ? Point{-11.f, 2.f, 0.f} : Point{0.f, 2.f, -7.f};
    const Point default_at = is_sponza ? Point{10.f, 3.5f, 0.f} : Point{0.f, 1.f, 2.f};
    const float default_fov = is_sponza ? 60.f : 45.f;

    constexpr Vector Up = {0, 1, 0};
    const float fovHrad = options.FovDegrees.value_or(default_fov) * 3.14f / 180.f;
    Camera camera{options.Eye.value_or(default_eye), options.At.value_or(default_at), Up, w, h, fovHrad};

    PathTracingShader path_tracing_shader{{0.0f, 0.0f, 0.0f}, DirectIlluminationMode::Importance};
    Scene scene = CreateGltfScene(scene_path, w, h);
    scene.SetAccelerationStructureType(options.Accel);
    scene.Build();

    const BoundingBox scene_bounds = scene.ComputeBoundingBox();
    std::cout << "Scene bounds: min(" << scene_bounds.Min.x << ", " << scene_bounds.Min.y << ", " << scene_bounds.Min.z << ") max(" << scene_bounds.Max.x << ", " << scene_bounds.Max.y << ", "
              << scene_bounds.Max.z << ")\n";

    // The glTF camera wins unless the user explicitly placed one on the
    // command line (the sponza theme always uses the interior view).
    const bool custom_camera = options.Eye.has_value() || options.At.has_value() || is_sponza;
    const Camera& render_camera = (scene.GetCamera() != nullptr && !custom_camera) ? *scene.GetCamera() : camera;
    image = renderer.Render(scene, render_camera, path_tracing_shader, options.SamplesPerPixel, true);
  }
  else if (options.Theme == "veach")
  {
    constexpr Point Eye = {0, 2, -7};
    constexpr Point At = {0, 1, 2};
    constexpr Vector Up = {0, 1, 0};
    constexpr float fovH = 45.f;
    constexpr float fovHrad = fovH * 3.14f / 180.f;
    Camera camera{Eye, At, Up, w, h, fovHrad};

    VeachShader veach_shader{{0.0f, 0.0f, 0.0f}};
    Scene scene = CreateVeachScene();
    scene.SetAccelerationStructureType(options.Accel);
    scene.Build();
    image = renderer.Render(scene, camera, veach_shader, options.SamplesPerPixel, true);
  }
  else
  {
    throw std::invalid_argument("Unknown theme '" + options.Theme + "'. Available: veach, motion-blur, bvh, sponza");
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