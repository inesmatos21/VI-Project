#pragma once

#include <cstddef>
#include <memory_resource>

namespace VI
{
using Allocator = std::pmr::polymorphic_allocator<std::byte>;
}
