// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <string>
#include <string_view>

#include <FCGlobal.h>

namespace Base
{
struct QuantityFormat;

/**
 * Complete numeric-locale context shared by Base, App, and GUI boundaries.
 *
 * Instances are passed by const reference to parsers and formatters. Publication replaces the
 * entire value atomically, so a consumer can never observe a locale identifier paired with
 * separators, signs, or grouping rules from a different configuration.
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
    /// UTF-8 representation of the locale's zero digit. Decimal digits are consecutive from it.
    std::string zeroDigit;

    bool operator==(const NumericLocaleContext&) const = default;
};

/// Format a scalar value using a quantity format and an explicit numeric-locale snapshot.
/// @throws ValueError if the snapshot contains an invalid non-C locale identifier.
BaseExport std::string formatNumericValue(
    double value,
    const QuantityFormat& format,
    const NumericLocaleContext& formatting
);

/// Return whether a localeId names the C/POSIX locale.
BaseExport bool isCLocaleName(std::string_view localeId);
/// Return the normalized ICU locale identifier used for numeric formatting.
/// @throws ValueError if a non-C locale identifier is invalid.
BaseExport std::string normalizeIcuLocaleId(std::string_view localeId);
/// Set ICU's default locale using the normalized numeric locale identifier.
/// @throws ValueError for an invalid identifier, or RuntimeError if ICU rejects the update.
BaseExport void setIcuDefaultLocale(std::string_view localeId);

/// Build a numeric formatting snapshot for an explicit locale identifier.
/// @throws ValueError for an invalid identifier, or RuntimeError if ICU cannot load its symbols.
BaseExport NumericLocaleContext createNumericLocaleContext(std::string_view localeId);
/// Build a numeric formatting snapshot from ICU's current default locale.
BaseExport NumericLocaleContext createNumericLocaleContext();
/// Publish a complete snapshot for subsequent formatting and user-input parsing.
BaseExport void publishNumericLocaleContext(NumericLocaleContext state);
/// Return a thread-safe copy of the currently published snapshot.
BaseExport NumericLocaleContext currentNumericLocaleContext();

}  // namespace Base
