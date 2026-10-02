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
const Base::NumericLocaleContext fallbackFormatting {"en_US_POSIX", ".", ",", "+", "-", 3, 3, "0"};

Base::NumericLocaleContext numericLocaleContext = fallbackFormatting;
std::mutex numericLocaleContextMutex;

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
    std::string positiveSign;
    std::string negativeSign;
    std::string zeroDigit;
};

NumericFormatSymbols resolveNumericFormatSymbols(
    const Base::NumericLocaleContext& formatting,
    const icu::Locale& locale
)
{
    NumericFormatSymbols result {
        formatting.decimalSeparator,
        formatting.groupingSeparator,
        formatting.positiveSign,
        formatting.negativeSign,
        formatting.zeroDigit
    };

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
        if (result.positiveSign.empty()) {
            result.positiveSign = toUtf8(symbols.getSymbol(icu::DecimalFormatSymbols::kPlusSignSymbol));
        }
        if (result.negativeSign.empty()) {
            result.negativeSign = toUtf8(
                symbols.getSymbol(icu::DecimalFormatSymbols::kMinusSignSymbol)
            );
        }
        if (result.zeroDigit.empty()) {
            result.zeroDigit = toUtf8(symbols.getSymbol(icu::DecimalFormatSymbols::kZeroDigitSymbol));
        }
    }

    if (result.decimal.empty()) {
        result.decimal = ".";
    }
    if (result.zeroDigit.empty()) {
        result.zeroDigit = "0";
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
    if (!symbols.positiveSign.empty()) {
        adjusted.setSymbol(
            icu::DecimalFormatSymbols::kPlusSignSymbol,
            icu::UnicodeString::fromUTF8(symbols.positiveSign),
            false
        );
    }
    if (!symbols.negativeSign.empty()) {
        adjusted.setSymbol(
            icu::DecimalFormatSymbols::kMinusSignSymbol,
            icu::UnicodeString::fromUTF8(symbols.negativeSign),
            false
        );
    }
    if (!symbols.zeroDigit.empty()) {
        adjusted.setSymbol(
            icu::DecimalFormatSymbols::kZeroDigitSymbol,
            icu::UnicodeString::fromUTF8(symbols.zeroDigit),
            false
        );
    }
    // Keep scientific output in the same token grammar consumed by FreeCAD. ICU otherwise uses
    // locale-specific multiplication and exponent words (for example "×10^" or Arabic text),
    // which are display notation rather than scanner syntax.
    adjusted.setSymbol(icu::DecimalFormatSymbols::kExponentialSymbol, icu::UnicodeString("e"), false);
    adjusted.setSymbol(
        icu::DecimalFormatSymbols::kExponentMultiplicationSymbol,
        icu::UnicodeString(),
        false
    );
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
    const Base::NumericLocaleContext& formatting
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
        if (formatting.primaryGroupingSize > 0) {
            df->setGroupingSize(formatting.primaryGroupingSize);
        }
        if (formatting.secondaryGroupingSize > 0) {
            df->setSecondaryGroupingSize(formatting.secondaryGroupingSize);
        }
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

Base::NumericLocaleContext Base::createNumericLocaleContext()
{
    return createNumericLocaleContext(icu::Locale::getDefault().getName());
}

Base::NumericLocaleContext Base::createNumericLocaleContext(std::string_view localeId)
{
    if (isCLocaleName(localeId)) {
        return fallbackFormatting;
    }

    const std::string normalizedLocaleId = normalizeIcuLocaleId(localeId);
    const icu::Locale locale = icu::Locale::createFromName(normalizedLocaleId.c_str());
    UErrorCode status = U_ZERO_ERROR;
    std::unique_ptr<icu::NumberFormat> numberFormat(icu::NumberFormat::createInstance(locale, status));
    auto* decimalFormat = dynamic_cast<icu::DecimalFormat*>(numberFormat.get());
    if (U_FAILURE(status) || !decimalFormat) {
        throw Base::RuntimeError(
            "Failed to create ICU decimal-format symbols for: " + normalizedLocaleId
        );
    }

    const auto* symbols = decimalFormat->getDecimalFormatSymbols();
    if (!symbols) {
        throw Base::RuntimeError(
            "Failed to obtain ICU decimal-format symbols for: " + normalizedLocaleId
        );
    }

    const auto primaryGroupingSize = decimalFormat->getGroupingSize();
    const auto secondaryGroupingSize = decimalFormat->getSecondaryGroupingSize();

    return {
        normalizedLocaleId,
        toUtf8(symbols->getSymbol(icu::DecimalFormatSymbols::kDecimalSeparatorSymbol)),
        toUtf8(symbols->getSymbol(icu::DecimalFormatSymbols::kGroupingSeparatorSymbol)),
        toUtf8(symbols->getSymbol(icu::DecimalFormatSymbols::kPlusSignSymbol)),
        toUtf8(symbols->getSymbol(icu::DecimalFormatSymbols::kMinusSignSymbol)),
        primaryGroupingSize,
        secondaryGroupingSize > 0 ? secondaryGroupingSize : primaryGroupingSize,
        toUtf8(symbols->getSymbol(icu::DecimalFormatSymbols::kZeroDigitSymbol))
    };
}

void Base::publishNumericLocaleContext(NumericLocaleContext state)
{
    const std::lock_guard lock(numericLocaleContextMutex);
    numericLocaleContext = std::move(state);
}

Base::NumericLocaleContext Base::currentNumericLocaleContext()
{
    const std::lock_guard lock(numericLocaleContextMutex);
    return numericLocaleContext;
}
