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


#ifndef BASE_UNITSSCHEMAFEMMLLIMETERNEWTON_H
#define BASE_UNITSSCHEMAFEMMLLIMETERNEWTON_H

#include <QString>
#include "UnitsSchema.h"


namespace Base {


/*  Milli metric / Newton / Seconds unit schema for use in FEM.
 *  Lengths are always in mm.
 *  Mass is in t.
 *  TimeSpann in S.
 *  Thus the Force is in Newton
 */
class UnitsSchemaFemMilliMeterNewton: public UnitsSchema
{
public:
    QString schemaTranslate(const Base::Quantity& quant, double &factor, QString &unitString) override;
};


} // namespace Base


#endif // BASE_UNITSSCHEMAFEMMLLIMETERNEWTON_H
