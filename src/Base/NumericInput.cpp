// SPDX-License-Identifier: LGPL-2.1-or-later

#include "NumericInput.h"

#include <charconv>
#include <cmath>
#include <cctype>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include "NumericFormatting.h"
#include "NumericInputLocale.h"
#include "StringUtils.h"

namespace
{
bool startsAt(std::string_view text, std::size_t position, std::string_view value)
{
    return !value.empty() && position + value.size() <= text.size()
        && text.substr(position, value.size()) == value;
}

bool boundary(const char ch)
{
    return std::isspace(static_cast<unsigned char>(ch)) || ch == '(' || ch == ')' || ch == '['
        || ch == ']' || ch == '<' || ch == '>' || ch == '+' || ch == '-' || ch == '*' || ch == '/'
        || ch == '^' || ch == ';';
}

Base::NumericGrammarPolicy grammarPolicy(
    const Base::NumericLocaleContext& locale,
    const Base::NumericSyntaxContext syntax
)
{
    if (syntax != Base::NumericSyntaxContext::FunctionArgument) {
        return {locale.decimalSeparator, locale.groupingSeparator, {}, true};
    }

    // A comma-decimal locale uses a semicolon for function arguments. In all other locales a
    // comma remains the function separator. Grouping is disabled when its symbol is structural.
    const std::string_view argumentSeparator = locale.decimalSeparator == ","
        ? std::string_view {";"}
        : std::string_view {","};
    return {
        locale.decimalSeparator,
        locale.groupingSeparator,
        argumentSeparator,
        locale.groupingSeparator != argumentSeparator
    };
}

Base::LocalizedNumberResult invalid(
    const Base::NumericDiagnosticKind kind,
    const std::size_t offset,
    const std::size_t consumed,
    const std::size_t length = 1
)
{
    Base::LocalizedNumberResult result;
    result.status = Base::LocalizedNumberResult::Status::Invalid;
    result.consumedBytes = consumed;
    result.diagnostic = Base::NumericDiagnostic {kind, offset, length};
    return result;
}

Base::LocalizedNumberResult incomplete(
    const Base::NumericDiagnosticKind kind,
    const std::size_t offset,
    const std::size_t consumed,
    std::string canonical,
    const std::size_t length = 1
)
{
    Base::LocalizedNumberResult result;
    result.status = Base::LocalizedNumberResult::Status::Incomplete;
    result.consumedBytes = consumed;
    result.canonicalText = std::move(canonical);
    result.diagnostic = Base::NumericDiagnostic {kind, offset, length};
    return result;
}

enum class CanonicalNumberStatus
{
    Complete,
    Invalid,
    OutOfRange
};

CanonicalNumberStatus parseCanonicalDouble(std::string_view text, double& value)
{
    // CMake probes the actual overload because some standard libraries expose <charconv> without
    // implementing floating-point from_chars.
#if FC_HAS_FLOATING_POINT_FROM_CHARS
    const auto first = text.data();
    const auto last = first + text.size();
    const auto conversion = std::from_chars(first, last, value, std::chars_format::general);
    if (conversion.ec == std::errc::result_out_of_range) {
        return CanonicalNumberStatus::OutOfRange;
    }
    if (conversion.ec != std::errc {} || conversion.ptr != last) {
        return CanonicalNumberStatus::Invalid;
    }
#else
    if (!Base::StringUtils::parseDouble(text, value)) {
        // The scanner has already validated the canonical grammar, so a failure here means that
        // the value cannot be represented by the fallback conversion.
        return CanonicalNumberStatus::OutOfRange;
    }
#endif

    return std::isfinite(value) ? CanonicalNumberStatus::Complete : CanonicalNumberStatus::OutOfRange;
}

bool decimalAt(
    std::string_view input,
    const std::size_t position,
    const Base::NumericGrammarPolicy& policy,
    std::size_t& length
)
{
    length = 0;
    if (startsAt(input, position, policy.decimalSeparator)) {
        if (policy.decimalSeparator == policy.argumentSeparator) {
            return false;
        }
        // In comma-decimal function syntax, whitespace after the comma makes it the argument
        // separator. Adjacent comma digits remain the localized decimal form.
        if (policy.argumentSeparator == ";" && policy.decimalSeparator == ","
            && position + policy.decimalSeparator.size() < input.size()
            && std::isspace(
                static_cast<unsigned char>(input[position + policy.decimalSeparator.size()])
            )) {
            return false;
        }
        length = policy.decimalSeparator.size();
        return true;
    }
    if (input[position] == '.' && policy.decimalSeparator != ".") {
        length = 1;
        return true;
    }
    return input[position] == '.';
}

bool groupingAt(
    std::string_view input,
    const std::size_t position,
    const Base::NumericLocaleContext& locale,
    const Base::NumericGrammarPolicy& policy,
    const bool alreadyGrouped,
    std::size_t& length
)
{
    length = 0;
    if (!policy.allowGrouping) {
        return false;
    }
    if (!startsAt(input, position, policy.groupingSeparator)) {
        return false;
    }

    // In comma-decimal locales the canonical dot is also commonly the grouping symbol. Keep a
    // lone dot as canonical decimal syntax unless the rest of the token proves that it is a
    // localized grouping separator followed by a localized decimal or another group.
    if (!alreadyGrouped && policy.groupingSeparator == "." && policy.decimalSeparator != ".") {
        const auto groupStart = position + policy.groupingSeparator.size();
        std::size_t digitCount = 0;
        std::size_t digitPosition = groupStart;
        while (digitPosition < input.size()) {
            int digit = 0;
            std::size_t digitLength = 0;
            if (!Base::localizedDigitAt(input, digitPosition, locale, digit, digitLength)) {
                break;
            }
            ++digitCount;
            digitPosition += digitLength;
        }
        const auto afterGroup = digitPosition;
        if (digitCount != static_cast<std::size_t>(locale.primaryGroupingSize)
            || (!startsAt(input, afterGroup, policy.decimalSeparator)
                && !startsAt(input, afterGroup, policy.groupingSeparator))) {
            return false;
        }
    }

    length = policy.groupingSeparator.size();
    if (policy.groupingSeparator == " ") {
        int digit = 0;
        std::size_t digitLength = 0;
        if (position + length >= input.size()
            || !Base::localizedDigitAt(input, position + length, locale, digit, digitLength)) {
            return false;
        }
    }
    return true;
}

bool validGrouping(const std::vector<int>& groups, const Base::NumericLocaleContext& locale)
{
    if (groups.size() < 2 || locale.primaryGroupingSize <= 0 || locale.secondaryGroupingSize <= 0) {
        return false;
    }
    if (groups.back() != locale.primaryGroupingSize) {
        return false;
    }
    for (std::size_t i = 1; i + 1 < groups.size(); ++i) {
        if (groups[i] != locale.secondaryGroupingSize) {
            return false;
        }
    }
    return groups.front() > 0 && groups.front() <= locale.secondaryGroupingSize;
}

bool signAt(
    std::string_view input,
    const std::size_t position,
    const Base::NumericLocaleContext& locale,
    std::size_t& length,
    char& canonical
)
{
    length = 0;
    canonical = 0;
    if (startsAt(input, position, locale.negativeSign)) {
        length = locale.negativeSign.size();
        canonical = '-';
        return true;
    }
    if (startsAt(input, position, locale.positiveSign)) {
        length = locale.positiveSign.size();
        canonical = '+';
        return true;
    }
    if (input[position] == '-' || input[position] == '+') {
        length = 1;
        canonical = input[position];
        return true;
    }
    return false;
}

}  // namespace

Base::NumericGrammarPolicy Base::numericGrammarPolicy(
    const NumericLocaleContext& locale,
    const NumericSyntaxContext syntax
)
{
    return grammarPolicy(locale, syntax);
}

Base::LocalizedNumberResult Base::scanLocalizedNumber(
    const std::string_view input,
    const NumericLocaleContext& locale,
    const NumericSyntaxContext syntax
)
{
    const auto policy = Base::numericGrammarPolicy(locale, syntax);
    if (input.empty()) {
        return incomplete(NumericDiagnosticKind::ExpectedDigit, 0, 0, {});
    }

    std::size_t position = 0;
    std::string canonical;
    canonical.reserve(input.size());

    std::size_t signLength = 0;
    char sign = 0;
    if (signAt(input, position, locale, signLength, sign)) {
        // std::from_chars accepts a leading minus but not a leading plus. A positive sign is
        // still recognized by the scanner; it is simply omitted from the canonical literal.
        if (sign != '+') {
            canonical.push_back(sign);
        }
        position += signLength;
        if (position == input.size() || boundary(input[position])) {
            return incomplete(NumericDiagnosticKind::IncompleteSign, position, position, canonical);
        }
    }

    std::vector<int> groups;
    int digitsInGroup = 0;
    int totalDigits = 0;
    bool grouped = false;
    std::size_t lastGroupingStart = 0;
    std::size_t lastGroupingLength = 0;

    while (position < input.size()) {
        int digit = 0;
        std::size_t digitLength = 0;
        if (localizedDigitAt(input, position, locale, digit, digitLength)) {
            canonical.push_back(static_cast<char>('0' + digit));
            position += digitLength;
            ++digitsInGroup;
            ++totalDigits;
            continue;
        }

        std::size_t separatorLength = 0;
        if (groupingAt(input, position, locale, policy, grouped, separatorLength)) {
            if (digitsInGroup == 0) {
                return invalid(NumericDiagnosticKind::InvalidGrouping, position, position, separatorLength);
            }
            groups.push_back(digitsInGroup);
            digitsInGroup = 0;
            grouped = true;
            lastGroupingStart = position;
            lastGroupingLength = separatorLength;
            position += separatorLength;
            int nextDigit = 0;
            std::size_t nextDigitLength = 0;
            if (position == input.size()
                || !localizedDigitAt(input, position, locale, nextDigit, nextDigitLength)) {
                return incomplete(
                    NumericDiagnosticKind::IncompleteGrouping,
                    position,
                    position,
                    canonical,
                    separatorLength
                );
            }
            continue;
        }
        break;
    }

    if (totalDigits == 0) {
        std::size_t decimalLength = 0;
        if (!decimalAt(input, position, policy, decimalLength)) {
            return invalid(NumericDiagnosticKind::ExpectedDigit, position, position);
        }
        canonical.push_back('.');
        position += decimalLength;
        int nextDigit = 0;
        std::size_t nextDigitLength = 0;
        if (position == input.size()
            || !localizedDigitAt(input, position, locale, nextDigit, nextDigitLength)) {
            return incomplete(
                NumericDiagnosticKind::IncompleteDecimal,
                position,
                position,
                canonical,
                decimalLength
            );
        }
        while (position < input.size()) {
            int fractionalDigit = 0;
            std::size_t fractionalDigitLength = 0;
            if (!localizedDigitAt(input, position, locale, fractionalDigit, fractionalDigitLength)) {
                break;
            }
            canonical.push_back(static_cast<char>('0' + fractionalDigit));
            position += fractionalDigitLength;
            ++totalDigits;
        }
    }
    else {
        if (grouped) {
            groups.push_back(digitsInGroup);
            if (!validGrouping(groups, locale)) {
                return invalid(
                    NumericDiagnosticKind::InvalidGrouping,
                    lastGroupingStart,
                    position,
                    lastGroupingLength
                );
            }
        }

        std::size_t decimalLength = 0;
        if (position < input.size() && decimalAt(input, position, policy, decimalLength)) {
            canonical.push_back('.');
            position += decimalLength;
            int nextDigit = 0;
            std::size_t nextDigitLength = 0;
            if (position == input.size()
                || !localizedDigitAt(input, position, locale, nextDigit, nextDigitLength)) {
                return incomplete(
                    NumericDiagnosticKind::IncompleteDecimal,
                    position,
                    position,
                    canonical,
                    decimalLength
                );
            }
            while (position < input.size()) {
                int fractionalDigit = 0;
                std::size_t fractionalDigitLength = 0;
                if (!localizedDigitAt(input, position, locale, fractionalDigit, fractionalDigitLength)) {
                    break;
                }
                canonical.push_back(static_cast<char>('0' + fractionalDigit));
                position += fractionalDigitLength;
            }
        }
    }

    if (position < input.size() && (input[position] == 'e' || input[position] == 'E')) {
        canonical.push_back('e');
        ++position;
        std::size_t exponentSignLength = 0;
        char exponentSign = 0;
        if (position < input.size()
            && signAt(input, position, locale, exponentSignLength, exponentSign)) {
            canonical.push_back(exponentSign);
            position += exponentSignLength;
        }
        const auto exponentStart = position;
        while (position < input.size()) {
            int exponentDigit = 0;
            std::size_t exponentDigitLength = 0;
            if (!localizedDigitAt(input, position, locale, exponentDigit, exponentDigitLength)) {
                break;
            }
            canonical.push_back(static_cast<char>('0' + exponentDigit));
            position += exponentDigitLength;
        }
        if (position == exponentStart) {
            return incomplete(NumericDiagnosticKind::IncompleteExponent, position, position, canonical);
        }
    }

    if (position < input.size()) {
        if (std::isspace(static_cast<unsigned char>(input[position]))) {
            auto next = position;
            while (next < input.size() && std::isspace(static_cast<unsigned char>(input[next]))) {
                ++next;
            }
            int nextDigit = 0;
            std::size_t nextDigitLength = 0;
            if (next < input.size()
                && localizedDigitAt(input, next, locale, nextDigit, nextDigitLength)) {
                return invalid(
                    NumericDiagnosticKind::UnexpectedSeparator,
                    position,
                    position,
                    next - position
                );
            }
        }

        // Do not silently stop before a visually similar UTF-8 separator followed by another
        // digit. Only the exact configured grouping separator belongs to this token.
        UChar32 separatorCodePoint = 0;
        std::size_t separatorBytes = 0;
        int nextDigit = 0;
        std::size_t nextDigitBytes = 0;
        if (NumericInputPrivate::decodeUtf8(input, position, separatorCodePoint, separatorBytes)
            && NumericInputPrivate::isSpaceLike(separatorCodePoint)
            && localizedDigitAt(input, position + separatorBytes, locale, nextDigit, nextDigitBytes)) {
            return invalid(NumericDiagnosticKind::UnexpectedSeparator, position, position, separatorBytes);
        }

        std::size_t separatorLength = 0;
        if (decimalAt(input, position, policy, separatorLength)
            || groupingAt(input, position, locale, policy, grouped, separatorLength)) {
            return invalid(NumericDiagnosticKind::UnexpectedSeparator, position, position, separatorLength);
        }
    }

    double value = 0.0;
    switch (parseCanonicalDouble(canonical, value)) {
        case CanonicalNumberStatus::Invalid:
            return invalid(NumericDiagnosticKind::InvalidLiteral, 0, position);
        case CanonicalNumberStatus::OutOfRange:
            return invalid(NumericDiagnosticKind::OutOfRange, 0, position);
        case CanonicalNumberStatus::Complete:
            break;
    }

    LocalizedNumberResult result;
    result.status = LocalizedNumberResult::Status::Complete;
    result.value = value;
    result.canonicalText = std::move(canonical);
    result.consumedBytes = position;
    return result;
}
