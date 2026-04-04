#include "Shaders/WhittedShader.hpp"

#include "Math/Math.hpp"
#include "Math/RGB.hpp"
#include "Math/Vector.hpp"
#include "Primitive/Material.hpp"
#include "Primitive/Primitive.hpp"
#include "Ray/Intersection.hpp"
#include "Ray/Ray.hpp"
#include "Scene/Scene.hpp"

#include <glm/common.hpp>
#include <glm/geometric.hpp>

namespace VI {

constexpr float MAX_DEPTH = 5;

RGB WhittedShader::Execute(const Ray &ray, const Scene &scene) const {
  Intersection intersection{};
  if (scene.Trace(ray, intersection)) {
    return DoExecute(ray, scene, intersection);
  }

  return m_BackgroundColor;
}

RGB WhittedShader::DoExecute(const Ray &ray, const Scene &scene,
                             const Intersection &intersection,
                             int depth) const {
  RGB color = RGB{0.0f};
  if (depth > MAX_DEPTH) {
    return color;
  }

  const Primitive &primitive = scene.GetPrimitive(intersection.ObjectIndex);
  const Material &material = scene.GetMaterial(primitive.MaterialIndex);

  if (material.GetMetallic() > 0.f) {
    color += IndirectIllumination(ray, scene, intersection, material, depth);
  }

  return color + DirectIllumination(ray, scene, intersection, material);
}

RGB WhittedShader::DirectIllumination(const Ray &ray, const Scene &scene,
                                      const Intersection &intersection,
                                      const Material &material) const {

  RGB color{0.0f};

  Vector shading_normal = FaceForward(intersection.Normal, -ray.Direction);
  const RGB diffuse_color =
      material.GetAlbedo() * (1.f - material.GetMetallic());

  for (const auto &light : scene.GetLights()) {
    const auto &light_material = scene.GetMaterial(light->GetMaterialIndex());
    auto light_type = light->GetType();
    if (light_type == LightType::Ambient) {
      color += diffuse_color * light_material.GetRadiance();
    } else if (light_type == LightType::Point) {
      auto *point_light = static_cast<PointLight *>(light.get());
      const Vector light_position = point_light->GetPosition();
      const Vector direction =
          glm::normalize(light_position - intersection.Position);
      const float light_distance =
          glm::length(light_position - intersection.Position);

      if (const float cos_light = glm::dot(direction, shading_normal);
          cos_light > 0) {
        const auto shadow_ray =
            Ray::WithOffset(intersection.Position, direction,
                            intersection.Normal, light_distance * EPSILON);

        if (scene.Visibility(shadow_ray, light_distance)) {
          color += cos_light * diffuse_color * light_material.GetRadiance();
        }
      }
    }
  }

  return color;
}

RGB WhittedShader::IndirectIllumination(const Ray &ray, const Scene &scene,
                                        const Intersection &intersection,
                                        const Material &material,
                                        int depth) const {
  const Vector shading_normal =
      FaceForward(intersection.Normal, -ray.Direction);
  const Vector reflected = glm::reflect(ray.Direction, shading_normal);
  const float cos_theta =
      glm::max(0.0f, glm::dot(shading_normal, -ray.Direction));
  Ray scattered_ray =
      Ray::WithOffset(intersection.Position, reflected, intersection.Normal);

  const RGB r0 = material.GetAlbedo();
  const RGB fresnel = r0 + (RGB{1.f} - r0) * glm::pow(1.f - cos_theta, 5.f);

  Intersection scattered_intersection{};
  if (!scene.Trace(scattered_ray, scattered_intersection)) {
    return fresnel * m_BackgroundColor;
  }

  return fresnel *
         DoExecute(scattered_ray, scene, scattered_intersection, depth + 1);
}
} // namespace VI
