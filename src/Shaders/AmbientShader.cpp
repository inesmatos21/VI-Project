#include "Shaders/AmbientShader.hpp"

#include "Math/RGB.hpp"
#include "Primitive/Primitive.hpp"
#include "Ray/Intersection.hpp"
#include "Scene/Scene.hpp"

namespace VI
{
RGB AmbientShader::Execute(const Ray& ray, const Scene& scene) const
{
  Intersection intersection{};
  if (!scene.Trace(ray, intersection))
  {
    return m_BackgroundColor;
  }

  const Primitive& primitive = scene.GetPrimitive(intersection.ObjectIndex);
  const Material& material = scene.GetMaterial(primitive.MaterialIndex);

  RGB color{0.f};

  for (const auto& light : scene.GetLights())
  {
    const auto& light_material = scene.GetMaterial(light->GetMaterialIndex());

    if (light->GetType() == LightType::Ambient)
    {
      color += material.GetAlbedo() * light_material.GetRadiance();
    }
  }
  return color;
}

} // namespace VI
