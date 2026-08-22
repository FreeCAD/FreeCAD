// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <string>

#include <FCGlobal.h>

namespace Base
{
/**
 * Complete numeric-locale context shared by localized scanners and their callers.
 *
 * The context is immutable by convention and is passed explicitly to numeric input code. It
 * contains all symbols and grouping rules needed to recognize one localized numeric token.
 */
struct BaseExport NumericLocaleContext
{
    std::string localeId;
    std::string decimalSeparator;
    std::string groupingSeparator;
    std::string positiveSign;
    std::string negativeSign;
    int primaryGroupingSize {};
    int secondaryGroupingSize {};
    std::string zeroDigit;

    bool operator==(const NumericLocaleContext&) const = default;
};
}  // namespace Base
