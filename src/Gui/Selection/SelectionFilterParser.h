// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <memory>
#include <string>
#include <string_view>

namespace Gui
{
struct Node_Block;

std::shared_ptr<Node_Block> parseSelectionFilter(std::string_view input, std::string& error);
}  // namespace Gui
