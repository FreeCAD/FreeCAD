// SPDX-License-Identifier: LGPL-2.1-or-later

#include "NumericInput.h"

#include <charconv>
#include <cmath>
#include <cctype>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <unicode/decimfmt.h>
#include <unicode/dcfmtsym.h>
#include <unicode/fmtable.h>
#include <unicode/locid.h>
#include <unicode/numfmt.h>
#include <unicode/parsepos.h>
#include <unicode/stringpiece.h>
#include <unicode/unistr.h>

#include "Exception.h"
#include "NumericFormatting.h"

namespace
{
enum class NumberSyntax
{
    Canonical,
    Localized
};

struct ParsedPrefix
{
    double value;
    std::size_t bytes;
    NumberSyntax syntax;
};

struct LocaleSymbols
{
    std::string decimal;
    std::string grouping;
    std::string positiveSign;
    std::string negativeSign;
};

bool startsWithAt(std::string_view text, std::size_t position, std::string_view symbol)
{
    return !symbol.empty() && position + symbol.size() <= text.size()
        && text.substr(position, symbol.size()) == symbol;
}

bool contains(std::string_view text, std::string_view symbol)
{
    return !symbol.empty() && text.find(symbol) != std::string_view::npos;
}

std::string toUtf8(const icu::UnicodeString& text)
{
    std::string result;
    text.toUTF8String(result);
    return result;
}

std::unique_ptr<icu::DecimalFormat> makeParser(
    std::string_view localeId,
    const Base::NumericFormattingState* formatting,
    bool allowGrouping,
    LocaleSymbols* effectiveSymbols = nullptr
)
{
    const std::string normalizedLocaleId = Base::normalizeIcuLocaleId(localeId);
    const icu::Locale locale = icu::Locale::createFromName(normalizedLocaleId.c_str());
    UErrorCode status = U_ZERO_ERROR;
    std::unique_ptr<icu::NumberFormat> numberFormat(icu::NumberFormat::createInstance(locale, status));
    if (U_FAILURE(status) || !numberFormat) {
        throw Base::ParserError("Failed to create localized numeric parser");
    }

    auto* rawDecimalFormat = dynamic_cast<icu::DecimalFormat*>(numberFormat.get());
    if (!rawDecimalFormat) {
        throw Base::ParserError("Localized numeric parser is not a decimal parser");
    }
    numberFormat.release();
    std::unique_ptr<icu::DecimalFormat> decimalFormat(rawDecimalFormat);
    decimalFormat->setLenient(false);
    decimalFormat->setGroupingUsed(allowGrouping);

    if (formatting) {
        const auto* currentSymbols = decimalFormat->getDecimalFormatSymbols();
        if (!currentSymbols) {
            throw Base::ParserError("Localized numeric parser has no symbols");
        }

        icu::DecimalFormatSymbols adjusted(*currentSymbols);
        if (!formatting->decimalSeparator.empty()) {
            adjusted.setSymbol(
                icu::DecimalFormatSymbols::kDecimalSeparatorSymbol,
                icu::UnicodeString::fromUTF8(formatting->decimalSeparator),
                false
            );
        }
        if (!formatting->groupingSeparator.empty()) {
            adjusted.setSymbol(
                icu::DecimalFormatSymbols::kGroupingSeparatorSymbol,
                icu::UnicodeString::fromUTF8(formatting->groupingSeparator),
                false
            );
        }
        decimalFormat->setDecimalFormatSymbols(adjusted);
    }

    if (effectiveSymbols) {
        const auto* symbols = decimalFormat->getDecimalFormatSymbols();
        if (!symbols) {
            throw Base::ParserError("Localized numeric parser has no effective symbols");
        }
        effectiveSymbols->decimal = toUtf8(
            symbols->getSymbol(icu::DecimalFormatSymbols::kDecimalSeparatorSymbol)
        );
        effectiveSymbols->grouping = toUtf8(
            symbols->getSymbol(icu::DecimalFormatSymbols::kGroupingSeparatorSymbol)
        );
        effectiveSymbols->positiveSign = toUtf8(
            symbols->getSymbol(icu::DecimalFormatSymbols::kPlusSignSymbol)
        );
        effectiveSymbols->negativeSign = toUtf8(
            symbols->getSymbol(icu::DecimalFormatSymbols::kMinusSignSymbol)
        );
    }

    return decimalFormat;
}

std::optional<ParsedPrefix> parseWith(
    const icu::NumberFormat& parser,
    std::string_view text,
    NumberSyntax syntax
)
{
    const auto input = icu::UnicodeString::fromUTF8(
        icu::StringPiece(text.data(), static_cast<int32_t>(text.size()))
    );
    icu::ParsePosition position(0);
    icu::Formattable parsed;
    parser.parse(input, parsed, position);
    if (position.getIndex() <= 0) {
        return std::nullopt;
    }

    std::string consumed;
    input.tempSubStringBetween(0, position.getIndex()).toUTF8String(consumed);

    UErrorCode status = U_ZERO_ERROR;
    const double value = parsed.getDouble(status);
    if (U_FAILURE(status) || !std::isfinite(value)) {
        throw Base::ParserError("Invalid localized numeric literal");
    }
    return ParsedPrefix {value, consumed.size(), syntax};
}

class NumericParser
{
public:
    explicit NumericParser(const Base::NumericFormattingState& formatting)
        : grouped(makeParser(formatting.localeId, &formatting, true, &symbols))
        , ungrouped(makeParser(formatting.localeId, &formatting, false))
        , canonical(makeParser("en_US_POSIX", nullptr, false))
    {}

    const LocaleSymbols& localeSymbols() const
    {
        return symbols;
    }

    std::optional<ParsedPrefix> parsePrefix(
        std::string_view text,
        bool allowGrouping,
        bool commaTerminatesNumber
    ) const
    {
        auto localized = parseWith(
            allowGrouping ? static_cast<const icu::NumberFormat&>(*grouped)
                          : static_cast<const icu::NumberFormat&>(*ungrouped),
            text,
            NumberSyntax::Localized
        );
        const auto canonicalResult = parseWith(*canonical, text, NumberSyntax::Canonical);

        if (commaTerminatesNumber && localized && symbols.decimal == ","
            && text.substr(0, localized->bytes).ends_with(',')) {
            localized.reset();
        }
        if (!localized) {
            return canonicalResult;
        }
        if (!canonicalResult) {
            return localized;
        }
        if (localized->bytes != canonicalResult->bytes) {
            return localized->bytes > canonicalResult->bytes ? localized : canonicalResult;
        }

        const auto token = text.substr(0, localized->bytes);
        if ((symbols.positiveSign != "+" && startsWithAt(token, 0, symbols.positiveSign))
            || (symbols.negativeSign != "-" && startsWithAt(token, 0, symbols.negativeSign))) {
            return localized;
        }

        if ((symbols.decimal != "." && contains(token, symbols.decimal))
            || (allowGrouping && symbols.grouping != symbols.decimal
                && contains(token, symbols.grouping))) {
            return localized;
        }
        return canonicalResult;
    }

private:
    LocaleSymbols symbols;
    std::unique_ptr<icu::DecimalFormat> grouped;
    std::unique_ptr<icu::DecimalFormat> ungrouped;
    std::unique_ptr<icu::DecimalFormat> canonical;
};

bool isAsciiDigit(char ch)
{
    return ch >= '0' && ch <= '9';
}

bool couldStartNumber(std::string_view text, std::size_t position, const LocaleSymbols& symbols)
{
    return isAsciiDigit(text[position]) || text[position] == '.'
        || startsWithAt(text, position, symbols.decimal)
        || startsWithAt(text, position, symbols.positiveSign)
        || startsWithAt(text, position, symbols.negativeSign);
}

bool previousTokenCanEndExpression(std::string_view text, std::size_t position)
{
    while (position > 0) {
        const auto ch = static_cast<unsigned char>(text[--position]);
        if (std::isspace(ch)) {
            continue;
        }
        return std::isalnum(ch) || ch == '_' || ch >= 0x80 || ch == ')' || ch == ']';
    }
    return false;
}

bool previousTokenIsIdentifier(std::string_view text, std::size_t position)
{
    while (position > 0) {
        const auto ch = static_cast<unsigned char>(text[--position]);
        if (std::isspace(ch)) {
            continue;
        }
        return std::isalnum(ch) || ch == '_' || ch >= 0x80;
    }
    return false;
}

std::string canonicalNumber(double value)
{
    char buffer[128];
    const auto [end, error] = std::to_chars(buffer, buffer + sizeof(buffer), value);
    if (error != std::errc {}) {
        throw Base::ParserError("Failed to canonicalize localized numeric literal");
    }
    return {buffer, end};
}

std::size_t copyExpressionString(std::string_view text, std::size_t position, std::string& output)
{
    const std::size_t start = position;
    position += 2;
    bool escaped = false;
    bool terminated = false;
    while (position + 1 < text.size()) {
        if (!escaped && text[position] == '>' && text[position + 1] == '>') {
            position += 2;
            terminated = true;
            break;
        }
        escaped = !escaped && text[position] == '\\';
        ++position;
    }
    if (!terminated) {
        // Canonicalization is not responsible for repairing expression syntax. Preserve the
        // complete malformed token so the canonical expression parser can report the error.
        position = text.size();
    }
    output.append(text.substr(start, position - start));
    return position;
}

std::size_t separatorLengthAt(std::string_view text, std::size_t position, const LocaleSymbols& symbols)
{
    if (text[position] == '.' || text[position] == ',') {
        return 1;
    }
    if (startsWithAt(text, position, symbols.decimal)) {
        return symbols.decimal.size();
    }
    if (startsWithAt(text, position, symbols.grouping)) {
        return symbols.grouping.size();
    }
    return 0;
}

bool isUnconsumedNumericSeparator(std::string_view text, std::size_t position, const LocaleSymbols& symbols)
{
    const std::size_t separatorLength = separatorLengthAt(text, position, symbols);
    const bool digitBefore = position > 0 && isAsciiDigit(text[position - 1]);
    const bool digitAfter = position + separatorLength < text.size()
        && isAsciiDigit(text[position + separatorLength]);
    return separatorLength != 0 && (digitBefore || digitAfter);
}

}  // namespace

std::string Base::canonicalizeNumericInput(
    std::string_view text,
    const NumericFormattingState& formatting,
    NumericInputMode mode
)
{
    const NumericParser parser(formatting);
    const auto& symbols = parser.localeSymbols();
    std::vector<bool> functionParentheses;
    int functionDepth = 0;

    std::string result;
    result.reserve(text.size());

    for (std::size_t position = 0; position < text.size();) {
        if (position + 1 < text.size() && text[position] == '<' && text[position + 1] == '<') {
            position = copyExpressionString(text, position, result);
            continue;
        }
        // Quantity.l treats bracketed text as a lexer comment. Keep it byte-for-byte so decimal,
        // grouping, and sign characters in descriptive text are never interpreted as numbers.
        if (mode == NumericInputMode::Quantity && text[position] == '[') {
            const auto commentEnd = text.find(']', position + 1);
            if (commentEnd == std::string_view::npos) {
                result.append(text.substr(position));
                break;
            }
            result.append(text.substr(position, commentEnd - position + 1));
            position = commentEnd + 1;
            continue;
        }

        if (text[position] == '(') {
            const bool functionCall = previousTokenIsIdentifier(text, position);
            functionParentheses.push_back(functionCall);
            functionDepth += functionCall ? 1 : 0;
            result.push_back(text[position++]);
            continue;
        }
        if (text[position] == ')') {
            if (!functionParentheses.empty()) {
                functionDepth -= functionParentheses.back() ? 1 : 0;
                functionParentheses.pop_back();
            }
            result.push_back(text[position++]);
            continue;
        }

        // Function commas are structural. Grouping is therefore limited to quantity literals
        // outside function calls, where the grammar has no competing comma separator.
        const bool inFunctionArguments = functionDepth > 0;
        if (inFunctionArguments && text[position] == ','
            && previousTokenCanEndExpression(text, position)) {
            result.push_back(';');
            ++position;
            continue;
        }
        if (isUnconsumedNumericSeparator(text, position, symbols) && position > 0
            && isAsciiDigit(text[position - 1])) {
            throw ParserError("Invalid or ambiguous localized numeric separator");
        }

        const bool allowGrouping = mode == NumericInputMode::Quantity && !inFunctionArguments;
        if (couldStartNumber(text, position, symbols)) {
            if (const auto parsed
                = parser.parsePrefix(text.substr(position), allowGrouping, inFunctionArguments)) {
                const auto original = text.substr(position, parsed->bytes);
                result += parsed->syntax == NumberSyntax::Canonical ? std::string(original)
                                                                    : canonicalNumber(parsed->value);
                position += parsed->bytes;
                continue;
            }
        }

        if (inFunctionArguments && text[position] == ',') {
            result.push_back(';');
            ++position;
            continue;
        }
        if (isUnconsumedNumericSeparator(text, position, symbols)) {
            throw ParserError("Invalid or ambiguous localized numeric separator");
        }

        result.push_back(text[position++]);
    }

    return result;
}
