/***************************************************************************
 *   Copyright (c) 2022 WandererFan <wandererfan@gmail.com>                *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QLocale>
# include <QRegularExpression>
# include <QString>
#endif

#include <fmt/format.h>

#include <Base/Console.h>
#include <Base/UnitsApi.h>

#include "DrawViewDimension.h"
#include "DimensionFormatter.h"
#include "Preferences.h"

// TODO: Cyclic dependency issue with DrawViewDimension

using namespace TechDraw;

bool DimensionFormatter::isMultiValueSchema() const
{
    const bool angularMeasure =
        (m_dimension->Type.isValue("Angle") || m_dimension->Type.isValue("Angle3Pt"));

    return (Base::UnitsApi::isMultiUnitAngle() && angularMeasure)
        || (Base::UnitsApi::isMultiUnitLength() && !angularMeasure);
}

std::string DimensionFormatter::formatValue(const qreal value,
                                            const QString& qFormatSpec,
                                            const Format partial,
                                            const bool isDim) const
{
    const bool angularMeasure =
        m_dimension->Type.isValue("Angle") || m_dimension->Type.isValue("Angle3Pt");
    const bool areaMeasure = m_dimension->Type.isValue("Area");

    Base::Unit unit;
    if (angularMeasure) {
        unit = Base::Unit::Angle;
    }
    else if (areaMeasure) {
        unit = Base::Unit::Area;
    }
    else {
        unit = Base::Unit::Length;
    }

    Base::Quantity asQuantity {value, unit};

    QStringList qsl = getPrefixSuffixSpec(qFormatSpec);
    const std::string formatPrefix = qsl[0].toStdString();
    const std::string formatSuffix = qsl[1].toStdString();
    QString formatSpecifier = qsl[2];

    // this handles mm to inch/km/parsec etc and decimal positions but
    // won't give more than Global_Decimals precision
    std::string basicString = formatPrefix + asQuantity.getUserString() + formatSuffix;

    if (isMultiValueSchema() && partial == Format::UNALTERED) {
        return basicString;  // Don't even try to use Alt Decimals or hide units
    }

    if (formatSpecifier.isEmpty()) {
        Base::Console().warning("Warning - no numeric format in Format Spec %s - %s\n",
                                qPrintable(qFormatSpec),
                                m_dimension->getNameInDocument());
        return qFormatSpec.toStdString();
    }

    // for older TD drawings the formatSpecifier "%g" was used, but the number of decimals was
    // nevertheless limited. To keep old drawings, we limit the number of decimals too
    // if the TD preferences option to use the global decimal number is set
    // the formatSpecifier can have a prefix and/or suffix
    if (m_dimension->useDecimals()
        && formatSpecifier.contains(QStringLiteral("%g"), Qt::CaseInsensitive)) {
        const int globalPrecision = Base::UnitsApi::getDecimals();
        // change formatSpecifier to e.g. "%.2f"
        const QString newSpecifier =
            QString::fromStdString("%." + std::to_string(globalPrecision) + "f");
        formatSpecifier.replace(QStringLiteral("%g"), newSpecifier, Qt::CaseInsensitive);
    }

    // since we are not using a multiValueSchema, we know that angles are in '°' and for
    // lengths we can get the unit of measure from UnitsApi::getBasicLengthUnit.

    // TODO: check the weird schemas (MKS, Imperial1) that report different UoM for different values

    // get value in the base unit with default decimals
    // for the conversion we use the same method as in DlgUnitsCalculator::valueChanged
    // get the conversion factor for the unit
    // the result is now just val / convertValue because val is always in the base unit
    // don't do this for angular values since they are not in the BaseLengthUnit
    std::string qBasicUnit =
        angularMeasure ? "°" : Base::UnitsApi::getBasicLengthUnit();
    double userVal = asQuantity.getValue();

    if (!angularMeasure) {
        const double convertValue = Base::Quantity::parse("1" + qBasicUnit).getValue();
        userVal /= convertValue;
        if (areaMeasure) {
            userVal /= convertValue;  // divide again as area is length²
            qBasicUnit += "²";
        }
    }

    QString formattedValue = formatValueToSpec(userVal, formatSpecifier);

    // replace decimal sign if necessary
    constexpr QChar dp = QChar::fromLatin1('.');
    if (const QLocale loc; loc.decimalPoint() != dp) {
        formattedValue.replace(dp, loc.decimalPoint());
    }

    // formattedValue is now in formatSpec format with local decimal separator
    std::string formattedValueString = formattedValue.toStdString();

    if (partial == Format::UNALTERED) {
        return basicString;
    }

    if (partial == Format::FORMATTED) {
        std::string unitStr {};

        if (angularMeasure) {
            unitStr = "°";
        }
        else if ((m_dimension->showUnits() || areaMeasure)
                 && !(isDim && m_dimension->haveTolerance())) {
            unitStr = " " + qBasicUnit;
        }

        return formatPrefix + formattedValueString + unitStr + formatSuffix;
    }

    if (partial == Format::UNIT) {
        return angularMeasure || m_dimension->showUnits() || areaMeasure ? qBasicUnit : "";
    }

    return formattedValueString;
}


//! get the formatted OverTolerance value
// wf: is this a leftover from when we only had 1 tolerance instead of over/under?
std::string DimensionFormatter::getFormattedToleranceValue(const Format partial) const
{
    QString FormatSpec = QString::fromUtf8(m_dimension->FormatSpecOverTolerance.getStrValue().data());
    QString ToleranceString;

    if (m_dimension->ArbitraryTolerances.getValue())
        ToleranceString = FormatSpec;
    else
        ToleranceString = QString::fromUtf8(formatValue(m_dimension->OverTolerance.getValue(),
                                                        FormatSpec,
                                                        partial,
                                                        false).c_str());

    return ToleranceString.toStdString();
}

//! get formatted over and under tolerances
std::pair<std::string, std::string> DimensionFormatter::getFormattedToleranceValues(const Format partial) const
{
    QString underFormatSpec = QString::fromUtf8(m_dimension->FormatSpecUnderTolerance.getStrValue().data());
    QString overFormatSpec = QString::fromUtf8(m_dimension->FormatSpecOverTolerance.getStrValue().data());
    std::pair<std::string, std::string> tolerances;
    QString underTolerance, overTolerance;

    if (m_dimension->ArbitraryTolerances.getValue()) {
        underTolerance = underFormatSpec;
        overTolerance = overFormatSpec;
    } else {
        underTolerance = QString::fromUtf8(formatValue(m_dimension->UnderTolerance.getValue(),
                                                           underFormatSpec,
                                                           partial,
                                                           false).c_str());
        overTolerance = QString::fromUtf8(formatValue(m_dimension->OverTolerance.getValue(),
                                                          overFormatSpec,
                                                          partial,
                                                          false).c_str());
    }

    tolerances.first = underTolerance.toStdString();
    tolerances.second = overTolerance.toStdString();

    return tolerances;
}

//partial = 2 unit only
std::string DimensionFormatter::getFormattedDimensionValue(const Format partial) const
{
    QString qFormatSpec = QString::fromUtf8(m_dimension->FormatSpec.getStrValue().data());

    if ( (m_dimension->Arbitrary.getValue() && !m_dimension->EqualTolerance.getValue())
        || (m_dimension->Arbitrary.getValue() && m_dimension->TheoreticalExact.getValue()) ) {
        return m_dimension->FormatSpec.getStrValue();
    }

    if (m_dimension->Arbitrary.getValue()) {
        return m_dimension->FormatSpec.getStrValue();
    }

    // if there is an equal over-/undertolerance (so only 1 tolerance to show with +/-) and
    // not theoretically exact (which has no tolerance), and
    // tolerance has been specified, ie
    // (OverTolerance != 0.0 (so a tolerance has been specified) or
    // ArbitraryTolerances are specified)
    // concatenate the tolerance to dimension

    // TODO: why all this QString if returned as std::string???
    if (m_dimension->EqualTolerance.getValue() &&
        !m_dimension->TheoreticalExact.getValue() &&
        (!DrawUtil::fpCompare(m_dimension->OverTolerance.getValue(), 0.0) ||
          m_dimension->ArbitraryTolerances.getValue())) {
        QString labelText = QString::fromUtf8(formatValue(m_dimension->getDimValue(),
                                                          qFormatSpec,
                                                          Format::FORMATTED,
                                                          true).c_str()); //just the number pref/spec[unit]/suf
        QString unitText = QString::fromUtf8(formatValue(m_dimension->getDimValue(),
                                                         qFormatSpec,
                                                         Format::UNIT,
                                                         false).c_str()); //just the unit
        QString tolerance = QString::fromStdString(getFormattedToleranceValue(Format::FORMATTED).c_str());

        // tolerance might start with a plus sign that we don't want, so cut it off
        // note plus sign is not at pos = 0!
        QRegularExpression plus(QStringLiteral("^\\s*\\+"));
        tolerance.remove(plus);

        return (labelText +
                 QString::fromUtf8(" \xC2\xB1 ") +          // +/- symbol
                 tolerance).toStdString();

        // Unreachable code??
        if (partial == Format::UNIT) {
            return unitText.toStdString();
        }

        return "";
    }

    //tolerance not specified, so just format dimension value?
    std::string formattedValue = formatValue(m_dimension->getDimValue(), qFormatSpec, partial, true);

    return formattedValue;
}

// format the value using the formatSpec. Also, handle the non-standard format-
// specifiers '%w', which works as %f but without trailing zeros
// and '%r', which round the value to the given precision.
QString DimensionFormatter::formatValueToSpec(const double value, QString formatSpecifier) const
{
    QString formattedValue;

    constexpr auto format = [](QString f, double value){
        return QString::fromStdString(fmt::sprintf(f.toStdString(), value));
    };

    QRegularExpression wrRegExp(QStringLiteral("%(?<dec>.*)(?<spec>[wWrR])"));
    QRegularExpressionMatch wrMatch = wrRegExp.match(formatSpecifier);

    if (! wrMatch.hasMatch()) {
        if (isNumericFormat(formatSpecifier)) {
            formattedValue = format(formatSpecifier, value);
        }
        return formattedValue;
    }

    QString spec = wrMatch.captured(QStringLiteral("spec")).toLower();
    QString dec = wrMatch.captured(QStringLiteral("dec"));

    if (spec == QStringLiteral("w")) {
        formattedValue = format(QStringLiteral("%") + dec + QStringLiteral("f"), value);
        // First, cut trailing zeros
        while(formattedValue.endsWith(QStringLiteral("0")))
        {
            formattedValue.chop(1);
        }
        // Second, try to cut also decimal dot
        if(formattedValue.endsWith(QStringLiteral(".")))
        {
            formattedValue.chop(1);
        }
    }
    else if (spec == QStringLiteral("r")) {
        // round the value to the given precision
        double rounder = dec.toDouble();
        double roundValue = std::ceil(value / rounder) * rounder;
        // format the result with the same decimal count than the rounder
        int dotIndex = dec.indexOf(QStringLiteral("."));
        int nDecimals = 0;
        if (dotIndex >= 0){
            // remove trailing zeros to avoid decimal overwriting
            while(dec.endsWith(QStringLiteral("0")))
            {
                dec.chop(1);
            }
            nDecimals = dec.size() - dotIndex - 1;
        }
        formatSpecifier = QStringLiteral("%.") + QString::number(nDecimals) + QStringLiteral("f");
        formattedValue = format(formatSpecifier, roundValue);
    }

    return formattedValue;
}

bool DimensionFormatter::isNumericFormat(const QString& formatSpecifier) const
{
    //printf double format spec
    const QRegularExpression rxFormat(QStringLiteral("%[+-]?[0-9]*\\.*[0-9]*[aefgwAEFGW]"));
    QRegularExpressionMatch rxMatch;
    return formatSpecifier.indexOf(rxFormat, 0, &rxMatch) != -1;
}

//TODO: similar code here and above
QStringList DimensionFormatter::getPrefixSuffixSpec(const QString& fSpec) const
{
    QStringList result;
    //find the %x.y tag in FormatSpec
    QRegularExpression rxFormat(QStringLiteral("%[+-]?[0-9]*\\.*[0-9]*[aefgrwAEFGRW]")); //printf double format spec
    QRegularExpressionMatch rxMatch;
    int pos = fSpec.indexOf(rxFormat, 0, &rxMatch);
    if (pos != -1)  {
        QString match = rxMatch.captured(0);                                  //entire capture of rx
        QString formatPrefix = fSpec.left(pos);
        result.append(formatPrefix);
        QString formatSuffix = fSpec.right(fSpec.size() - pos - match.size());
        result.append(formatSuffix);
        result.append(match);
    } else {       //printf format not found!
        Base::Console().warning("Warning - no numeric format in formatSpec %s - %s\n",
                                qPrintable(fSpec), m_dimension->getNameInDocument());
        result.append(QString());
        result.append(QString());
        result.append(fSpec);
    }
    return result;
}

std::string DimensionFormatter::getDefaultFormatSpec(bool isToleranceFormat) const
{
    std::string prefFormat = Preferences::formatSpec();
    QString formatSpec;
    QString qPrefix;
    if (prefFormat.empty()) {
        QString format1 = QStringLiteral("%.");
        QString format2 = QStringLiteral("f");
        int precision;
        if (m_dimension->useDecimals()) {
            precision = Base::UnitsApi::getDecimals();
        } else {
            precision = Preferences::altDecimals();
        }
        QString formatPrecision = QString::number(precision);

        std::string prefix = m_dimension->getPrefixForDimType();

        if (!prefix.empty()) {
            qPrefix = QString::fromUtf8(prefix.data(), prefix.size());
        }

        formatSpec = qPrefix + format1 + formatPrecision + format2;
    } else {

        std::string prefix = m_dimension->getPrefixForDimType();
        qPrefix = QString::fromUtf8(prefix.data(), prefix.size());
        formatSpec = qPrefix + QString::fromStdString(prefFormat);

    }

    if (isToleranceFormat) {
        formatSpec.replace(QStringLiteral("%"), QStringLiteral("%+"));
    }

    return formatSpec.toStdString();
}

