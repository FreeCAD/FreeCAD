// SPDX-License-Identifier: LGPL-2.1-or-later

/************************************************************************
 *                                                                      *
 *   This file is part of the FreeCAD CAx development system.           *
 *                                                                      *
 *   This library is free software; you can redistribute it and/or      *
 *   modify it under the terms of the GNU Library General Public        *
 *   License as published by the Free Software Foundation; either       *
 *   version 2 of the License, or (at your option) any later version.   *
 *                                                                      *
 *   This library  is distributed in the hope that it will be useful,   *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of     *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the      *
 *   GNU Library General Public License for more details.               *
 *                                                                      *
 *   You should have received a copy of the GNU Library General Public  *
 *   License along with this library; see the file COPYING.LIB. If not, *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,      *
 *   Suite 330, Boston, MA  02111-1307, USA                             *
 *                                                                      *
 ************************************************************************/

#include <cmath>
#include <algorithm>
#include <iomanip>
#include <limits>
#include <memory>
#include <sstream>
#include <string>

#include <unicode/decimfmt.h>
#include <unicode/dcfmtsym.h>
#include <unicode/locid.h>
#include <unicode/numfmt.h>
#include <unicode/unistr.h>

#include "Quantity.h"
#include "Tools.h"
#include "UnitsSchema.h"
#include "UnitsSchemasData.h"
#include "UnitsSchemasSpecs.h"
#include "Exception.h"

using Base::UnitsSchema;
using Base::UnitsSchemaSpec;

namespace
{
std::string toUtf8(const icu::UnicodeString& s)
{
    std::string out;
    s.toUTF8String(out);
    return out;
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

NumericFormatSymbols resolveNumericFormatSymbols(const icu::Locale& locale)
{
    NumericFormatSymbols result {
        Base::Tools::getCurrentNumericFormattingDecimalSeparator(),
        Base::Tools::getCurrentNumericFormattingGroupingSeparator()
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

icu::Locale resolveIcuLocale(std::string_view localeId)
{
    if (localeId.empty() || Base::Tools::isCLocaleName(localeId)) {
        return icu::Locale("en_US_POSIX");
    }

    const std::string localeName(localeId);
    return icu::Locale::createFromName(localeName.c_str());
}

std::string formatNumberIcu(const double value, const Base::QuantityFormat& format, std::string_view localeId)
{
    UErrorCode status = U_ZERO_ERROR;
    const icu::Locale locale = resolveIcuLocale(localeId);
    const NumericFormatSymbols symbols = resolveNumericFormatSymbols(locale);

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
}  // namespace


UnitsSchema::UnitsSchema(UnitsSchemaSpec spec)
    : spec {std::move(spec)}
{}

std::string UnitsSchema::translate(const Quantity& quant) const
{  // to satisfy GCC
    return translate(quant, Base::Tools::getCurrentNumericFormattingLocale());
}

std::string UnitsSchema::translate(const Quantity& quant, std::string_view localeId) const
{
    double dummy1 {};
    std::string dummy2;
    return translate(quant, localeId, dummy1, dummy2);
}

std::string UnitsSchema::translate(const Quantity& quant, double& factor, std::string& unitString) const
{
    return translate(quant, Base::Tools::getCurrentNumericFormattingLocale(), factor, unitString);
}

std::string UnitsSchema::translate(
    const Quantity& quant,
    std::string_view localeId,
    double& factor,
    std::string& unitString
) const
{
    // Use defaults without schema-level translation.
    factor = 1.0;
    unitString = quant.getUnit().getString();

    if (spec.translationSpecs.empty()) {
        return toLocale(quant, localeId, factor, unitString);
    }

    const auto unitName = quant.getUnit().getTypeString();
    if (!spec.translationSpecs.contains(unitName)) {
        return toLocale(quant, localeId, factor, unitString);
    }

    const auto value = quant.getValue();
    const auto magnitude = std::abs(value);
    auto isSuitable = [&](const UnitTranslationSpec& row) {
        // Shrink threshold slightly so values at exact threshold boundaries
        // (e.g. "1 S/m" = 1e-9 at threshold 1e-9) fall through to the next unit.
        constexpr double relEps = 1e-12;
        return row.threshold * (1.0 - relEps) > magnitude
            || row.threshold == 0;  // zero indicates default
    };

    auto unitSpecs = spec.translationSpecs.at(unitName);
    const auto unitSpec = std::find_if(unitSpecs.begin(), unitSpecs.end(), isSuitable);
    if (unitSpec == unitSpecs.end()) {
        throw RuntimeError(
            "Suitable threshold not found. Schema: " + spec.name + " value: " + std::to_string(value)
        );
    }

    if (unitSpec->factor == 0) {
        const QuantityFormat& format = quant.getFormat();
        return UnitsSchemasData::runSpecial(
            unitSpec->unitString,
            value,
            format.getPrecision(),
            format.getDenominator(),
            factor,
            unitString
        );
    }

    factor = unitSpec->factor;
    unitString = unitSpec->unitString;

    return toLocale(quant, localeId, factor, unitString);
}

std::string UnitsSchema::toLocale(
    const Quantity& quant,
    std::string_view localeId,
    const double factor,
    const std::string& unitString
)
{
    const QuantityFormat& format = quant.getFormat();
    const double v = quant.getValue() / factor;
    const std::string valueString = std::isfinite(v) ? formatNumberIcu(v, format, localeId)
                                                     : std::to_string(v);

    auto notUnit = [](auto s) {
        return s.empty() || s == "°" || s == "″" || s == "′" || s == "\"" || s == "'";
    };

    return fmt::format("{}{}{}", valueString, notUnit(unitString) ? "" : " ", unitString);
}

bool UnitsSchema::isMultiUnitLength() const
{
    return spec.isMultUnitLen;
}

bool UnitsSchema::isMultiUnitAngle() const
{
    return spec.isMultUnitAngle;
}

std::string UnitsSchema::getBasicLengthUnit() const
{
    return spec.basicLengthUnitStr;
}

std::string UnitsSchema::getName() const
{
    return spec.name;
}

std::string UnitsSchema::getDescription() const
{
    return spec.description;
}

int UnitsSchema::getNum() const
{
    return static_cast<int>(spec.num);
}
