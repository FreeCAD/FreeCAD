/***************************************************************************
 *   Copyright (c) 2009 Jürgen Riegel <FreeCAD@juergen-riegel.net>         *
 *   Copyright (c) 2020 Bernd Hahnebach <bernd@bimstatik.org>              *
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

#include "UnitsSchemaFemMilliMeterNewton.h"


using namespace Base;


QString UnitsSchemaFemMilliMeterNewton::schemaTranslate(const Quantity &quant, double &factor, QString &unitString)
{
    Unit unit = quant.getUnit();
    if (unit == Unit::Length) {
        // all length units in millimeters
        unitString = QString::fromLatin1("mm");
        factor = 1.0;
    }
    else if (unit == Unit::Mass) {
        // all mass units in t
        unitString = QString::fromUtf8("t");
        factor = 1e3;
    }
    else {
        // default action for all cases without special treatment:
        unitString = quant.getUnit().getString();
        factor = 1.0;
    }
    return toLocale(quant, factor, unitString);
}
