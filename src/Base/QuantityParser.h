// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <string_view>

namespace Base
{
class Quantity;

namespace QuantityParserSupport
{
Quantity parse(std::string_view input);
}
}  // namespace Base
