#pragma once

#include "Image/Image.hpp"
#include "Math/RGB.hpp"
#include "Math/Vector.hpp"

#include <algorithm>
#include <glm/common.hpp>

namespace VI
{

struct MaterialDesc
{
  std::string_view Name;
  RGB Albedo{};
  float Roughness = 1.f;
  float Metallic = 0.f;
  RGB EmissionColor{};
  float EmissionPower = 0.f;
  std::optional<Image> AlbedoImage = std::nullopt;
};

class Material
{
public:
  constexpr Material(MaterialDesc desc) : m_Name{desc.Name}, m_Albedo{desc.Albedo}, m_EmissionColor{desc.EmissionColor}, m_Metallic{desc.Metallic}, m_Roughness{desc.Roughness}, m_EmissionPower{desc.EmissionPower}, m_AlbedoTexture{std::move(desc.AlbedoImage)} {}

  const std::string& GetName() const
  {
    return m_Name;
  }

  constexpr RGB GetAlbedo() const
  {
    return m_Albedo;
  }
  constexpr RGB GetAlbedo(int x, int y) const
  {
    if (m_AlbedoTexture.has_value())
    {
      return m_AlbedoTexture->Get(x, y);
    }

    return m_Albedo;
  }

  constexpr RGB GetRadiance() const
  {
    return m_EmissionColor * m_EmissionPower;
  }

  constexpr float GetRoughness() const
  {
    return m_Roughness;
  }
  constexpr float GetMetallic() const
  {
    return m_Metallic;
  }
  float GetSpecularProbability() const
  {
    const RGB f0 = glm::mix(RGB{0.04f}, GetAlbedo(), GetMetallic());
    const float base_probability = std::max(f0.x, std::max(f0.y, f0.z));
    const float roughness_influence = glm::smoothstep(0.0f, 1.0f, GetRoughness() * 0.7f);
    return glm::mix(base_probability, base_probability * 0.5f, roughness_influence);
  }

  constexpr RGB GetEmissionColor() const
  {
    return m_EmissionColor;
  }
  constexpr float GetEmissionPower() const
  {
    return m_EmissionPower;
  }

private:
  std::string m_Name;
  RGB m_Albedo, m_EmissionColor;
  float m_Metallic;
  float m_Roughness;
  float m_EmissionPower{0.f};

  std::optional<Image> m_AlbedoTexture{std::nullopt};
};
} // namespace VI
