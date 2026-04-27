#pragma once

#include "Image/Image.hpp"
#include "Math/RGB.hpp"
#include "Math/Vector.hpp"

#include <algorithm>
#include <cmath>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <glm/common.hpp>

namespace VI
{

enum class TextureWrapMode
{
  Repeat,
  ClampToEdge,
  MirroredRepeat,
};

enum class TextureFilterMode
{
  Nearest,
  Linear,
};

struct TextureSampler
{
  Image Data;
  TextureWrapMode WrapS{TextureWrapMode::Repeat};
  TextureWrapMode WrapT{TextureWrapMode::Repeat};
  TextureFilterMode Filter{TextureFilterMode::Linear};

  RGB Sample(const Vec2& uv) const
  {
    const float u = ApplyWrap(uv.x, WrapS);
    const float v = ApplyWrap(uv.y, WrapT);

    if (Filter == TextureFilterMode::Nearest)
    {
      return SampleNearest(u, v);
    }

    return SampleLinear(u, v);
  }

private:
  static float ApplyWrap(float value, TextureWrapMode mode)
  {
    switch (mode)
    {
      case TextureWrapMode::ClampToEdge:
        return glm::clamp(value, 0.0f, 1.0f);
      case TextureWrapMode::MirroredRepeat:
      {
        const float wrapped = value - std::floor(value);
        const auto period = static_cast<int>(std::floor(value));
        return period % 2 == 0 ? wrapped : 1.0f - wrapped;
      }
      case TextureWrapMode::Repeat:
        return value - std::floor(value);
    }

    return value;
  }

  RGB SampleNearest(float u, float v) const
  {
    const int x = std::min(static_cast<int>(u * Data.GetWidth()), Data.GetWidth() - 1);
    const int y = std::min(static_cast<int>(v * Data.GetHeight()), Data.GetHeight() - 1);
    return Data.Get(x, y);
  }

  RGB SampleLinear(float u, float v) const
  {
    const float x = u * static_cast<float>(Data.GetWidth() - 1);
    const float y = v * static_cast<float>(Data.GetHeight() - 1);

    const int x0 = static_cast<int>(std::floor(x));
    const int y0 = static_cast<int>(std::floor(y));
    const int x1 = std::min(x0 + 1, Data.GetWidth() - 1);
    const int y1 = std::min(y0 + 1, Data.GetHeight() - 1);

    const float tx = x - static_cast<float>(x0);
    const float ty = y - static_cast<float>(y0);

    const RGB c00 = Data.Get(x0, y0);
    const RGB c10 = Data.Get(x1, y0);
    const RGB c01 = Data.Get(x0, y1);
    const RGB c11 = Data.Get(x1, y1);

    return glm::mix(glm::mix(c00, c10, tx), glm::mix(c01, c11, tx), ty);
  }
};

struct MaterialDesc
{
  std::string_view Name;
  RGB Albedo{};
  float Roughness = 1.f;
  float Metallic = 0.f;
  RGB EmissionColor{};
  float EmissionPower = 0.f;
  std::optional<TextureSampler> AlbedoTexture = std::nullopt;
};

class Material
{
public:
  Material(MaterialDesc desc) : m_Name{desc.Name}, m_Albedo{desc.Albedo}, m_EmissionColor{desc.EmissionColor}, m_Metallic{desc.Metallic}, m_Roughness{desc.Roughness}, m_EmissionPower{desc.EmissionPower}, m_AlbedoTexture{std::move(desc.AlbedoTexture)} {}

  const std::string& GetName() const
  {
    return m_Name;
  }

  constexpr RGB GetAlbedo() const
  {
    return m_Albedo;
  }

  RGB GetAlbedo(const Vec2& uv) const
  {
    if (m_AlbedoTexture.has_value())
    {
      return m_Albedo * m_AlbedoTexture->Sample(uv);
    }

    return m_Albedo;
  }

  bool HasAlbedoTexture() const
  {
    return m_AlbedoTexture.has_value();
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
    // This value is a good target for the path tracer's probability of choosing
    // the GGX/specular sampling strategy instead of the Lambert/diffuse one.
    //
    // Useful intuition for students:
    // - high metallic materials should sample GGX more often
    // - low roughness materials should sample GGX more often
    // - very rough dielectrics should sample Lambert more often
    // - F0 estimates how reflective the material is at normal incidence
    //
    // A first exercise can use a fixed 0.5 sampling probability in the shader.
    // A better implementation can call this function so the sampling strategy
    // follows the material instead of treating every surface the same.
    const RGB f0 = glm::mix(RGB{0.04f}, GetAlbedo(), GetMetallic());
    const float base_probability = std::max(f0.x, std::max(f0.y, f0.z));
    const float roughness_influence = glm::smoothstep(0.0f, 1.0f, GetRoughness() * 0.7f);
    return glm::mix(base_probability, base_probability * 0.5f, roughness_influence);
  }

  float GetSpecularProbability(const Vec2& uv) const
  {
    const RGB f0 = glm::mix(RGB{0.04f}, GetAlbedo(uv), GetMetallic());
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

  std::optional<TextureSampler> m_AlbedoTexture{std::nullopt};
};
} // namespace VI
