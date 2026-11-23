// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <optional>
#include <string>
#include <string_view>

#include <FCGlobal.h>

namespace Base
{
struct NumericLocaleContext;

/** Syntactic position in which a localized number is being scanned. */
enum class NumericSyntaxContext
{
    /// A complete standalone quantity or number.
    Standalone,
    /// A numeric token embedded in an expression.
    Expression,
    /// A numeric token where the locale's function separator is structural punctuation.
    FunctionArgument
};

/** Effective separators and grouping rule for one numeric syntax context. */
struct BaseExport NumericGrammarPolicy
{
    std::string_view decimalSeparator;
    std::string_view groupingSeparator;
    std::string_view argumentSeparator;
    bool allowGrouping {true};
};

enum class NumericDiagnosticKind
{
    ExpectedDigit,
    IncompleteSign,
    IncompleteDecimal,
    IncompleteGrouping,
    IncompleteExponent,
    InvalidGrouping,
    UnexpectedSeparator,
    InvalidLiteral,
    OutOfRange
};

struct BaseExport NumericDiagnostic
{
    NumericDiagnosticKind kind {NumericDiagnosticKind::InvalidLiteral};
    /// Offset of the diagnostic range in the original UTF-8 input.
    std::size_t offsetBytes {};
    /// Length of the diagnostic range in UTF-8 bytes.
    std::size_t lengthBytes {};
};

/** Result of scanning one localized numeric token. */
struct BaseExport LocalizedNumberResult
{
    enum class Status
    {
        Complete,
        Incomplete,
        Invalid
    };

    Status status {Status::Invalid};
    /// Parsed value; meaningful only when status is Complete.
    double value {};
    /// Locale-independent spelling of a complete token.
    std::string canonicalText;
    /// Number of bytes belonging to the token in the original UTF-8 input.
    std::size_t consumedBytes {};
    /// Typed error and UTF-8 source range for incomplete or invalid input.
    std::optional<NumericDiagnostic> diagnostic;
};

/** Return the separator policy used by the localized number scanner. */
BaseExport NumericGrammarPolicy
numericGrammarPolicy(const NumericLocaleContext& locale, NumericSyntaxContext syntax);

/** Return the localized decimal digit at @p position, and its UTF-8 width. */
BaseExport bool localizedDigitAt(
    std::string_view input,
    std::size_t position,
    const NumericLocaleContext& locale,
    int& digit,
    std::size_t& consumedBytes
);

/**
 * Scan exactly one localized numeric token without asking ICU to decide token boundaries.
 *
 * The caller owns the surrounding grammar and must select the corresponding syntax context.
 * A complete result may consume only a prefix of @p input; consumedBytes identifies that prefix.
 */
BaseExport LocalizedNumberResult scanLocalizedNumber(
    std::string_view input,
    const NumericLocaleContext& locale,
    NumericSyntaxContext syntax
);

}  // namespace Base
