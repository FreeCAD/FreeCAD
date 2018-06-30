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
#include "Exception.h"
#include "UnitsApi.h"
#include "UnitsSchemaImperial1.h"
#include <cmath>

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
        }else if(UnitValue < 1609344.0){
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
        // TODO Cascade for the wights
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
    else{
        // default action for all cases without special treatment:
        unitString = quant.getUnit().getString();
        factor = 1.0;
    }

    return toLocale(quant, factor, unitString);
}

QString UnitsSchemaImperialDecimal::schemaTranslate(const Base::Quantity& quant, double &factor, QString &unitString)
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
        //}else if(UnitValue < 2.54){ // smaller then 0.1 inch -> Thou (mil)
        //    unitString = QString::fromLatin1("thou");
        //    factor = 0.0254;
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
        // TODO Cascade for the wights
        // default action for all cases without special treatment:
        unitString = QString::fromLatin1("lb");
        factor = 0.45359237;
    }
    else if (unit == Unit::Pressure) {
        if (UnitValue < 6894.744) {// psi is the smallest
            unitString = QString::fromLatin1("psi");
            factor = 6.894744825494;
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

QString UnitsSchemaImperialBuilding::schemaTranslate(const Quantity &quant, double &factor, QString &unitString)
{
    // this schema expresses distances in feet + inches + fractions
    // ex: 3'- 4 1/4"
    Unit unit = quant.getUnit();
    if (unit == Unit::Length) {
        unitString = QString::fromLatin1("in");
        factor = 25.4;
        double inchValue = std::abs(quant.getValue())/25.4;
        int feet = inchValue/12;
        double inchPart = inchValue - (double)feet*12;
        int inches = (int)inchPart;
        double fraction = inchPart - (int)inchPart;
        if (fraction > 0.9375) {
            inches++;
            fraction = 0.0;
        }

        // if the quantity is too small it is rounded to zero
        if (std::abs(quant.getValue()) <= 1.5875)
            return QString::fromLatin1("0");

        // build representation
        std::stringstream output;
        if (quant.getValue() < 0)
            output << "-";

        // feet
        if (feet > 0) {
            output << feet << "'";
            if ( (inches > 0) || (fraction > 0.0625) ) {
                if (quant.getValue() < 0)
                    output << " -";
                else
                    output << " ";
            }
        }

        // inches
        if (inches > 0) {
            output << inches;
            if (fraction > 0.0625) {
                if (quant.getValue() < 0)
                    output << "-";
                else
                    output << "+";
            }
            else
                output << "\"";
        }

        // fraction
        if (fraction <= 0.0625) {}
        else if (fraction > 0.8125)
            output << "7/8\"";
        else if (fraction > 0.6875)
            output << "3/4\"";
        else if (fraction > 0.5625)
            output << "5/8\"";
        else if (fraction > 0.4375)
            output << "1/2\"";
        else if (fraction > 0.3125)
            output << "3/8\"";
        else if (fraction > 0.1875)
            output << "1/4\"";
        else
            output << "1/8\"";
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
