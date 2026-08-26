// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <string>

namespace CadX
{

class GraphSnapshot;

std::string canonicalSemantic(const GraphSnapshot& snapshot);
std::string canonicalPresentation(const GraphSnapshot& snapshot);
std::string sha256Revision(const std::string& canonical);

}  // namespace CadX
