#include "Math/DiscreteDistribution.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <vector>

namespace VI
{

std::vector<float> NormalizeWeightsToPDF(const std::vector<float>& weights)
{
  std::vector<float> pdf(weights.size(), 0.f);
  float total_weight = 0.f;

  for (size_t i = 0; i < weights.size(); ++i)
  {
    pdf[i] = std::max(weights[i], 0.f);
    total_weight += pdf[i];
  }

  if (total_weight <= 0.f)
  {
    return {};
  }

  for (float& probability : pdf)
  {
    probability /= total_weight;
  }

  return pdf;
}

std::vector<float> BuildCDF(const std::vector<float>& pdf)
{
  if (pdf.empty())
  {
    return {};
  }

  std::vector<float> cdf(pdf.size(), 0.f);
  float cumulative = 0.f;

  for (size_t i = 0; i < pdf.size(); ++i)
  {
    cumulative += std::max(pdf[i], 0.f);
    cdf[i] = cumulative;
  }

  if (cumulative <= 0.f)
  {
    return {};
  }

  for (float& value : cdf)
  {
    value /= cumulative;
  }
  cdf.back() = 1.f;

  return cdf;
}

int SampleCDFIndex(const std::vector<float>& cdf, float u)
{
  if (cdf.empty())
  {
    return -1;
  }

  const float sample = std::clamp(u, 0.f, std::nextafter(1.f, 0.f));
  const auto it = std::lower_bound(cdf.begin(), cdf.end(), sample);
  if (it == cdf.end())
  {
    return static_cast<int>(cdf.size()) - 1;
  }

  return static_cast<int>(std::distance(cdf.begin(), it));
}

float GetPDFValue(const std::vector<float>& pdf, int index)
{
  if (index < 0 || static_cast<size_t>(index) >= pdf.size())
  {
    return 0.f;
  }

  return pdf[index];
}

} // namespace VI
