// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "BaseClass.h"

#include <optional>
#include <string_view>

namespace Base
{
class Quantity;

/** Data-driven lookup for unit symbols, aliases, and allowed SI prefixes. */
class BaseExport UnitRegistry
{
public:
    static std::optional<Quantity> lookup(std::string_view symbol);
    static Quantity require(std::string_view symbol);
};
}  // namespace Base
