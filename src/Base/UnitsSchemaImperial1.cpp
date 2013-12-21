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
#ifdef __GNUC__
# include <unistd.h>
#endif

#include <QString>
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

QString UnitsSchemaImperial1::schemaTranslate(Base::Quantity quant,double &factor,QString &unitString)
{
    double UnitValue = std::abs(quant.getValue());
	Unit unit = quant.getUnit();
    // for imperial user/programmer mind; UnitValue is in internal system, that means
    // mm/kg/s. And all combined units have to be calculated from there! 

    // now do special treatment on all cases seems necessary:
    if(unit == Unit::Length){  // Length handling ============================
        if(UnitValue < 0.00000254){// smaller then 0.001 thou -> inch and scientific notation
            unitString = QString::fromLatin1("in");
            factor = 25.4;
        }else if(UnitValue < 2.54){ // smaller then 0.1 inch -> Thou (mil)
            unitString = QString::fromLatin1("thou");
            factor = 0.0254;
        }else if(UnitValue < 304.8){ 
            unitString = QString::fromLatin1("\"");
            factor = 25.4;
        }else if(UnitValue < 914.4){
            unitString = QString::fromLatin1("\'");
            factor = 304.8;
        }else if(UnitValue < 1609344.0){
            unitString = QString::fromLatin1("yd");
            factor = 914.4;
        }else if(UnitValue < 1609344000.0 ){
            unitString = QString::fromLatin1("mi");
            factor = 1609344.0;
        }else{ // bigger then 1000 mi -> scientific notation 
            unitString = QString::fromLatin1("in");
            factor = 25.4;
        }
    }else if (unit == Unit::Area){
        // TODO Cascade for the Areas
        // default action for all cases without special treatment:
        unitString = QString::fromLatin1("in^2");
        factor = 645.16;
    }else if (unit == Unit::Volume){
        // TODO Cascade for the Volume
        // default action for all cases without special treatment:
        unitString = QString::fromLatin1("in^3");
        factor = 16387.064;
    }else if (unit == Unit::Mass){
        // TODO Cascade for the wights
        // default action for all cases without special treatment:
        unitString = QString::fromLatin1("lb");
        factor = 0.45359237;
    }else if (unit == Unit::Pressure){
        if(UnitValue < 145.038){// psi is the smallest
            unitString = QString::fromLatin1("psi");
            factor = 0.145038;
        }else if(UnitValue < 145038){
            unitString = QString::fromLatin1("ksi");
            factor = 145.038;
        }else{ // bigger then 1000 ksi -> psi + scientific notation 
            unitString = QString::fromLatin1("psi");
            factor = 0.145038;
        }
    }else{
        // default action for all cases without special treatment:
        unitString = quant.getUnit().getString();
        factor = 1.0;
    }
	return QString::fromLatin1("%1 %2").arg(quant.getValue() / factor).arg(unitString);
}
