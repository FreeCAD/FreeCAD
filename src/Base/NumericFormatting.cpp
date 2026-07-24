// SPDX-License-Identifier: LGPL-2.1-or-later

#include "NumericFormatting.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>

#include <unicode/decimfmt.h>
#include <unicode/dcfmtsym.h>
#include <unicode/locid.h>
#include <unicode/numfmt.h>
#include <unicode/unistr.h>

#include "Exception.h"
#include "Quantity.h"

namespace
{
const Base::NumericFormattingState fallbackFormatting {"en_US_POSIX", ".", ","};

Base::NumericFormattingState numericFormattingState = fallbackFormatting;
std::mutex numericFormattingStateMutex;

std::string toUtf8(const icu::UnicodeString& text)
{
    std::string result;
    text.toUTF8String(result);
    return result;
}

bool useQtLikeGeneralScientific(const double value, const int precision)
{
    if (!std::isfinite(value) || value == 0.0 || precision <= 0) {
        return false;
    }

    const auto exponent = static_cast<int>(std::floor(std::log10(std::abs(value))));
    return exponent < -4 || exponent >= precision;
}

struct NumericFormatSymbols
{
    std::string decimal;
    std::string grouping;
};

NumericFormatSymbols resolveNumericFormatSymbols(
    const Base::NumericFormattingState& formatting,
    const icu::Locale& locale
)
{
    NumericFormatSymbols result {formatting.decimalSeparator, formatting.groupingSeparator};

    UErrorCode status = U_ZERO_ERROR;
    icu::DecimalFormatSymbols symbols(locale, status);
    if (U_SUCCESS(status)) {
        if (result.decimal.empty()) {
            result.decimal = toUtf8(
                symbols.getSymbol(icu::DecimalFormatSymbols::kDecimalSeparatorSymbol)
            );
        }
        if (result.grouping.empty()) {
            result.grouping = toUtf8(
                symbols.getSymbol(icu::DecimalFormatSymbols::kGroupingSeparatorSymbol)
            );
        }
    }

    if (result.decimal.empty()) {
        result.decimal = ".";
    }

    return result;
}

void applyNumericFormatSymbols(icu::DecimalFormat& format, const NumericFormatSymbols& symbols)
{
    const auto* current = format.getDecimalFormatSymbols();
    if (!current) {
        return;
    }

    icu::DecimalFormatSymbols adjusted(*current);
    if (!symbols.decimal.empty()) {
        const auto decimal = icu::UnicodeString::fromUTF8(symbols.decimal);
        adjusted.setSymbol(icu::DecimalFormatSymbols::kDecimalSeparatorSymbol, decimal, false);
        adjusted.setSymbol(icu::DecimalFormatSymbols::kMonetarySeparatorSymbol, decimal, false);
    }
    if (!symbols.grouping.empty()) {
        const auto grouping = icu::UnicodeString::fromUTF8(symbols.grouping);
        adjusted.setSymbol(icu::DecimalFormatSymbols::kGroupingSeparatorSymbol, grouping, false);
        adjusted.setSymbol(icu::DecimalFormatSymbols::kMonetaryGroupingSeparatorSymbol, grouping, false);
    }
    format.setDecimalFormatSymbols(adjusted);
}

std::string localizeDecimalSeparator(std::string value, std::string_view decimalSeparator)
{
    if (decimalSeparator.empty() || decimalSeparator == ".") {
        return value;
    }

    auto pos = value.find('.');
    while (pos != std::string::npos) {
        value.replace(pos, 1, decimalSeparator);
        pos = value.find('.', pos + decimalSeparator.size());
    }

    return value;
}

std::string formatDefaultScientificLikeQt(
    const double value,
    const Base::QuantityFormat& format,
    const NumericFormatSymbols& symbols
)
{
    std::ostringstream out;
    out << std::setprecision(std::max(1, format.getPrecision())) << value;
    return localizeDecimalSeparator(out.str(), symbols.decimal);
}
}  // namespace

std::string Base::formatNumericValue(
    const double value,
    const Base::QuantityFormat& format,
    const Base::NumericFormattingState& formatting
)
{
    UErrorCode status = U_ZERO_ERROR;
    const std::string normalizedLocaleId = Base::normalizeIcuLocaleId(formatting.localeId);
    const icu::Locale locale = icu::Locale::createFromName(normalizedLocaleId.c_str());
    const NumericFormatSymbols symbols = resolveNumericFormatSymbols(formatting, locale);

    std::unique_ptr<icu::NumberFormat> nf(icu::NumberFormat::createInstance(locale, status));
    if (!U_SUCCESS(status) || !nf) {
        // Fallback: locale-independent formatting.
        std::ostringstream out;
        switch (format.format) {
            case Base::QuantityFormat::Fixed:
                out << std::fixed;
                break;
            case Base::QuantityFormat::Scientific:
                out << std::scientific;
                break;
            case Base::QuantityFormat::Default:
            default:
                break;
        }
        out << std::setprecision(format.getPrecision()) << value;
        return localizeDecimalSeparator(out.str(), symbols.decimal);
    }

    if (auto* df = dynamic_cast<icu::DecimalFormat*>(nf.get())) {
        applyNumericFormatSymbols(*df, symbols);
    }

    if (format.option & Base::QuantityFormat::OmitGroupSeparator) {
        nf->setGroupingUsed(false);
    }

    const int precision = format.getPrecision();
    switch (format.format) {
        case Base::QuantityFormat::Fixed:
            nf->setMinimumFractionDigits(precision);
            nf->setMaximumFractionDigits(precision);
            break;
        case Base::QuantityFormat::Scientific:
            if (auto* df = dynamic_cast<icu::DecimalFormat*>(nf.get())) {
                df->setScientificNotation(true);
                df->setMinimumFractionDigits(precision);
                df->setMaximumFractionDigits(precision);
                break;
            }
            [[fallthrough]];
        case Base::QuantityFormat::Default:
            if (useQtLikeGeneralScientific(value, precision)) {
                return formatDefaultScientificLikeQt(value, format, symbols);
            }
            if (auto* df = dynamic_cast<icu::DecimalFormat*>(nf.get()); precision > 0 && df) {
                df->setSignificantDigitsUsed(true);
                df->setMinimumSignificantDigits(1);
                df->setMaximumSignificantDigits(precision);
                break;
            }
            [[fallthrough]];
        default:
            nf->setMaximumFractionDigits(precision);
            break;
    }

    icu::UnicodeString s;
    nf->format(value, s);
    return toUtf8(s);
}

bool Base::isCLocaleName(std::string_view localeId)
{
    return localeId == "C" || localeId == "c" || localeId == "C.UTF-8" || localeId == "C.utf8"
        || localeId == "c.utf8" || localeId == "POSIX" || localeId == "posix";
}

std::string Base::normalizeIcuLocaleId(std::string_view localeId)
{
    if (localeId.empty() || isCLocaleName(localeId)) {
        return "en_US_POSIX";
    }

    const std::string name(localeId);
    const icu::Locale locale = icu::Locale::createFromName(name.c_str());
    if (locale.isBogus()) {
        throw Base::ValueError("Invalid ICU locale identifier: " + name);
    }

    const std::string normalizedLocaleId = locale.getName();
    if (normalizedLocaleId.empty()) {
        throw Base::ValueError("ICU returned an empty locale identifier for: " + name);
    }
    return normalizedLocaleId;
}

void Base::setIcuDefaultLocale(std::string_view localeId)
{
    UErrorCode status = U_ZERO_ERROR;
    const std::string normalizedLocaleId = normalizeIcuLocaleId(localeId);
    const icu::Locale locale = icu::Locale::createFromName(normalizedLocaleId.c_str());
    icu::Locale::setDefault(locale, status);
    if (U_FAILURE(status)) {
        throw Base::RuntimeError("Failed to set ICU default locale: " + normalizedLocaleId);
    }
}

Base::NumericFormattingState Base::createNumericFormattingState()
{
    return createNumericFormattingState(icu::Locale::getDefault().getName());
}

Base::NumericFormattingState Base::createNumericFormattingState(std::string_view localeId)
{
    const std::string normalizedLocaleId = normalizeIcuLocaleId(localeId);
    const icu::Locale locale = icu::Locale::createFromName(normalizedLocaleId.c_str());
    UErrorCode status = U_ZERO_ERROR;
    icu::DecimalFormatSymbols symbols(locale, status);
    if (U_FAILURE(status)) {
        throw Base::RuntimeError(
            "Failed to create ICU decimal-format symbols for: " + normalizedLocaleId
        );
    }

    return {
        normalizedLocaleId,
        toUtf8(symbols.getSymbol(icu::DecimalFormatSymbols::kDecimalSeparatorSymbol)),
        toUtf8(symbols.getSymbol(icu::DecimalFormatSymbols::kGroupingSeparatorSymbol))
    };
}

void Base::publishNumericFormattingState(NumericFormattingState state)
{
    const std::lock_guard lock(numericFormattingStateMutex);
    numericFormattingState = std::move(state);
}

Base::NumericFormattingState Base::currentNumericFormattingState()
{
    const std::lock_guard lock(numericFormattingStateMutex);
    return numericFormattingState;
}
