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
#include "Primitive/Geometry/Geometry.hpp"
#include "Scene/Scene.hpp"
#include "Shaders/DirectIllumination.hpp"

#include <glm/common.hpp>
#include <glm/exponential.hpp>
#include <glm/geometric.hpp>

#include <algorithm>

// Balance heuristic (beta=1) for two strategies with one sample each
float BalanceHeuristic(float pdf_a, float pdf_b)
{
  const float denom = pdf_a + pdf_b;
  if (denom <= 0.f)
    return 0.f;
  return pdf_a / denom;
}

// Power heuristic (beta=2) for two strategies with one sample each
float PowerHeuristic(float pdf_a, float pdf_b)
{
    const float pa2 = pdf_a * pdf_a;
    const float pb2 = pdf_b * pdf_b;
  const float denom = pa2 + pb2;
  if (denom <= 0.f)
    return 0.f;
  return pa2 / denom;
}

namespace VI
{
constexpr float MAX_DEPTH = 5;
constexpr int RUSSIAN_ROULETTE_DEPTH = 2;
constexpr float MAX_SAMPLE_RADIANCE = 10.0f;

RGB ClampRadiance(const RGB& radiance)
{
  return glm::clamp(radiance, RGB{0.0f}, RGB{MAX_SAMPLE_RADIANCE});
}

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
    const RGB radiance = material.GetRadiance(intersection.TexCoord);
    if (std::max(radiance.x, std::max(radiance.y, radiance.z)) > 0.0f)
    {
      return allow_emissive ? radiance : RGB{0.0f};
    }
  }

  // Dielectric materials (glass/water) are handled separately:
  // they refract/reflect the ray using Snell's law instead of the BRDF.
  if (material.IsDielectric())
  {
    return DielectricScatter(ray, scene, intersection, material, depth);
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
  const Vector shading_normal = FaceForward(intersection.Normal, -ray.Direction);
  const OrthonormalBasis basis{shading_normal};
  const Vector wo_local = basis.WorldToLocal(-ray.Direction);
  if (wo_local.z <= 0.f)
  {
    return RGB{0.0f};
  }

  MfacetLambertBRDF microfacetBRDF{};
  // the probability of selecting the specular (GGX) path is BRDF dependent
  const float microfacet_probability = material.GetSpecularProbability(intersection.TexCoord);
  const float diffuse_probability = 1.0f - microfacet_probability;

  // stochastically select whether to sample the direction according to specular (microfacet) or diffuse (lambertian)
  const bool sample_microfacet = Random::RandomFloat(0.f, 1.f) < microfacet_probability;
    
  // sample the direction according to the selected BRDF mode
  const Vector wi_local = microfacetBRDF.Sample(
            wo_local, material,
            sample_microfacet ? MODE::GGX_MODE : MODE::LAMBERT_MODE,
            intersection.TexCoord);
  if (wi_local.z <= 0.f)    return RGB{0.0f};

  // get the BRDF value
  RGB const f = microfacetBRDF.Evaluate(wo_local, wi_local, material, intersection.TexCoord);

  // get the 2 probabilities with which the direction
  // could have been sampled
  const float diffuse_pdf = microfacetBRDF.PDF(wo_local, wi_local, material, MODE::LAMBERT_MODE, intersection.TexCoord);
  const float microfacet_pdf = microfacetBRDF.PDF(wo_local, wi_local, material, MODE::GGX_MODE, intersection.TexCoord);
  // this is the actual mixture probability with which this direction could
  // have been sampled by either BRDF branch.
  const float pdf = microfacet_probability * microfacet_pdf + diffuse_probability * diffuse_pdf;
  if (pdf <= 0.f)    return RGB{0.0f};
     
  const float cos_theta = wi_local.z;
  const RGB throughput = f * cos_theta / pdf;
    
  // Russian Roulette
  float continuation_probability = 1.0f;
  if (depth >= RUSSIAN_ROULETTE_DEPTH)
  {
    continuation_probability = glm::clamp(std::max(throughput.x, std::max(throughput.y, throughput.z)), 0.05f, 0.95f);
    if (Random::RandomFloat(0.f, 1.f) >= continuation_probability)      return RGB{0.0f};
  }

  // follow up ray
  const Vector wi_world = glm::normalize(basis.LocalToWorld(wi_local));
  // Propagate the original ray's time so that secondary rays see moving objects
  // at the same shutter instant as the primary ray (section 2.5 of the guide).
  const Ray scattered_ray = Ray::WithOffset(intersection.Position, wi_world, shading_normal, ray.Time);

  Intersection scattered_intersection{};
  RGB incoming_radiance = m_BackgroundColor;
  if (scene.Trace(scattered_ray, scattered_intersection))
  {
    // Direct light sampling handles non-primary emitter contributions. Keep
    // primary emitter visibility, but avoid randomly accepting light hits based
    // on which BRDF branch happened to generate the scattered ray.
    constexpr bool allow_emissive_on_indirect_hit = false;
    incoming_radiance = DoExecute(scattered_ray, scene, scattered_intersection, depth + 1, allow_emissive_on_indirect_hit);
  }

  return ClampRadiance(throughput * incoming_radiance / continuation_probability);
}

// ─────────────────────────────────────────────────────────────────────────────
// Dielectric scatter — follows the PDF (sections 11.2–11.4):
//   1. Compute eta ratio depending on whether we enter or exit the material
//   2. Check for Total Internal Reflection (Snell's law)
//   3. Use Schlick approximation to randomly choose reflect vs refract
//   4. Propagate ray.Time so moving dielectric objects respect motion blur
//
// Key implementation note: when a refracted ray travels inside a dielectric
// sphere, the GridAccelerationStructure may miss the exit surface because the
// ray origin is inside the grid cell. We therefore first test the dielectric
// primitive directly (guaranteed hit for the exit surface), and only fall back
// to the full scene Trace for the outgoing ray after it exits the glass.
// ─────────────────────────────────────────────────────────────────────────────

// Helper: refract a unit vector uv around normal n with eta ratio ri.
static Vector Refract(const Vector& uv, const Vector& n, float ri)
{
  const float cos_theta = std::fmin(glm::dot(-uv, n), 1.0f);
  const Vector r_perp = ri * (uv + cos_theta * n);
  const Vector r_para = -std::sqrt(std::fabs(1.0f - glm::dot(r_perp, r_perp))) * n;
  return r_perp + r_para;
}

// Helper: Schlick reflectance approximation.
static float Schlick(float cosine, float ri)
{
  float r0 = (1.0f - ri) / (1.0f + ri);
  r0 = r0 * r0;
  return r0 + (1.0f - r0) * std::pow(1.0f - cosine, 5.0f);
}

RGB PathTracingShader::DielectricScatter(const Ray& ray, const Scene& scene,
    const Intersection& intersection, const Material& material, int depth) const
{
  const float n = material.GetRefractionIndex();
  // ri = eta_i / eta_t
  const float ri = intersection.FrontFace ? (1.0f / n) : n;

  const Vector unit_dir = glm::normalize(ray.Direction);
  const float cos_theta = std::fmin(glm::dot(-unit_dir, intersection.Normal), 1.0f);
  const float sin_theta = std::sqrt(1.0f - cos_theta * cos_theta);

  const bool cannot_refract = ri * sin_theta > 1.0f;
  const bool do_reflect = cannot_refract || Schlick(cos_theta, n) > Random::RandomFloat();

  Vector direction;
  Vector offset_n;
  if (do_reflect)
  {
    direction = glm::reflect(unit_dir, intersection.Normal);
    offset_n  = intersection.Normal;
  }
  else
  {
    direction = Refract(unit_dir, intersection.Normal, ri);
    offset_n  = -intersection.Normal;
  }

  const Vector norm_dir = glm::normalize(direction);
  const Ray scattered{
      .Origin    = intersection.Position + 1e-4f * offset_n,
      .Direction = norm_dir,
      .Time      = ray.Time,
  };

  Intersection next{};
  
  RGB incoming = m_BackgroundColor;

  if (scene.Trace(scattered, next))
  {
      const Primitive& prim = scene.GetPrimitive(next.ObjectIndex);
      const Material& next_mat = scene.GetMaterial(prim.MaterialIndex);

      if (next_mat.IsDielectric())
          incoming = DielectricScatter(scattered, scene, next, next_mat, depth + 1);
      else
          incoming = DoExecute(scattered, scene, next, depth + 1, true);
  }

  // Fresnel weighting
  const float reflect_prob = Schlick(cos_theta, n);

  if (do_reflect)
      return reflect_prob * incoming;
  else
      return (1.0f - reflect_prob) * incoming;
  }

} // namespace VI