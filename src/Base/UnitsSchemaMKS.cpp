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
#include "UnitsSchemaMKS.h"
#include <cmath>

using namespace Base;


QString UnitsSchemaMKS::schemaTranslate(Base::Quantity quant,double &factor,QString &unitString)
{
    double UnitValue = std::abs(quant.getValue());
	Unit unit = quant.getUnit();

    // now do special treatment on all cases seams nececarry:
    if(unit == Unit::Length){  // Length handling ============================
        if(UnitValue < 0.000000001){// smaller then 0.001 nm -> scientific notation
            unitString = QString::fromLatin1("mm");
            factor = 1.0;
        }else if(UnitValue < 0.001){
            unitString = QString::fromLatin1("nm");
            factor = 0.000001;
        }else if(UnitValue < 1.0){
            unitString = QString::fromUtf8("\xC2\xB5m");
            factor = 0.001;
        }else if(UnitValue < 100.0){
            unitString = QString::fromLatin1("mm");
            factor = 1.0;
        }else if(UnitValue < 10000000.0){
            unitString = QString::fromLatin1("m");
            factor = 1000.0;
        }else if(UnitValue < 100000000000.0 ){
            unitString = QString::fromLatin1("km");
            factor = 1000000.0;
        }else{ // bigger then 1000 km -> scientific notation 
            unitString = QString::fromLatin1("mm");
            factor = 1.0;
        }
    }else if (unit == Unit::Area){
        // TODO Cascade for the Areas
        // default action for all cases without special treatment:
        unitString = quant.getUnit().getString();
        factor = 1.0;
    }else if (unit == Unit::Mass){
        // TODO Cascade for the wights
        // default action for all cases without special treatment:
        unitString = quant.getUnit().getString();
        factor = 1.0;
    }else if (unit == Unit::Pressure){
        if(UnitValue < 10.0){// Pa is the smallest
            unitString = QString::fromLatin1("Pa");
            factor = 0.001;
        }else if(UnitValue < 10000.0){
            unitString = QString::fromLatin1("kPa");
            factor = 1.0;
        }else if(UnitValue < 10000000.0){
            unitString = QString::fromLatin1("GPa");
            factor = 1000.0;
        }else{ // bigger then 1000 GPa -> scientific notation 
            unitString = QString::fromLatin1("Pa");
            factor = 1.0;
        }
    }else{
        // default action for all cases without special treatment:
        unitString = quant.getUnit().getString();
        factor = 1.0;
    }
	return QString::fromUtf8("%1 %2").arg(quant.getValue() / factor).arg(unitString);
}
