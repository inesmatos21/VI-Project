#pragma once

#include "Math/Math.hpp"
#include "Math/Vector.hpp"

namespace VI
{
struct Ray
{
  Point Origin;
  Vector Direction;

  static Ray WithOffset(const Point& origin, const Vector& direction, const Vector& normal, float epsilon = EPSILON)
  {
    return {.Origin = OffsetPoint(origin, normal, epsilon), .Direction = direction};
  }
};
} // namespace VI
