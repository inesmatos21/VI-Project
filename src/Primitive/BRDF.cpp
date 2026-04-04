#include "Primitive/BRDF.hpp"

#include "Math/RGB.hpp"
#include "Math/Random.hpp"
#include "Math/Vector.hpp"
#include "Primitive/Material.hpp"
#include "glm/common.hpp"
#include "glm/exponential.hpp"
#include "glm/ext/scalar_constants.hpp"
#include "glm/geometric.hpp"
#include "glm/trigonometric.hpp"

#include <glm/glm.hpp>

namespace VI {

constexpr float MIN_ROUGHNESS = 0.02f;
constexpr float EPS_COS = 1e-4f;
constexpr float EPS_VOH = 1e-4f;
constexpr float EPS_PDF = 1e-6f;

Vector LambertianBRDF::Sample(const Vector &wo_local [[maybe_unused]],
                              const Material &material [[maybe_unused]]) const {
  float u1 = Random::RandomFloat(0, 1);
  float u2 = Random::RandomFloat(0, 1);

  float r = glm::sqrt(u1);
  float theta = 2.0f * glm::pi<float>() * u2;

  float x = r * glm::cos(theta);
  float y = r * glm::sin(theta);
  float z = glm::sqrt(1.0f - u1);

  return Vector{x, y, z};
}

RGB LambertianBRDF::Evaluate(const Vector &wo_local [[maybe_unused]],
                             const Vector &wi_local [[maybe_unused]],
                             const Material &material) const {

  return material.GetAlbedo() / glm::pi<float>();
}

float LambertianBRDF::PDF(const Vector &wo_local [[maybe_unused]],
                          const Vector &wi_local,
                          const Material &material [[maybe_unused]]) const {
  if (wi_local.z <= 0.f) {
    return 0.f;
  }

  return wi_local.z / glm::pi<float>();
}

Vector MicrofacetBRDF::Sample(const Vector &wo_local,
                              const Material &material) const {
  if (wo_local.z <= 0.0f)
    return Vector{0.0f};

  float roughness = glm::max(material.GetRoughness(), MIN_ROUGHNESS);
  float a = roughness * roughness;

  Vector random{Random::RandomFloat(0.0f, 1.0f),
                Random::RandomFloat(0.0f, 1.0f), 0.f};
  float phi = 2.0f * glm::pi<float>() * random.x;

  float cos_theta =
      glm::sqrt((1.0f - random.y) / (1.0f + (a * a - 1.0f) * random.y));
  float sin_theta = glm::sqrt((1.0f - cos_theta * cos_theta));

  Vector local_h{sin_theta * glm::cos(phi), sin_theta * glm::sin(phi),
                 cos_theta};
  Vector wi_local = glm::reflect(-wo_local, local_h);

  if (wi_local.z <= 0.0f)
    return Vector{0.0f};

  return wi_local;
}

RGB MicrofacetBRDF::Evaluate(const Vector &wo_local, const Vector &wi_local,
                             const Material &material) const {
  float NoV = wo_local.z;
  float NoL = wi_local.z;

  if (NoV <= 0.0f || NoL <= 0.0f)
    return RGB{0.0f};

  Vector h = glm::normalize(wo_local + wi_local);
  float NoH = glm::max(h.z, 0.0f);
  float VoH = glm::max(glm::dot(wo_local, h), 0.0f);

  float roughness = glm::max(material.GetRoughness(), MIN_ROUGHNESS);
  float a = roughness * roughness;

  float D = D_GGX(NoH, a);
  float G = G_Smith(NoV, NoL, a);

  RGB F0 = glm::mix(RGB{0.04f}, material.GetAlbedo(), material.GetMetallic());
  RGB F = Fresnel_Schlick(VoH, F0);

  return (D * G * F) / (4.0f * NoV * NoL);
}

float MicrofacetBRDF::PDF(const Vector &wo_local, const Vector &wi_local,
                          const Material &material) const {
  if (wo_local.z <= 0.0f || wi_local.z <= 0.0f)
    return 0.0f;

  Vector h = glm::normalize(wo_local + wi_local);

  float nh = glm::max(h.z, EPS_COS);
  float voh = glm::max(glm::dot(wo_local, h), EPS_VOH);
  float D = D_GGX(nh, material.GetRoughness());

  return glm::max(D * nh / (4.0f * voh), EPS_PDF);
}

float MicrofacetBRDF::G_Smith(float NoV, float NoL, float roughness) const {
  float a = glm::max(roughness, MIN_ROUGHNESS);
  float k = a * 0.5;
  float nv = glm::clamp(NoV, EPS_COS, 1.0f);
  float nl = glm::clamp(NoL, EPS_COS, 1.0f);
  float G1V = nv / (nv * (1.0f - k) + k);
  float G1L = nl / (nl * (1.0 - k) + k);
  return G1V * G1L;
}

RGB MicrofacetBRDF::Fresnel_Schlick(float cosTheta, const RGB &F0) const {
  return F0 + (RGB{1.0f} - F0) * glm::pow(1.0f - cosTheta, 5.0f);
}

float MicrofacetBRDF::D_GGX(float NoH, float roughness) const {
  float a = glm::max(roughness, MIN_ROUGHNESS);
  float a2 = a * a;
  float nh = glm::clamp(NoH, 0.0f, 1.0f);
  float denom = nh * nh * (a2 - 1.0f) + 1.0f;
  return a2 / (glm::pi<float>() * denom * denom);
}

} // namespace VI
