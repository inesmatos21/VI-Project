#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/detail/qualifier.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

template <typename OStream, glm::length_t L, typename T, glm::qualifier Q> inline OStream& operator<<(OStream& os, const glm::vec<L, T, Q>& vector)
{
  return os << glm::to_string(vector);
}

template <typename OStream, glm::length_t C, glm::length_t R, typename T, glm::qualifier Q> inline OStream& operator<<(OStream& os, const glm::mat<C, R, T, Q>& matrix)
{
  return os << glm::to_string(matrix);
}

template <typename OStream, typename T, glm::qualifier Q> inline OStream& operator<<(OStream& os, glm::qua<T, Q> quaternion)
{
  return os << glm::to_string(quaternion);
}
