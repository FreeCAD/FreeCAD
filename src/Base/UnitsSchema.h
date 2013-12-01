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
#include "Quantity.h"

//#include "UnitsApi.h"


namespace Base {

/** Units systems*/
enum UnitSystem {
    SI1 = 0 , /** internal (mm,kg,s) SI system (http://en.wikipedia.org/wiki/International_System_of_Units) */
    SI2 = 1 , /** MKS (m,kg,s) SI system */
    Imperial1 = 2 /** the Imperial system (http://en.wikipedia.org/wiki/Imperial_units) */
} ;
    

/** The UnitSchema class
 * The subclasses of this class define the stuff for a 
 * certain units schema. 
 */
class UnitsSchema 
{
public:
    /// this methode translate the quantity in a string as the user may expect it
	virtual QString schemaTranslate(Base::Quantity quant,double &factor,QString &unitString)=0;
};


} // namespace Base


#endif // BASE_UNITSSCHEMA_H
