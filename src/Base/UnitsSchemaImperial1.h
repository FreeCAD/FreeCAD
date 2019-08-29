/***************************************************************************
 *   Copyright (c) 2009 Juergen Riegel  (FreeCAD@juergen-riegel.net>              *
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


#ifndef BASE_UNITSSCHEMAIMPERIAL1_H
#define BASE_UNITSSCHEMAIMPERIAL1_H


#include <string>
#include <QString>
#include "UnitsSchema.h"

namespace Base {
    

/** The schema class for the imperial unit system
 *  Here are the definitions for the imperial unit system.
 *  It also defines how the value/units get printed.
 */
class UnitsSchemaImperial1: public UnitsSchema
{
public:
    //virtual void setSchemaUnits(void);
    //virtual void resetSchemaUnits(void);
    virtual QString schemaTranslate(const Base::Quantity& quant, double &factor, QString &unitString);
};

/** The schema class for the imperial unit system
 *  Here are the definitions for the imperial unit system.
 *  It also defines how the value/units get printed.
 */
class UnitsSchemaImperialDecimal: public UnitsSchema
{
public:
    //virtual void setSchemaUnits(void);
    //virtual void resetSchemaUnits(void);
    virtual QString schemaTranslate(const Base::Quantity& quant, double &factor, QString &unitString);
};

/** The schema class for the imperial unit system
 *  Here are the definitions for the imperial unit system.
 *  It also defines how the value/units get printed.
 */
class UnitsSchemaImperialBuilding: public UnitsSchema
{
public:
    //virtual void setSchemaUnits(void);
    //virtual void resetSchemaUnits(void);
    virtual QString schemaTranslate(const Base::Quantity& quant, double &factor, QString &unitString);
};

/** The schema class for Civil Engineering in the imperial unit system
 *  All measurements in ft, ft^2, ft^3, ft/sec.  
 *  Pressure is in psi.
 */
class UnitsSchemaImperialCivil: public UnitsSchema
{
public:
    //virtual void setSchemaUnits(void);
    //virtual void resetSchemaUnits(void);
    virtual QString schemaTranslate(const Base::Quantity& quant, double &factor, QString &unitString);
};


} // namespace Base


#endif // BASE_UNITSSCHEMAIMPERIAL1_H
