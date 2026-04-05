#pragma once

#include <vector>

namespace VI
{

struct LightSamplingDistribution
{
  std::vector<int> LightIndices{};
  std::vector<float> PDF{};
  std::vector<float> CDF{};

  bool IsValid() const noexcept
  {
    return !LightIndices.empty() && PDF.size() == LightIndices.size() && CDF.size() == LightIndices.size();
  }
};

std::vector<float> NormalizeWeightsToPDF(const std::vector<float>& weights);
std::vector<float> BuildCDF(const std::vector<float>& pdf);
int SampleCDFIndex(const std::vector<float>& cdf, float u);
float GetPDFValue(const std::vector<float>& pdf, int index);

} // namespace VI
