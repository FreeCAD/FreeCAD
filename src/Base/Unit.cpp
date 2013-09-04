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

#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include "Unit.h"

using namespace Base;


Unit::Unit(int8_t Length,
           int8_t Mass,
           int8_t Time,
           int8_t ElectricCurrent,
           int8_t ThermodynamicTemperature,
           int8_t AmountOfSubstance,
           int8_t LuminoseIntensity,
           int8_t Angle)
{
    Sig.Length                   = Length;                   
    Sig.Mass                     = Mass;                     
    Sig.Time                     = Time;                     
    Sig.ElectricCurrent          = ElectricCurrent;          
    Sig.ThermodynamicTemperature = ThermodynamicTemperature; 
    Sig.AmountOfSubstance        = AmountOfSubstance;        
    Sig.LuminoseIntensity        = LuminoseIntensity;        
    Sig.Angle                    = Angle;                    
}


Unit::Unit()
{

}

Unit::Unit(const Unit& that)
{
    this->Sig = that.Sig;
}

Unit::Unit(const std::string& Pars)
{
   
}


Unit Unit::pow(char exp)const
{
    Unit result;
    result.Sig.Length                   = Sig.Length                    * exp;
    result.Sig.Mass                     = Sig.Mass                      * exp;
    result.Sig.Time                     = Sig.Time                      * exp;
    result.Sig.ElectricCurrent          = Sig.ElectricCurrent           * exp;
    result.Sig.ThermodynamicTemperature = Sig.ThermodynamicTemperature  * exp;
    result.Sig.AmountOfSubstance        = Sig.AmountOfSubstance         * exp;
    result.Sig.LuminoseIntensity        = Sig.LuminoseIntensity         * exp;
    result.Sig.Angle                    = Sig.Angle                     * exp;

    return result;

}

bool Unit::isEmpty(void)const
{
    return (this->Sig.Length == 0) 
        && (this->Sig.Mass == 0)
        && (this->Sig.Time == 0)
        && (this->Sig.ElectricCurrent == 0)
        && (this->Sig.ThermodynamicTemperature == 0)
        && (this->Sig.AmountOfSubstance == 0)
        && (this->Sig.LuminoseIntensity == 0)
        && (this->Sig.Angle == 0);
}

bool Unit::operator ==(const Unit& that) const
{
    return (this->Sig.Length == that.Sig.Length) 
        && (this->Sig.Mass == that.Sig.Mass)
        && (this->Sig.Time == that.Sig.Time)
        && (this->Sig.ElectricCurrent == that.Sig.ElectricCurrent)
        && (this->Sig.ThermodynamicTemperature == that.Sig.ThermodynamicTemperature)
        && (this->Sig.AmountOfSubstance == that.Sig.AmountOfSubstance)
        && (this->Sig.LuminoseIntensity == that.Sig.LuminoseIntensity)
        && (this->Sig.Angle == that.Sig.Angle);
}


Unit Unit::operator *(const Unit &right) const
{
    Unit result;
    result.Sig.Length                   = Sig.Length                    + right.Sig.Length;
    result.Sig.Mass                     = Sig.Mass                      + right.Sig.Mass;
    result.Sig.Time                     = Sig.Time                      + right.Sig.Time;
    result.Sig.ElectricCurrent          = Sig.ElectricCurrent           + right.Sig.ElectricCurrent;
    result.Sig.ThermodynamicTemperature = Sig.ThermodynamicTemperature  + right.Sig.ThermodynamicTemperature;
    result.Sig.AmountOfSubstance        = Sig.AmountOfSubstance         + right.Sig.AmountOfSubstance;
    result.Sig.LuminoseIntensity        = Sig.LuminoseIntensity         + right.Sig.LuminoseIntensity;
    result.Sig.Angle                    = Sig.Angle                     + right.Sig.Angle;

    return result;
}

Unit Unit::operator /(const Unit &right) const
{
    Unit result;
    result.Sig.Length                   = Sig.Length                    - right.Sig.Length;
    result.Sig.Mass                     = Sig.Mass                      - right.Sig.Mass;
    result.Sig.Time                     = Sig.Time                      - right.Sig.Time;
    result.Sig.ElectricCurrent          = Sig.ElectricCurrent           - right.Sig.ElectricCurrent;
    result.Sig.ThermodynamicTemperature = Sig.ThermodynamicTemperature  - right.Sig.ThermodynamicTemperature;
    result.Sig.AmountOfSubstance        = Sig.AmountOfSubstance         - right.Sig.AmountOfSubstance;
    result.Sig.LuminoseIntensity        = Sig.LuminoseIntensity         - right.Sig.LuminoseIntensity;
    result.Sig.Angle                    = Sig.Angle                     - right.Sig.Angle;

    return result;
}

Unit& Unit::operator = (const Unit &New)
{
    Sig.Length                   = New.Sig.Length;                      
    Sig.Mass                     = New.Sig.Mass   ;                 
    Sig.Time                     = New.Sig.Time    ;                
    Sig.ElectricCurrent          = New.Sig.ElectricCurrent         ;
    Sig.ThermodynamicTemperature = New.Sig.ThermodynamicTemperature;
    Sig.AmountOfSubstance        = New.Sig.AmountOfSubstance       ;
    Sig.LuminoseIntensity        = New.Sig.LuminoseIntensity       ;
    Sig.Angle                    = New.Sig.Angle    ;

    return *this;
}