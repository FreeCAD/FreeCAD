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
#endif

#include <Base/Console.h>
#include <Base/UnitsApi.h>

#include "DrawViewDimension.h"
#include "DimensionFormatter.h"
#include "Preferences.h"

// TODO: Cyclic dependency issue with DrawViewDimension

using namespace TechDraw;

bool DimensionFormatter::isMultiValueSchema() const
{
    bool angularMeasure = (m_dimension->Type.isValue("Angle") ||
                           m_dimension->Type.isValue("Angle3Pt"));

    if (Base::UnitsApi::isMultiUnitAngle() &&
            angularMeasure) {
        return true;
    } else if (Base::UnitsApi::isMultiUnitLength() &&
               !angularMeasure) {
        return true;
    }
    return false;
}

std::string DimensionFormatter::formatValue(const qreal value,
                                            const QString& qFormatSpec,
                                            const Format partial,
                                            const bool isDim) const
{
//    Base::Console().Message("DF::formatValue() - %s isRestoring: %d\n",
//                            m_dimension->getNameInDocument(), m_dimension->isRestoring());
    bool angularMeasure = m_dimension->Type.isValue("Angle") || m_dimension->Type.isValue("Angle3Pt");
    bool areaMeasure = m_dimension->Type.isValue("Area");
    QLocale loc;

    Base::Quantity asQuantity;
    asQuantity.setValue(value);
    if (angularMeasure) {
        asQuantity.setUnit(Base::Unit::Angle);
    }
    else if (areaMeasure) {
        asQuantity.setUnit(Base::Unit::Area);
    }
    else {
        asQuantity.setUnit(Base::Unit::Length);
    }

    // this handles mm to inch/km/parsec etc and decimal positions but
    // won't give more than Global_Decimals precision
    QString qUserString = QString::fromStdString(asQuantity.getUserString());

    //get formatSpec prefix/suffix/specifier
    QStringList qsl = getPrefixSuffixSpec(qFormatSpec);
    QString formatPrefix    = qsl[0];   //FormatSpec prefix
    QString formatSuffix    = qsl[1];   //FormatSpec suffix
    QString formatSpecifier = qsl[2];   //FormatSpec specifier

    QString qMultiValueStr;
    QString qBasicUnit = QString::fromStdString(Base::UnitsApi::getBasicLengthUnit());

    QString formattedValue;
    if (isMultiValueSchema() && partial == Format::UNALTERED) {
        //handle multi value schemes (yd/ft/in, dms, etc). don't even try to use Alt Decimals or hide units
        qMultiValueStr = formatPrefix + qUserString + formatSuffix;
        return qMultiValueStr.toStdString();
    } else {
        //not multivalue schema
        if (formatSpecifier.isEmpty()) {
            Base::Console().Warning("Warning - no numeric format in Format Spec %s - %s\n",
                                    qPrintable(qFormatSpec), m_dimension->getNameInDocument());
            return qFormatSpec.toStdString();
        }

        // for older TD drawings the formatSpecifier "%g" was used, but the number of decimals was
        // neverheless limited. To keep old drawings, we limit the number of decimals too
        // if the TD preferences option to use the global decimal number is set
        // the formatSpecifier can have a prefix and/or suffix
        if (m_dimension->useDecimals() && formatSpecifier.contains(QStringLiteral("%g"), Qt::CaseInsensitive)) {
                int globalPrecision = Base::UnitsApi::getDecimals();
                // change formatSpecifier to e.g. "%.2f"
                QString newSpecifier = QString::fromStdString("%." + std::to_string(globalPrecision) + "f");
                formatSpecifier.replace(QStringLiteral("%g"), newSpecifier, Qt::CaseInsensitive);
        }

        // since we are not using a multiValueSchema, we know that angles are in '°' and for
        // lengths we can get the unit of measure from UnitsApi::getBasicLengthUnit.

        // TODO: check the weird schemas (MKS, Imperial1)that report different UoM
        // for different values

        // get value in the base unit with default decimals
        // for the conversion we use the same method as in DlgUnitsCalculator::valueChanged
        // get the conversion factor for the unit
        // the result is now just val / convertValue because val is always in the base unit
        // don't do this for angular values since they are not in the BaseLengthUnit
        double userVal;
        if (angularMeasure) {
            userVal = asQuantity.getValue();
            qBasicUnit = QStringLiteral("°");
        }
        else {
            double convertValue = Base::Quantity::parse("1" + qBasicUnit.toStdString()).getValue();
            userVal = asQuantity.getValue() / convertValue;
            if (areaMeasure) {
                userVal = userVal / convertValue; // divide again as area is length²
                qBasicUnit = qBasicUnit + QStringLiteral("²");
            }
        }

        if (isTooSmall(userVal, formatSpecifier)) {
            Base::Console().Warning("Dimension %s value %.6f is too small for format specifier: %s\n",
                            m_dimension->getNameInDocument(), userVal, qPrintable(formatSpecifier));
        }

        formattedValue = formatValueToSpec(userVal, formatSpecifier);

        // replace decimal sign if necessary
        QChar dp = QChar::fromLatin1('.');
        if (loc.decimalPoint() != dp) {
            formattedValue.replace(dp, loc.decimalPoint());
        }
    }

    //formattedValue is now in formatSpec format with local decimal separator
    std::string formattedValueString = formattedValue.toStdString();
    if (partial == Format::UNALTERED) {  // prefix + unit subsystem string + suffix
        return formatPrefix.toStdString() +
                 qUserString.toStdString() +
                 formatSuffix.toStdString();
    }
    else if (partial == Format::FORMATTED)  {
        if (angularMeasure) {
            //always insert unit after value
            return formatPrefix.toStdString() + formattedValueString + "°" +
                     formatSuffix.toStdString();
        }
        else if (m_dimension->showUnits() || areaMeasure){
            if (isDim && m_dimension->haveTolerance()) {
                //unit will be included in tolerance so don't repeat it here
                return formatPrefix.toStdString() +
                         formattedValueString +
                         formatSuffix.toStdString();
            }
            else {
                //no tolerance, so we need to include unit
                return formatPrefix.toStdString() +
                         formattedValueString + " " +
                         qBasicUnit.toStdString() +
                         formatSuffix.toStdString();
            }
        }
        else {
            //showUnits is false
            return formatPrefix.toStdString() +
                     formattedValueString +
                     formatSuffix.toStdString();
        }
    }
    else if (partial == Format::UNIT) {
        if (angularMeasure) {
            return qBasicUnit.toStdString();
        }
        else if (m_dimension->showUnits() || areaMeasure) {
            return qBasicUnit.toStdString();
        }
        else {
            return "";
        }
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
                 QStringLiteral(" \xC2\xB1 ") +          // +/- symbol
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
        return QString::asprintf(f.toStdString().c_str(), value);
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
    QRegularExpression rxFormat(QStringLiteral("%[+-]?[0-9]*\\.*[0-9]*[aefgwAEFGW]")); //printf double format spec
    QRegularExpressionMatch rxMatch;
    int pos = formatSpecifier.indexOf(rxFormat, 0, &rxMatch);
    if (pos != -1)  {
        return true;
    }
    return false;
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
        Base::Console().Warning("Warning - no numeric format in formatSpec %s - %s\n",
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

//true if value is too small to display using formatSpec
bool DimensionFormatter::isTooSmall(const double value, const QString& formatSpec) const
{
    if (TechDraw::DrawUtil::fpCompare(value, 0.0)) {
        //zero values always fit, so it isn't too small
        return false;
    }

    QRegularExpression rxFormat(QStringLiteral("%[+-]?[0-9]*\\.*([0-9]*)[aefgrwAEFGRW]")); //printf double format spec
    QRegularExpressionMatch rxMatch = rxFormat.match(formatSpec);
    if (rxMatch.hasMatch()) {
        QString decimalGroup = rxMatch.captured(1);
        int factor = decimalGroup.toInt();
        double minValue = pow(10.0, -factor);
        if (std::fabs(value) < minValue) {
            return true;
        }
    } else {
        Base::Console().Warning("Failed to parse dimension format spec\n");
    }
    return false;
}
