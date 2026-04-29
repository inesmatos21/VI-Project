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

namespace VI
{

constexpr float MIN_ROUGHNESS = 0.02f;
constexpr float EPS_COS = 1e-4f;
constexpr float EPS_VOH = 1e-4f;
constexpr float EPS_PDF = 1e-6f;

Vector LambertianBRDF::Sample(const Vector& wo_local [[maybe_unused]], const Material& material [[maybe_unused]]) const
{
  // Lambertian reflection models a perfectly diffuse surface. Its outgoing
  // energy is proportional to cos(theta), so cosine-weighted hemisphere
  // sampling generates directions that match the BRDF shape well.
  float u1 = Random::RandomFloat(0, 1);
  float u2 = Random::RandomFloat(0, 1);

  float r = glm::sqrt(u1);
  float theta = 2.0f * glm::pi<float>() * u2;

  float x = r * glm::cos(theta);
  float y = r * glm::sin(theta);
  float z = glm::sqrt(1.0f - u1);

  return Vector{x, y, z};
}

RGB LambertianBRDF::Evaluate(const Vector& wo_local [[maybe_unused]], const Vector& wi_local [[maybe_unused]], const Material& material) const
{
  // Diffuse BRDF: albedo / pi. It does not depend on the viewing direction or
  // light direction, only on the material color.
  return material.GetAlbedo() / glm::pi<float>();
}

float LambertianBRDF::PDF(const Vector& wo_local [[maybe_unused]], const Vector& wi_local, const Material& material [[maybe_unused]]) const
{
  if (wi_local.z <= 0.f)
  {
    return 0.f;
  }

  return wi_local.z / glm::pi<float>();
}

Vector MicrofacetBRDF::Sample(const Vector& wo_local, const Material& material) const
{
  if (wo_local.z <= 0.0f)
    return Vector{0.0f};

  // GGX is a microfacet model: it assumes the surface is made of tiny mirror
  // facets whose normals follow a statistical distribution. Roughness controls
  // the width of that distribution. Lower roughness produces a tighter,
  // mirror-like lobe; higher roughness produces a wider glossy lobe.
  float roughness = glm::max(material.GetRoughness(), MIN_ROUGHNESS);
  float a = roughness * roughness;

  // Sample a half-vector h from the GGX normal distribution. The reflected
  // direction is then produced by mirroring -wo around h.
  Vector random{Random::RandomFloat(0.0f, 1.0f), Random::RandomFloat(0.0f, 1.0f), 0.f};
  float phi = 2.0f * glm::pi<float>() * random.x;

  float cos_theta = glm::sqrt((1.0f - random.y) / (1.0f + (a * a - 1.0f) * random.y));
  float sin_theta = glm::sqrt((1.0f - cos_theta * cos_theta));

  Vector local_h{sin_theta * glm::cos(phi), sin_theta * glm::sin(phi), cos_theta};
  Vector wi_local = glm::reflect(-wo_local, local_h);

  if (wi_local.z <= 0.0f)
    return Vector{0.0f};

  return wi_local;
}

RGB MicrofacetBRDF::Evaluate(const Vector& wo_local, const Vector& wi_local, const Material& material) const
{
  float NoV = wo_local.z;
  float NoL = wi_local.z;

  if (NoV <= 0.0f || NoL <= 0.0f)
    return RGB{0.0f};

  Vector h = glm::normalize(wo_local + wi_local);
  float NoH = glm::max(h.z, 0.0f);
  float VoH = glm::max(glm::dot(wo_local, h), 0.0f);

  float roughness = glm::max(material.GetRoughness(), MIN_ROUGHNESS);
  float a = roughness * roughness;

  // Cook-Torrance microfacet BRDF:
  // D: how many microfacets are aligned with the half-vector
  // G: how much masking/shadowing happens between microfacets
  // F: Fresnel reflectance, which increases at grazing angles
  float D = D_GGX(NoH, a);
  float G = G_Smith(NoV, NoL, a);

  // F0 is the reflectance at normal incidence. Dielectrics start near 4%;
  // metals use their base color as the specular reflectance.
  RGB F0 = glm::mix(RGB{0.04f}, material.GetAlbedo(), material.GetMetallic());
  RGB F = Fresnel_Schlick(VoH, F0);

  return (D * G * F) / (4.0f * NoV * NoL);
}

float MicrofacetBRDF::PDF(const Vector& wo_local, const Vector& wi_local, const Material& material) const
{
  if (wo_local.z <= 0.0f || wi_local.z <= 0.0f)
    return 0.0f;

  Vector h = glm::normalize(wo_local + wi_local);

  float nh = glm::max(h.z, EPS_COS);
  float voh = glm::max(glm::dot(wo_local, h), EPS_VOH);
  // The GGX distribution is sampled in half-vector space. The 1/(4*VoH)
  // factor converts the half-vector PDF into a reflected-direction PDF.
  float D = D_GGX(nh, material.GetRoughness());

  return glm::max(D * nh / (4.0f * voh), EPS_PDF);
}

float MicrofacetBRDF::G_Smith(float NoV, float NoL, float roughness) const
{
  // Smith geometry term approximates how many microfacets are visible from
  // both the view direction and the light direction.
  float a = glm::max(roughness, MIN_ROUGHNESS);
  float k = a * 0.5;
  float nv = glm::clamp(NoV, EPS_COS, 1.0f);
  float nl = glm::clamp(NoL, EPS_COS, 1.0f);
  float G1V = nv / (nv * (1.0f - k) + k);
  float G1L = nl / (nl * (1.0 - k) + k);
  return G1V * G1L;
}

RGB MicrofacetBRDF::Fresnel_Schlick(float cosTheta, const RGB& F0) const
{
  // Schlick's approximation is a cheap Fresnel curve: reflectance is F0 when
  // looking straight on and approaches 1 near grazing angles.
  return F0 + (RGB{1.0f} - F0) * glm::pow(1.0f - cosTheta, 5.0f);
}

float MicrofacetBRDF::D_GGX(float NoH, float roughness) const
{
  // GGX/Trowbridge-Reitz normal distribution. This controls the shape of the
  // specular highlight. Compared with older distributions, GGX has longer
  // tails, which keeps rough specular reflections visibly broader.
  float a = glm::max(roughness, MIN_ROUGHNESS);
  float a2 = a * a;
  float nh = glm::clamp(NoH, 0.0f, 1.0f);
  float denom = nh * nh * (a2 - 1.0f) + 1.0f;
  return a2 / (glm::pi<float>() * denom * denom);
}


Vector MfacetLambertBRDF::Sample(const Vector& wo_local [[maybe_unused]], const Material& material [[maybe_unused]]) const
{
    switch (mode) {
        case MODE::LAMBERT_MODE:
            return lambertBRDF.Sample(wo_local,  material);
            break;
        case MODE::GGX_MODE:
            return microBRDF.Sample(wo_local,  material);
            break;
    }
}


Vector MfacetLambertBRDF::Sample(const Vector& wo_local [[maybe_unused]], const Material& material [[maybe_unused]], const MODE _mode)
{
    MODE save_mode = mode;
    mode = _mode;
    const Vector ret_vec = Sample(wo_local,material);
    mode = save_mode;
    return ret_vec;
}

RGB MfacetLambertBRDF::Evaluate(const Vector& wo_local [[maybe_unused]], const Vector& wi_local [[maybe_unused]], const Material& material) const
{
  // The material decides how much of the BRDF value is diffuse vs specular.
  // This is separate from the path tracer's sampling probability, although in
  // practice matching them often reduces noise.
  const float specular_weight = material.GetSpecularProbability();
  const float diffuse_weight = 1.0f - specular_weight;
  return diffuse_weight * lambertBRDF.Evaluate(wo_local, wi_local, material) + specular_weight * microBRDF.Evaluate(wo_local, wi_local, material);
}

float MfacetLambertBRDF::PDF(const Vector& wo_local [[maybe_unused]], const Vector& wi_local, const Material& material [[maybe_unused]]) const
{
    switch (mode) {
        case MODE::LAMBERT_MODE:
            return lambertBRDF.PDF(wo_local, wi_local, material);
            break;
        case MODE::GGX_MODE:
            return microBRDF.PDF(wo_local, wi_local, material);
            break;
    }
}

float MfacetLambertBRDF::PDF(const Vector& wo_local [[maybe_unused]], const Vector& wi_local, const Material& material [[maybe_unused]], const MODE _mode)
{
    MODE save_mode = mode;
    mode = _mode;
    float const ret = PDF(wo_local, wi_local, material);
    mode = save_mode;
    return ret;
}

} // namespace VI
