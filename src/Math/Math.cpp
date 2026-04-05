#include "Math/Math.hpp"

#include <glm/common.hpp>
#include <glm/geometric.hpp>

#include "Math/Vector.hpp"

namespace VI
{
OrthonormalBasis::OrthonormalBasis(const Vector& n) : m_Normal{glm::normalize(n)}
{
  if (glm::abs(m_Normal.x) > glm::abs(m_Normal.z))
  {
    m_Tangent = glm::normalize(Vector{-m_Normal.y, m_Normal.x, 0.f});
  }
  else
  {
    m_Tangent = glm::normalize(Vector{0.f, -m_Normal.z, m_Normal.y});
  }

  m_Bitangent = glm::cross(m_Normal, m_Tangent);
}

Vector OrthonormalBasis::WorldToLocal(const Vector& v) const
{
  return {
      glm::dot(v, m_Tangent),
      glm::dot(v, m_Bitangent),
      glm::dot(v, m_Normal),
  };
}

Vector OrthonormalBasis::LocalToWorld(const Vector& v) const
{
  return v.x * m_Tangent + v.y * m_Bitangent + v.z * m_Normal;
}
} // namespace VI
