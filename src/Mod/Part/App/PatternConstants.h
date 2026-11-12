// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <App/PropertyStandard.h>
#include <Mod/Part/PartGlobal.h>

namespace Part::PatternConstants
{
// Limit automatically generated populations before allocating the occurrences.
inline constexpr int MaximumOccurrences = 10000;

/**
 * @brief Constraints for user-entered pattern occurrence counts.
 *
 * The upper bound is the @c MaximumPatternOccurrences parameter in <tt>Preferences/Mod/Part</tt>
 * (default 1000), read once per session, so users who need larger patterns can raise it.
 *
 * @return Constraints shared by all pattern occurrence properties.
 */
PartExport const App::PropertyIntegerConstraint::Constraints* occurrenceConstraints();
}  // namespace Part::PatternConstants
