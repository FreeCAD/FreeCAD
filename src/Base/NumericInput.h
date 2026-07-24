// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <string>
#include <string_view>

#include <FCGlobal.h>

namespace Base
{
struct NumericFormattingState;

/** Select the grammar used when canonicalizing localized numeric input. */
enum class NumericInputMode
{
    Expression,
    Quantity
};

/**
 * Convert localized numeric tokens to the canonical syntax understood by the existing parsers.
 *
 * Canonical expression and quantity parsers remain unchanged; this front end is intended for
 * text originating at a user-input boundary.
 */
BaseExport std::string canonicalizeNumericInput(
    std::string_view text,
    const NumericFormattingState& formatting,
    NumericInputMode mode
);

}  // namespace Base
