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
#include <Base/Tools.h>
#include <Base/UnitsApi.h>

#include "DimensionFormatter.h"
#include "Preferences.h"


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

// Todo: make this enum
//partial = 0 return the unaltered user string from the Units subsystem
//partial = 1 return value formatted according to the format spec and preferences for
//            useAltDecimals and showUnits
//partial = 2 return only the unit of measure
std::string DimensionFormatter::formatValue(const qreal value,
                                            const QString& qFormatSpec,
                                            const int partial,
                                            const bool isDim) const
{
//    Base::Console().Message("DF::formatValue() - %s isRestoring: %d\n",
//                            m_dimension->getNameInDocument(), m_dimension->isRestoring());
    bool angularMeasure = false;
    QLocale loc;

    Base::Quantity asQuantity;
    asQuantity.setValue(value);
    if ( (m_dimension->Type.isValue("Angle")) ||
         (m_dimension->Type.isValue("Angle3Pt")) ) {
        angularMeasure = true;
        asQuantity.setUnit(Base::Unit::Angle);
    } else {
        asQuantity.setUnit(Base::Unit::Length);
    }

    QString qUserString = asQuantity.getUserString();  // this handles mm to inch/km/parsec etc
                                                       // and decimal positions but won't give more than
                                                       // Global_Decimals precision

    //get formatSpec prefix/suffix/specifier
    QStringList qsl = getPrefixSuffixSpec(qFormatSpec);
    QString formatPrefix    = qsl[0];   //FormatSpec prefix
    QString formatSuffix    = qsl[1];   //FormatSpec suffix
    QString formatSpecifier = qsl[2];   //FormatSpec specifier

    QString qMultiValueStr;
    QString qBasicUnit = Base::Tools::fromStdString(Base::UnitsApi::getBasicLengthUnit());

    QString formattedValue;
    if (isMultiValueSchema() && partial == 0) {
        //handle multi value schemes (yd/ft/in, dms, etc). don't even try to use Alt Decimals or hide units
        qMultiValueStr = formatPrefix + qUserString + formatSuffix;
        return qMultiValueStr.toStdString();
    } else {
        //not multivalue schema
        if (formatSpecifier.isEmpty()) {
            Base::Console().Warning("Warning - no numeric format in Format Spec %s - %s\n",
                                    qPrintable(qFormatSpec), m_dimension->getNameInDocument());
            return Base::Tools::toStdString(qFormatSpec);
        }

        // for older TD drawings the formatSpecifier "%g" was used, but the number of decimals was
        // neverheless limited. To keep old drawings, we limit the number of decimals too
        // if the TD preferences option to use the global decimal number is set
        // the formatSpecifier can have a prefix and/or suffix
        if (m_dimension->useDecimals() && formatSpecifier.contains(QString::fromLatin1("%g"), Qt::CaseInsensitive)) {
                int globalPrecision = Base::UnitsApi::getDecimals();
                // change formatSpecifier to e.g. "%.2f"
                QString newSpecifier = QString::fromStdString("%." + std::to_string(globalPrecision) + "f");
                formatSpecifier.replace(QString::fromLatin1("%g"), newSpecifier, Qt::CaseInsensitive);
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
            qBasicUnit = QString::fromUtf8("°");
        } else {
            double convertValue = Base::Quantity::parse(QString::fromLatin1("1") + qBasicUnit).getValue();
            userVal = asQuantity.getValue() / convertValue;
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
    if (partial == 0) {   //prefix + unit subsystem string + suffix
        return Base::Tools::toStdString(formatPrefix) +
               Base::Tools::toStdString(qUserString) +
               Base::Tools::toStdString(formatSuffix);
    } else if (partial == 1)  {            // prefix number[unit] suffix
        if (angularMeasure) {
            //always insert unit after value
            return Base::Tools::toStdString(formatPrefix) +
                     formattedValueString + "°" +
                     Base::Tools::toStdString(formatSuffix);
        } else if (m_dimension->showUnits()){
            if (isDim && m_dimension->haveTolerance()) {
                //unit will be included in tolerance so don't repeat it here
                return Base::Tools::toStdString(formatPrefix) +
                         formattedValueString +
                         Base::Tools::toStdString(formatSuffix);
            } else {
                //no tolerance, so we need to include unit
                return Base::Tools::toStdString(formatPrefix) +
                         formattedValueString + " " +
                         Base::Tools::toStdString(qBasicUnit) +
                         Base::Tools::toStdString(formatSuffix);
            }
        } else {
            //showUnits is false
            return Base::Tools::toStdString(formatPrefix) +
                     formattedValueString +
                     Base::Tools::toStdString(formatSuffix);
        }
    } else if (partial == 2) {             // just the unit
        if (angularMeasure) {
            return Base::Tools::toStdString(qBasicUnit);
        } else if (m_dimension->showUnits()) {
            return Base::Tools::toStdString(qBasicUnit);
        } else {
            return "";
        }
    }

    return formattedValueString;
}


//! get the formatted OverTolerance value
// wf: is this a leftover from when we only had 1 tolerance instead of over/under?
std::string DimensionFormatter::getFormattedToleranceValue(const int partial) const
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
std::pair<std::string, std::string> DimensionFormatter::getFormattedToleranceValues(const int partial) const
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
std::string DimensionFormatter::getFormattedDimensionValue(const int partial) const
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
    if (m_dimension->EqualTolerance.getValue() &&
        !m_dimension->TheoreticalExact.getValue() &&
        (!DrawUtil::fpCompare(m_dimension->OverTolerance.getValue(), 0.0) ||
          m_dimension->ArbitraryTolerances.getValue())) {
        QString labelText = QString::fromUtf8(formatValue(m_dimension->getDimValue(),
                                                          qFormatSpec,
                                                          1,
                                                          true).c_str()); //just the number pref/spec[unit]/suf
        QString unitText = QString::fromUtf8(formatValue(m_dimension->getDimValue(),
                                                         qFormatSpec,
                                                         2,
                                                         false).c_str()); //just the unit
        QString tolerance = QString::fromStdString(getFormattedToleranceValue(1).c_str());

        // tolerance might start with a plus sign that we don't want, so cut it off
        // note plus sign is not at pos = 0!
        QRegularExpression plus(QString::fromUtf8("^\\s*\\+"));
        tolerance.remove(plus);

        return (labelText +
                 QString::fromUtf8(" \xC2\xB1 ") +          // +/- symbol
                 tolerance).toStdString();

        if (partial == 2) {
            return unitText.toStdString();
        }

        return "";
    }

    //tolerance not specified, so just format dimension value?
    std::string formattedValue = formatValue(m_dimension->getDimValue(), qFormatSpec, partial, true);

    return formattedValue;
}

// format the value using the formatSpec. Also, handle the non-standard format-
// specifier '%w', which has the following rules: works as %f, but no trailing zeros
QString DimensionFormatter::formatValueToSpec(const double value, const QString& formatSpecifier) const
{
    QString formattedValue;
    if (formatSpecifier.contains(QRegularExpression(QStringLiteral("%.*[wW]")))) {
        QString fs = formatSpecifier;
        fs.replace(QRegularExpression(QStringLiteral("%(.*)w")), QStringLiteral("%\\1f"));
        fs.replace(QRegularExpression(QStringLiteral("%(.*)W")), QStringLiteral("%\\1F"));
        formattedValue = QString::asprintf(Base::Tools::toStdString(fs).c_str(), value);
        // First, try to cut trailing zeros, if AFTER decimal dot there are nonzero numbers
        // Second, try to cut also decimal dot and zeros, if there are just zeros after it
        formattedValue.replace(QRegularExpression(QStringLiteral("([0-9][0-9]*\\.[0-9]*[1-9])00*$")), QStringLiteral("\\1"));
        formattedValue.replace(QRegularExpression(QStringLiteral("([0-9][0-9]*)\\.0*$")), QStringLiteral("\\1"));
    } else {
        if (isNumericFormat(formatSpecifier)) {
            formattedValue = QString::asprintf(Base::Tools::toStdString(formatSpecifier).c_str(), value);
        }
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
    QRegularExpression rxFormat(QStringLiteral("%[+-]?[0-9]*\\.*[0-9]*[aefgwAEFGW]")); //printf double format spec
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
        QString format1 = Base::Tools::fromStdString("%.");
        QString format2 = Base::Tools::fromStdString("f");
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
        formatSpec.replace(QString::fromUtf8("%"), QString::fromUtf8("%+"));
    }

    return Base::Tools::toStdString(formatSpec);
}

//true if value is too small to display using formatSpec
bool DimensionFormatter::isTooSmall(const double value, const QString& formatSpec) const
{
    if (TechDraw::DrawUtil::fpCompare(value, 0.0)) {
        //zero values always fit, so it isn't too small
        return false;
    }

    QRegularExpression rxFormat(QStringLiteral("%[+-]?[0-9]*\\.*([0-9]*)[aefgwAEFGW]")); //printf double format spec
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
