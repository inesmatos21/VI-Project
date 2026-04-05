#pragma once

#include "Mesh.hpp"
#include "Sphere.hpp"
#include "Triangle.hpp"

namespace VI
{

struct Ray;
struct Intersection;

using Geometry = std::variant<Sphere, Triangle, Mesh>;

bool Intersect(const Geometry& geom, const Ray& ray, Intersection& intersection);

BoundingBox GetBoundingBox(const Geometry& geom);

} // namespace VI
