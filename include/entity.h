#pragma once

#include <limits>

#include "basicTypes.h"

namespace Hydrogen
{
    struct Entity
    {
        uint32 id = std::numeric_limits<uint32>::max();
        bool IsValid() const { return id != std::numeric_limits<uint32>::max(); }
        bool operator==(const Entity&) const = default;
    };
}
