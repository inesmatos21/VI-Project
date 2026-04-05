#pragma once

#include "Math/RGB.hpp"
#include "Math/Vector.hpp"

namespace VI
{

class Material;

class BRDF
{
public:
  virtual ~BRDF() = default;

  virtual RGB Sample(const Vector& wo_local, const Material& material) const = 0;
  virtual Vector Evaluate(const Vector& wo_local, const Vector& wi_local, const Material& material) const = 0;
  virtual float PDF(const Vector& wo_local, const Vector& wi_local, const Material& material) const = 0;
};

class LambertianBRDF final : public BRDF
{
public:
  RGB Sample(const Vector& wo_local, const Material& material) const override;
  Vector Evaluate(const Vector& wo_local, const Vector& wi_local, const Material& material) const override;
  float PDF(const Vector& wo_local, const Vector& wi_local, const Material& material) const override;
};

class MicrofacetBRDF final : public BRDF
{
public:
  RGB Sample(const Vector& wo_local, const Material& material) const override;
  Vector Evaluate(const Vector& wo_local, const Vector& wi_local, const Material& material) const override;
  float PDF(const Vector& wo_local, const Vector& wi_local, const Material& material) const override;

private:
  float G1_Smith(float NoX, float a) const;
  float D_GGX(float NoH, float alpha) const;
  Vector SampleGGX_VNDF(const Vector& wo, float a) const;
  RGB Fresnel_Schlick(float cos_theta, const RGB& F0) const;
  float G_Smith(float NoV, float NoL, float alpha) const;
  Vector SampleGGX(float alpha) const;
};

} // namespace VI
