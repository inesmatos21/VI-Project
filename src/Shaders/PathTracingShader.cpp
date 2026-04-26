#include "Shaders/PathTracingShader.hpp"

#include "Math/Math.hpp"
#include "Math/RGB.hpp"
#include "Math/Random.hpp"
#include "Math/Vector.hpp"
#include "Primitive/BRDF.hpp"
#include "Primitive/Material.hpp"
#include "Primitive/Primitive.hpp"
#include "Ray/Intersection.hpp"
#include "Ray/Ray.hpp"
#include "Scene/Scene.hpp"
#include "Shaders/DirectIllumination.hpp"

#include <glm/common.hpp>
#include <glm/exponential.hpp>
#include <glm/geometric.hpp>

#include <algorithm>

// Balance heuristic (beta=1) for two strategies with one sample each
float BalanceHeuristic(float pdf_a, float p_s_a, float pdf_b, float p_s_b)
{
    const float pa = pdf_a * p_s_a;
    const float pb = pdf_b * p_s_b;
  const float denom = pa + pb;
  if (denom <= 0.f)
    return 0.f;
  return pa / denom;
}

// Power heuristic (beta=2) for two strategies with one sample each
float PowerHeuristic(float pdf_a, float p_s_a, float pdf_b, float p_s_b)
{
    const float pa2 = pdf_a * p_s_a * pdf_a * p_s_a;
    const float pb2 = pdf_b * p_s_b * pdf_b * p_s_b;
  const float denom = pa2 + pb2;
  if (denom <= 0.f)
    return 0.f;
  return pa2 / denom;
}

namespace VI
{
constexpr float MAX_DEPTH = 5;
constexpr int RUSSIAN_ROULETTE_DEPTH = 2;

RGB PathTracingShader::Execute(const Ray& ray, const Scene& scene) const
{
  Intersection intersection{};
  if (!scene.Trace(ray, intersection))
  {
    return m_BackgroundColor;
  }

  return DoExecute(ray, scene, intersection);
}

RGB PathTracingShader::DoExecute(const Ray& ray, const Scene& scene, const Intersection& intersection, int depth, bool allow_emissive) const
{
  RGB color{0.0f};
  if (depth > MAX_DEPTH)
  {
    return color;
  }

  const Primitive& primitive = scene.GetPrimitive(intersection.ObjectIndex);
  const Material& material = scene.GetMaterial(primitive.MaterialIndex);

  if (material.GetEmissionPower() > 0.0f)
  {
    return allow_emissive ? material.GetRadiance() : RGB{0.0f};
  }

  color += IndirectIllumination(ray, scene, intersection, material, depth, allow_emissive);

  color += DirectIllumination(ray, scene, intersection);

  return color;
}

RGB PathTracingShader::DirectIllumination(const Ray& ray, const Scene& scene, const Intersection& intersection) const
{
  const auto& object = scene.GetPrimitive(intersection.ObjectIndex);
  const auto& material = scene.GetMaterial(object.MaterialIndex);

  return SampleDirectIllumination(ray, scene, intersection, material, m_DirectIlluminationMode);
}

RGB PathTracingShader::IndirectIllumination(const Ray& ray, const Scene& scene, const Intersection& intersection, const Material& material, int depth, bool allow_emissive [[maybe_unused]]) const
{
    const bool do_MIS = true;
  const Vector shading_normal = FaceForward(intersection.Normal, -ray.Direction);
  const OrthonormalBasis basis{shading_normal};
  const Vector wo_local = basis.WorldToLocal(-ray.Direction);
  if (wo_local.z <= 0.f)
  {
    return RGB{0.0f};
  }

  MfacetLambertBRDF microfacetBRDF{};
  // the probability of selecting the specular (GGX) path is BRDF dependent
  const float microfacet_probability = material.GetSpecularProbability();

  // stochastically select whether to sample the direction according to specular (microfacet) or diffuse (lambertian)
  const bool sample_microfacet = Random::RandomFloat(0.f, 1.f) < microfacet_probability;
    
  // sample the direction according to the selected BRDF mode
  const Vector wi_local = microfacetBRDF.Sample(
            wo_local, material,
            sample_microfacet ? MODE::GGX_MODE : MODE::LAMBERT_MODE);
  if (wi_local.z <= 0.f)    return RGB{0.0f};

  // get the BRDF value
  RGB const f = microfacetBRDF.Evaluate(wo_local, wi_local, material);

  // get the 2 probabilities with which the direction
  // could have been sampled
  const float diffuse_pdf = microfacetBRDF.PDF(wo_local, wi_local, material, MODE::LAMBERT_MODE);
  const float microfacet_pdf = microfacetBRDF.PDF(wo_local, wi_local, material, MODE::GGX_MODE);
  // this is the actual probability with which it was sampled
  float pdf = (sample_microfacet ? microfacet_pdf : diffuse_pdf);
  if (pdf <= 0.f)    return RGB{0.0f};
  // include the probability with which the sampling MODE was selected
    
  // NOTE: THat probability shall not be included
  // the BRDF evaluation includes both the GGX and the Lambert lobes
  //pdf *= (sample_microfacet ? microfacet_probability : (1.-microfacet_probability));
    
  // get MIS weight
  const float w_mis = (sample_microfacet ? PowerHeuristic(microfacet_pdf, microfacet_probability, diffuse_pdf, (1.f - microfacet_probability)) : PowerHeuristic(diffuse_pdf, (1.f - microfacet_probability), microfacet_pdf, microfacet_probability));

  const float cos_theta = wi_local.z;
  const RGB throughput = ((do_MIS ? w_mis : 1.f) * f * cos_theta) / pdf;
    
  // Russian Roulette
  float continuation_probability = 1.0f;
  if (depth >= RUSSIAN_ROULETTE_DEPTH)
  {
    continuation_probability = glm::clamp(std::max(throughput.x, std::max(throughput.y, throughput.z)), 0.05f, 0.95f);
    if (Random::RandomFloat(0.f, 1.f) >= continuation_probability)      return RGB{0.0f};
  }

  // follow up ray
  const Vector wi_world = glm::normalize(basis.LocalToWorld(wi_local));
  const Ray scattered_ray = Ray::WithOffset(intersection.Position, wi_world, shading_normal);

  Intersection scattered_intersection{};
  RGB incoming_radiance = m_BackgroundColor;
  if (scene.Trace(scattered_ray, scattered_intersection))
  {
    // if intersected get radiance
    const bool next_allow_emissive = sample_microfacet;
    incoming_radiance = DoExecute(scattered_ray, scene, scattered_intersection, depth + 1, next_allow_emissive);
  }

  return throughput * incoming_radiance / continuation_probability;
}

} // namespace VI
