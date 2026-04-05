#pragma once

#include "Math/Vector.hpp"

namespace VI
{
enum class LightType
{
  Ambient,
  Point,
  Area,
};

#define LIGHT_TYPE_CLASS(type)                                                                                                                                                                                                                                                                             \
  LightType GetType() const noexcept override                                                                                                                                                                                                                                                              \
  {                                                                                                                                                                                                                                                                                                        \
    return LightType::type;                                                                                                                                                                                                                                                                                \
  }

class Light
{
public:
  explicit Light(int material_index) : m_MaterialIndex{material_index} {}
  Light(const Light&) = default;
  Light(Light&&) = default;
  Light& operator=(const Light&) = default;
  Light& operator=(Light&&) = default;
  virtual ~Light() noexcept = default;
  virtual LightType GetType() const noexcept = 0;

  int GetMaterialIndex() const noexcept
  {
    return m_MaterialIndex;
  }

private:
  int m_MaterialIndex;
};

class AmbientLight : public Light
{
public:
  explicit AmbientLight(int material_index) : Light{material_index} {}
  LIGHT_TYPE_CLASS(Ambient)
};

class PointLight : public Light
{
public:
  LIGHT_TYPE_CLASS(Point)

  PointLight(int material_index, Point position) : Light{material_index}, m_Position{position} {}

  Point GetPosition() const noexcept
  {
    return m_Position;
  }

private:
  Point m_Position;
};

class AreaLight : public Light
{
public:
  LIGHT_TYPE_CLASS(Area)

  AreaLight(int material_index, int object_index) : Light{material_index}, m_ObjectIndex{object_index} {}

  int GetObjectIndex() const noexcept
  {
    return m_ObjectIndex;
  }

private:
  int m_ObjectIndex;
};
} // namespace VI
