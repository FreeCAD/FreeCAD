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
#include "UnitsSchemaInternal.h"

using namespace Base;


QString UnitsSchemaInternal::schemaTranslate(Base::Quantity quant,double &factor,QString &unitString)
{
    double UnitValue = quant.getValue();
	Unit unit = quant.getUnit();
    // sending back the choosen values:
    unitString = QString::fromAscii(quant.getUnit().getString().c_str());
    factor = 1.0;
	return QString::fromAscii("%1 %2").arg(UnitValue).arg(QString::fromAscii(unit.getString().c_str()));
}
//
//Base::Quantity UnitsSchemaInternal::schemaPrefUnit(const Base::Unit &unit,QString &outUnitString)
//{
//	if(unit == Unit::Length){
//		outUnitString = QString::fromAscii("mm");
//		return Base::Quantity(1.0,Unit::Length);
//	}else{
//		outUnitString = QString::fromAscii(unit.getString().c_str());
//		return Base::Quantity(1,unit);
//	}

//}
