/***************************************************************************
 *   Copyright (c) 2009 Juergen Riegel  (FreeCAD@juergen-riegel.net>       *
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


#ifndef BASE_UNITSSCHEMA_H
#define BASE_UNITSSCHEMA_H


#include <string>
#include <QString>
#include <Base/Quantity.h>


namespace Base {

/** Units systems */
enum UnitSystem {
    SI1 = 0 , /** internal (mm,kg,s) SI system (http://en.wikipedia.org/wiki/International_System_of_Units) */
    SI2 = 1 , /** MKS (m,kg,s) SI system */
    Imperial1 = 2, /** the Imperial system (http://en.wikipedia.org/wiki/Imperial_units) */
    ImperialDecimal = 3, /** Imperial with length in inch only */
    Centimeters = 4, /** All lengths in centimeters, areas and volumes in square/cubic meters */
    ImperialBuilding = 5, /** All lengths in feet + inches + fractions */
    MmMin = 6, /** Lengths in mm, Speed in mm/min. Angle in degrees. Useful for small parts & CNC */
    ImperialCivil = 7, /** Lengths in ft, Speed in ft/sec. Used in Civil Eng in North America */
    NumUnitSystemTypes // must be the last item!
};


/** The UnitSchema class
 * The subclasses of this class define the stuff for a 
 * certain units schema. 
 */
class UnitsSchema 
{
public:
    virtual ~UnitsSchema(){}
    /** Gets called if this schema gets activated.
      * Here it's theoretically possible that you can change the static factors
      * for certain units (e.g. mi = 1,8km instead of mi=1.6km).
      */
    virtual void setSchemaUnits(void){}
    /// If you use setSchemaUnits() you also have to impment this method to undo your changes!
    virtual void resetSchemaUnits(void){}

    /// This method translates the quantity in a string as the user may expect it.
    virtual QString schemaTranslate(const Base::Quantity& quant, double &factor, QString &unitString)=0;

    QString toLocale(const Base::Quantity& quant, double factor, const QString& unitString) const;
};


} // namespace Base


#endif // BASE_UNITSSCHEMA_H
