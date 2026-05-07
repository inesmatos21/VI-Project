#pragma once

#include "Math/Vector.hpp"
#include "Ray/Ray.hpp"

#include <glm/common.hpp>

#include <algorithm>
#include <cmath>
#include <limits>

namespace VI
{
constexpr float MachineEpsilon = std::numeric_limits<float>::epsilon() * 0.5;

constexpr float gamma(int n)
{
  return (n * MachineEpsilon) / (1 - n * MachineEpsilon);
}

struct BoundingBox
{
  Point Min{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
  Point Max{std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};

  constexpr void Update(const Point& p)
  {
    Min.x = glm::min(Min.x, p.x);
    Min.y = glm::min(Min.y, p.y);
    Min.z = glm::min(Min.z, p.z);
    Max.x = glm::max(Max.x, p.x);
    Max.y = glm::max(Max.y, p.y);
    Max.z = glm::max(Max.z, p.z);
  }

  /*
   * I suggest you implement:
   *  bool intersect (Ray r) { }
   *
   * based on PBRT's 3rd ed. book , sec 3.1.2, pags 125..128 + 214,217,221
   *
   * or https://doi.org/10.1007/978-1-4842-7185-8_2
   *
   */
  bool Intersect(const Ray& ray, float& tmin, float& tmax) const
  {
    tmin = 0.0f;
    tmax = std::numeric_limits<float>::infinity();

    const auto intersect_axis = [&](float origin, float direction, float min, float max) {
      if (std::abs(direction) <= MachineEpsilon)
      {
        return origin >= min && origin <= max;
      }

      const float inv_direction = 1.0f / direction;
      float near_t = (min - origin) * inv_direction;
      float far_t = (max - origin) * inv_direction;
      if (near_t > far_t)
      {
        std::swap(near_t, far_t);
      }

      tmin = std::max(tmin, near_t);
      tmax = std::min(tmax, far_t);
      return tmin <= tmax;
    };

    return intersect_axis(ray.Origin.x, ray.Direction.x, Min.x, Max.x) &&
           intersect_axis(ray.Origin.y, ray.Direction.y, Min.y, Max.y) &&
           intersect_axis(ray.Origin.z, ray.Direction.z, Min.z, Max.z);
  }

  constexpr void Update(const BoundingBox& other)
  {
    Min.x = glm::min(Min.x, other.Min.x);
    Min.y = glm::min(Min.y, other.Min.y);
    Min.z = glm::min(Min.z, other.Min.z);
    Max.x = glm::max(Max.x, other.Max.x);
    Max.y = glm::max(Max.y, other.Max.y);
    Max.z = glm::max(Max.z, other.Max.z);
  }
};
} // namespace VI
