// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <optional>
#include <string>
#include <string_view>

#include <FCGlobal.h>

namespace Base
{
struct NumericLocaleContext;

enum class NumericSyntaxContext
{
    Standalone,
    Expression,
    FunctionArgument
};

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
    std::size_t offset {};
    std::size_t length {};
};

struct BaseExport LocalizedNumberResult
{
    enum class Status
    {
        Complete,
        Incomplete,
        Invalid
    };

    Status status {Status::Invalid};
    double value {};
    std::string canonicalText;
    std::size_t consumedBytes {};
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

/** Scan exactly one localized numeric token without asking ICU to decide token boundaries. */
BaseExport LocalizedNumberResult scanLocalizedNumber(
    std::string_view input,
    const NumericLocaleContext& locale,
    NumericSyntaxContext syntax
);

}  // namespace Base
