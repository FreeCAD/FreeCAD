/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel                                     *
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


#ifndef BASE_Unit_H
#define BASE_Unit_H

#ifdef _MSC_VER
#  include <boost/cstdint.hpp>
#else
#  include <stdint.h>
#endif

namespace Base {


struct UnitSignature{
    int32_t Length:4;
    int32_t Mass:4;
    int32_t Time:4;
    int32_t ElectricCurrent:4;
    int32_t ThermodynamicTemperature:4;
    int32_t AmountOfSubstance:4;
    int32_t LuminoseIntensity:4;
    int32_t Angle:4;
};
/**
 * The Unit class.
 */
class BaseExport Unit
{
public:
    /// default constructor
    Unit(int8_t Length,int8_t Mass=0,int8_t Time=0,int8_t ElectricCurrent=0,int8_t ThermodynamicTemperature=0,int8_t AmountOfSubstance=0,int8_t LuminoseIntensity=0,int8_t Angle=0);
    Unit(void);
    Unit(const Unit&);
    Unit(const std::string& Pars);
    /// Destruction
    ~Unit () {};


    /** Operators. */
    //@{
    Unit& operator *=(const Unit&that) const {return *this * that;}
    Unit& operator /=(const Unit&that) const {return *this / that;}
    Unit operator *(const Unit&) const;
    Unit operator /(const Unit&) const;
    bool operator ==(const Unit&) const;
    bool operator !=(const Unit&that) const {return !(*this == that);}
    Unit& operator =(const Unit&);
    Unit pow(char exp)const;
    //@}

    bool isEmpty(void)const;
    char getLengthDimension(void){return Sig.Length;}

protected:
    UnitSignature Sig;
};

} // namespace Base

#endif // BASE_Unit_H
