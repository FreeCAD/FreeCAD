// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <string>
#include <string_view>

#include <FCGlobal.h>

namespace Base
{
struct QuantityFormat;

/**
 * Numeric locale information shared by Base, App, and GUI formatting/parsing boundaries.
 *
 * The snapshot keeps the locale identifier and its effective separators together so consumers
 * cannot combine values obtained from different locale configurations.
 */
struct BaseExport NumericFormattingState
{
    /// ICU/BCP-47-style locale identifier used to select locale-specific numeric rules.
    std::string localeId;
    /// Effective decimal separator, possibly overridden by the GUI's QLocale.
    std::string decimalSeparator;
    /// Effective grouping separator, possibly overridden by the GUI's QLocale.
    std::string groupingSeparator;

    bool operator==(const NumericFormattingState&) const = default;
};

/// Format a scalar value using a quantity format and an explicit numeric-locale snapshot.
/// @throws ValueError if the snapshot contains an invalid non-C locale identifier.
BaseExport std::string formatNumericValue(
    double value,
    const QuantityFormat& format,
    const NumericFormattingState& formatting
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
BaseExport NumericFormattingState createNumericFormattingState(std::string_view localeId);
/// Build a numeric formatting snapshot from ICU's current default locale.
BaseExport NumericFormattingState createNumericFormattingState();
/// Publish a complete snapshot for subsequent formatting and user-input parsing.
BaseExport void publishNumericFormattingState(NumericFormattingState state);
/// Return a thread-safe copy of the currently published snapshot.
BaseExport NumericFormattingState currentNumericFormattingState();

}  // namespace Base
