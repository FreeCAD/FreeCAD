/***************************************************************************
 *   Copyright (c) 2009 Juergen Riegel (FreeCAD@juergen-riegel.net)        *
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
# include <sstream>
#endif
#ifdef __GNUC__
# include <unistd.h>
#endif

#include <QString>
#include <QLocale>
#include "Console.h"
#include "Exception.h"
#include "UnitsApi.h"
#include "UnitsSchemaImperial1.h"
#include <cmath>
#include <iomanip>


using namespace Base;

//void UnitsSchemaImperial1::setSchemaUnits(void){
//    // here you could change the constances used by the parser (defined in Quantity.cpp)
//    Quantity::Inch =  Quantity (25.4          ,Unit(1));
//    Quantity::Foot =  Quantity (304.8         ,Unit(1));
//    Quantity::Thou =  Quantity (0.0254        ,Unit(1));
//    Quantity::Yard =  Quantity (914.4         ,Unit(1));
//    Quantity::Mile =  Quantity (1609344.0     ,Unit(1));
//}
//
//void UnitsSchemaImperial1::resetSchemaUnits(void){
//    // set units to US customary / Imperial units
//    Quantity::Inch =  Quantity (25.4          ,Unit(1));
//    Quantity::Foot =  Quantity (304.8         ,Unit(1));
//    Quantity::Thou =  Quantity (0.0254        ,Unit(1));
//    Quantity::Yard =  Quantity (914.4         ,Unit(1));
//    Quantity::Mile =  Quantity (1609344.0     ,Unit(1));
//}

QString UnitsSchemaImperial1::schemaTranslate(const Quantity &quant, double &factor, QString &unitString)
{
    double UnitValue = std::abs(quant.getValue());
    Unit unit = quant.getUnit();
    // for imperial user/programmer mind; UnitValue is in internal system, that means
    // mm/kg/s. And all combined units have to be calculated from there!

    // now do special treatment on all cases seems necessary:
    if (unit == Unit::Length) {  // Length handling ============================
        if (UnitValue < 0.00000254) {// smaller then 0.001 thou -> inch and scientific notation
            unitString = QString::fromLatin1("in");
            factor = 25.4;
        }
        else if(UnitValue < 2.54) { // smaller then 0.1 inch -> Thou (mil)
            unitString = QString::fromLatin1("thou");
            factor = 0.0254;
        }
        else if(UnitValue < 304.8) {
            unitString = QString::fromLatin1("\"");
            factor = 25.4;
        }
        else if(UnitValue < 914.4) {
            unitString = QString::fromLatin1("\'");
            factor = 304.8;
        }
        else if(UnitValue < 1609344.0) {
            unitString = QString::fromLatin1("yd");
            factor = 914.4;
        }
        else if(UnitValue < 1609344000.0) {
            unitString = QString::fromLatin1("mi");
            factor = 1609344.0;
        }
        else { // bigger then 1000 mi -> scientific notation
            unitString = QString::fromLatin1("in");
            factor = 25.4;
        }
    }
    else if (unit == Unit::Area) {
        // TODO Cascade for the Areas
        // default action for all cases without special treatment:
        unitString = QString::fromLatin1("in^2");
        factor = 645.16;
    }
    else if (unit == Unit::Volume) {
        // TODO Cascade for the Volume
        // default action for all cases without special treatment:
        unitString = QString::fromLatin1("in^3");
        factor = 16387.064;
    }
    else if (unit == Unit::Mass) {
        // TODO Cascade for the weights
        // default action for all cases without special treatment:
        unitString = QString::fromLatin1("lb");
        factor = 0.45359237;
    }
    else if (unit == Unit::Pressure) {
        if (UnitValue < 6894.744) {// psi is the smallest
            unitString = QString::fromLatin1("psi");
            factor = 6.894744825494;
        }
        else if (UnitValue < 6894744.825) {
            unitString = QString::fromLatin1("ksi");
            factor = 6894.744825494;
        }
        else { // bigger then 1000 ksi -> psi + scientific notation
            unitString = QString::fromLatin1("psi");
            factor = 6.894744825494;
        }
    }
    else if (unit == Unit::Velocity) {
        unitString = QString::fromLatin1("in/min");
        factor = 25.4/60;
    }
    else {
        // default action for all cases without special treatment:
        unitString = quant.getUnit().getString();
        factor = 1.0;
    }

    return toLocale(quant, factor, unitString);
}

QString UnitsSchemaImperialDecimal::schemaTranslate(const Base::Quantity& quant, double &factor, QString &unitString)
{
    //double UnitValue = std::abs(quant.getValue());
    Unit unit = quant.getUnit();
    // for imperial user/programmer mind; UnitValue is in internal system, that means
    // mm/kg/s. And all combined units have to be calculated from there!

    // now do special treatment on all cases seems necessary:
    if (unit == Unit::Length) {  // Length handling ============================
        unitString = QString::fromLatin1("in");
        factor = 25.4;
    }
    else if (unit == Unit::Area) {
        // TODO Cascade for the Areas
        // default action for all cases without special treatment:
        unitString = QString::fromLatin1("in^2");
        factor = 645.16;
    }
    else if (unit == Unit::Volume) {
        // TODO Cascade for the Volume
        // default action for all cases without special treatment:
        unitString = QString::fromLatin1("in^3");
        factor = 16387.064;
    }
    else if (unit == Unit::Mass) {
        // TODO Cascade for the weights
        // default action for all cases without special treatment:
        unitString = QString::fromLatin1("lb");
        factor = 0.45359237;
    }
    else if (unit == Unit::Pressure) {
        unitString = QString::fromLatin1("psi");
        factor = 6.894744825494;
    }
    else if (unit == Unit::Velocity) {
        unitString = QString::fromLatin1("in/min");
        factor = 25.4/60;
    }
    else {
        // default action for all cases without special treatment:
        unitString = quant.getUnit().getString();
        factor = 1.0;
    }

    return toLocale(quant, factor, unitString);
}

QString UnitsSchemaImperialBuilding::schemaTranslate(const Quantity &quant, double &factor, QString &unitString)
{
    // this schema expresses distances in feet + inches + fractions
    // ex: 3'- 4 1/4" with proper rounding
    Unit unit = quant.getUnit();
    if (unit == Unit::Length) {
        unitString = QString::fromLatin1("in");
        factor = 25.4;

        // Total number of inches to format
        double totalInches = std::abs(quant.getValue())/factor;

        // minimum denominator (8 for 1/8, 16 for 1/16, etc)
        int       minden;

        // Outputs
        int       feet;    // whole feet
        int       inches;  // whole inches
        int       num,den; // numerator and denominator of fractional val
        std::stringstream output; // output stream

        // Intermediate values
        int       ntot;    // total fractional units
        int       a,b,d;   // used to compute greatest common denominator
        int       tmp;     // temporary variable for GCD

        // Get the current user specified minimum denominator
        minden = quant.getFormat().getDenominator();

        // Compute and round the total number of fractional units
        ntot = (int)std::round(totalInches * (double)minden);

        // If this is zero, nothing to do but return
        if( ntot==0 )
            return QString::fromLatin1("0");

        // Compute the whole number of feet and remaining units
        feet = (int)std::floor(ntot / (12*minden));
        ntot = ntot - 12*minden*feet;

        // Compute the remaining number of whole inches
        inches = (int)std::floor(ntot/minden);

        // Lastly the fractional quantities
        num = ntot - inches*minden;
        den = minden;

        // If numerator is not zero, compute greatest common divisor and reduce
        // fraction
        if( num!=0 )
        {
            // initialize
            a = num;
            b = den;
            while (b != 0)
            {
                tmp = a % b;

                a = b;
                b = tmp;
            }
            d = a;

            num /= d;
            den /= d;
        }

        // Process into string. Start with negative sign if quantity is less
        // than zero
        if( quant.getValue() < 0 )
            output << "-";

        // Print feet if we have any
        if( feet!=0 )
        {
            output << feet << "'";

            // if there is to be trailing numbers, add space
            if( inches!=0 || num!=0 )
            {
                output << " ";
            }
        }

        // Three cases:
        //   1. Whole inches, no fraction
        //   2. Whole inches, fraction
        //   3. Fraction only
        if( inches>0 && num==0 ) // case 1.
        {
            output << inches << "\"";
        }
        else if( inches>0 && num!=0 ) // case 2
        {
            output << inches << "+" << num << "/" << den << "\"";
        }
        else if( inches==0 && num!=0 ) // case 3
        {
            output << num << "/" << den << "\"";
        }

        // Done!
        return QString::fromLatin1(output.str().c_str());
    }
    else if (unit == Unit::Area) {
        unitString = QString::fromLatin1("sqft");
        factor = 92903.04;
    }
    else if (unit == Unit::Volume) {
        unitString = QString::fromLatin1("cuft");
        factor = 28316846.592;
    }
    else if (unit == Unit::Velocity) {
        unitString = QString::fromLatin1("in/min");
        factor = 25.4/60;
    }
    else {
        unitString = quant.getUnit().getString();
        factor = 1.0;
    }

    return toLocale(quant, factor, unitString);
}

QString UnitsSchemaImperialCivil::schemaTranslate(const Base::Quantity& quant, double &factor, QString &unitString)
{
//    double UnitValue = std::abs(quant.getValue());
    Unit unit = quant.getUnit();
    // for imperial user/programmer mind; UnitValue is in internal system, that means
    // mm/kg/s. And all combined units have to be calculated from there!

    // now do special treatment on all cases seems necessary:
    if (unit == Unit::Length) {  // Length handling ============================
            unitString = QString::fromLatin1("ft");  //always ft
            factor = 304.8;      //12 * 25.4
    }
    else if (unit == Unit::Area) {
        unitString = QString::fromLatin1("ft^2");    //always sq.ft
        factor = 92903.04;
    }
    else if (unit == Unit::Volume) {
        unitString = QString::fromLatin1("ft^3");    //always cu. ft
        factor = 28316846.592;
    }
    else if (unit == Unit::Mass) {
        unitString = QString::fromLatin1("lb");     //always lbs. 
        factor = 0.45359237;
    }
    else if (unit == Unit::Pressure) {
            unitString = QString::fromLatin1("psi");
            factor = 6.894744825494;
    }
    else if (unit == Unit::Velocity) {
        unitString = QString::fromLatin1("mph");
        factor =  0.002235598;                         //1mm/sec => mph
    }
    // this schema expresses angles in degrees + minutes + seconds
    else if (unit == Unit::Angle) {
        unitString = QString::fromUtf8("deg");
        QString degreeString = QString::fromUtf8("\xC2\xB0");          //degree symbol
        QString minuteString = QString::fromUtf8("\xE2\x80\xB2");      //prime symbol
        QString secondString = QString::fromUtf8("\xE2\x80\xB3");      //double prime symbol
        factor = 1.0;                                  //1deg = 1"\xC2\xB0 "

        double totalDegrees = quant.getValue()/factor;
        double wholeDegrees = std::floor(totalDegrees);
        double sumMinutes = totalDegrees * 60.0;            //quant as minutes
        double rawMinutes = sumMinutes - wholeDegrees * 60.0;
        double wholeMinutes = std::floor(rawMinutes);
        double sumSeconds = totalDegrees * 3600.0;          //quant as seconds
        double rawSeconds = sumSeconds - (wholeDegrees * 3600.0) - (wholeMinutes * 60);
//        double wholeSeconds = std::floor(rawSeconds);
//        double remainSeconds = rawSeconds - wholeSeconds;

        int outDeg = (int) wholeDegrees;
        int outMin = (int) wholeMinutes;
        int outSec = (int) std::round(rawSeconds);

        std::stringstream output;
        output << outDeg << degreeString.toUtf8().constData();
        if ((outMin > 0) || (outSec > 0)) {
            output << outMin << minuteString.toUtf8().constData();
        }
        if (outSec > 0) {
            output << outSec << secondString.toUtf8().constData();
        }
// uncomment this for decimals on seconds
//        if (remainSeconds < (1.0 * pow(10.0,-Base::UnitsApi::getDecimals())) ) {
//            //NOP too small to display
//        } else {
//            output << std::setprecision(Base::UnitsApi::getDecimals()) << std::fixed <<
//                      rawSeconds << secondString.toStdString();
//        }
        return QString::fromUtf8(output.str().c_str());
    }
    else {
        // default action for all cases without special treatment:
        unitString = quant.getUnit().getString();
        factor = 1.0;
    }

    return toLocale(quant, factor, unitString);
}

