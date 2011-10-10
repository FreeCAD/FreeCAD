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


#ifndef BASE_UNITSSCHEMA_H
#define BASE_UNITSSCHEMA_H


#include <string>
#include <QString>
//#include "UnitsApi.h"


namespace Base {
    
    /** Units systems*/
    enum UnitSystem { 
        SI1  = 0    , /** internal (mm,kg,s) SI system (http://en.wikipedia.org/wiki/International_System_of_Units) */  
        SI2  = 1    , /** MKS (m,kg,s) SI system  */  
        Imperial1 = 2  /** the Imperial system (http://en.wikipedia.org/wiki/Imperial_units) */  
    } ;

    /** quantity types*/
    enum QuantityType{ 
        Length      ,   
        Area        ,   
        Volume      ,   
        Angle       , 
        TimeSpan    , 
        Velocity    , 
        Acceleration, 
        Mass        ,
        Temperature
    } ;

/** The UnitSchema class
 * The subclasses of this class define the stuff for a 
 * certain units schema. 
 */
class UnitsSchema 
{
public:
    /// if called set all the units of the units schema
    virtual void setSchemaUnits(void)=0;
    /// return the value and the unit as string
    virtual void toStrWithUserPrefs(QuantityType t,double Value,QString &outValue,QString &outUnit)=0;
    /** return one string with the formated value/unit pair.
     *  The designer of the unit schema can decide how he wants the 
     *  value presented. Only rule is its still parseble by the 
     *  units parser. 
     */
    virtual QString toStrWithUserPrefs(QuantityType t,double Value)=0;
};


} // namespace Base


#endif // BASE_UNITSSCHEMA_H
