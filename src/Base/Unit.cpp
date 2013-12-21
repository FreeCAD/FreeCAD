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
# include <sstream>
# include <stdlib.h>
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
    Sig.Length                   = 0;                   
    Sig.Mass                     = 0;                     
    Sig.Time                     = 0;                     
    Sig.ElectricCurrent          = 0;          
    Sig.ThermodynamicTemperature = 0; 
    Sig.AmountOfSubstance        = 0;        
    Sig.LuminoseIntensity        = 0;        
    Sig.Angle                    = 0;                    
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

QString Unit::getString(void) const
{
	std::stringstream ret;

	if(isEmpty())
		return QString();

	if(     Sig.Length                  > 0 ||
		    Sig.Mass                    > 0 ||
		    Sig.Time                    > 0 ||
		    Sig.ElectricCurrent         > 0 ||
		    Sig.ThermodynamicTemperature> 0 ||
		    Sig.AmountOfSubstance       > 0 ||
		    Sig.LuminoseIntensity       > 0 ||
			Sig.Angle                   > 0 ){

        bool mult = false;
		if(Sig.Length > 0){
            mult = true;
			ret << "mm";
			if(Sig.Length >1)
				ret << "^" << Sig.Length;
		}
		if(Sig.Mass > 0){
            if(mult) ret<<'*';
            mult = true;
			ret << "kg";
			if(Sig.Mass >1)
				ret << "^" << Sig.Mass;
		}
		if(Sig.Time > 0){
            if(mult) ret<<'*';
            mult = true;
			ret << "s";
			if(Sig.Time >1)
				ret << "^" << Sig.Time;
		}
		if(Sig.ElectricCurrent > 0){
            if(mult) ret<<'*';
            mult = true;
			ret << "A";
			if(Sig.ElectricCurrent >1)
				ret << "^" << Sig.ElectricCurrent;
		}
		if(Sig.ThermodynamicTemperature > 0){
            if(mult) ret<<'*';
            mult = true;
			ret << "K";
			if(Sig.ThermodynamicTemperature >1)
				ret << "^" << Sig.ThermodynamicTemperature;
		}
		if(Sig.AmountOfSubstance > 0){
            if(mult) ret<<'*';
            mult = true;
			ret << "mol";
			if(Sig.AmountOfSubstance >1)
				ret << "^" << Sig.AmountOfSubstance;
		}
		if(Sig.LuminoseIntensity > 0){
            if(mult) ret<<'*';
            mult = true;
			ret << "cd";
			if(Sig.LuminoseIntensity >1)
				ret << "^" << Sig.LuminoseIntensity;
		}
		if(Sig.Angle > 0){
            if(mult) ret<<'*';
            mult = true;
			ret << "deg";
			if(Sig.Angle >1)
				ret << "^" << Sig.Angle;
		}
	}else{
		ret << "1";
	}

	if(     Sig.Length                  < 0 ||
		    Sig.Mass                    < 0 ||
		    Sig.Time                    < 0 ||
		    Sig.ElectricCurrent         < 0 ||
		    Sig.ThermodynamicTemperature< 0 ||
		    Sig.AmountOfSubstance       < 0 ||
		    Sig.LuminoseIntensity       < 0 ||
			Sig.Angle                   < 0 ){
		ret << "/";

        int nnom = Sig.Length<0?1:2 +
                    Sig.Mass<0?1:2 +
                    Sig.Time<0?1:2 +
                    Sig.ElectricCurrent<0?1:2 +
                    Sig.ThermodynamicTemperature<0?1:2 +
                    Sig.AmountOfSubstance<0?1:2 +
                    Sig.LuminoseIntensity<0?1:2 +
                    Sig.Angle<0?1:2 ;
        if (nnom > 1) ret << '(';
        bool mult=false;
		if(Sig.Length < 0){
			ret << "mm";
            mult = true;
			if(Sig.Length <-1)
				ret << "^" << abs(Sig.Length);
		}
		if(Sig.Mass < 0){
            if(mult) ret<<'*';
            mult = true;
            ret << "kg";
			if(Sig.Mass <-1)
				ret << "^" << abs(Sig.Mass);
		}
		if(Sig.Time < 0){
            if(mult) ret<<'*';
            mult = true;
			ret << "s";
			if(Sig.Time <-1)
				ret << "^" << abs(Sig.Time);
		}
		if(Sig.ElectricCurrent < 0){
            if(mult) ret<<'*';
            mult = true;
			ret << "A";
			if(Sig.ElectricCurrent <-1)
				ret << "^" << abs(Sig.ElectricCurrent);
		}
		if(Sig.ThermodynamicTemperature < 0){
            if(mult) ret<<'*';
            mult = true;
			ret << "K";
			if(Sig.ThermodynamicTemperature <-1)
				ret << "^" << abs(Sig.ThermodynamicTemperature);
		}
		if(Sig.AmountOfSubstance < 0){
            if(mult) ret<<'*';
            mult = true;
			ret << "mol";
			if(Sig.AmountOfSubstance <-1)
				ret << "^" << abs(Sig.AmountOfSubstance);
		}
		if(Sig.LuminoseIntensity < 0){
            if(mult) ret<<'*';
            mult = true;
			ret << "cd";
			if(Sig.LuminoseIntensity <-1)
				ret << "^" << abs(Sig.LuminoseIntensity);
		}
		if(Sig.Angle < 0){
            if(mult) ret<<'*';
            mult = true;
			ret << "deg";
			if(Sig.Angle <-1)
				ret << "^" << abs(Sig.Angle);
		}
        if (nnom > 1) ret << ')';
	}

    return QString::fromUtf8(ret.str().c_str());
}

QString Unit::getTypeString(void) const
{
    if(*this == Unit::Length            )       return QString::fromLatin1("Length"); else
    if(*this == Unit::Area              )       return QString::fromLatin1("Area"); else
    if(*this == Unit::Volume            )       return QString::fromLatin1("Volume"); else
    if(*this == Unit::Mass              )       return QString::fromLatin1("Mass"); else
    if(*this == Unit::Angle             )       return QString::fromLatin1("Angle"); else
    if(*this == Unit::TimeSpan          )       return QString::fromLatin1("TimeSpan"); else
    if(*this == Unit::Velocity          )       return QString::fromLatin1("Velocity"); else
    if(*this == Unit::Acceleration      )       return QString::fromLatin1("Acceleration"); else
    if(*this == Unit::Temperature       )       return QString::fromLatin1("Temperature"); else
    if(*this == Unit::ElectricCurrent   )       return QString::fromLatin1("ElectricCurrent"); else
    if(*this == Unit::AmountOfSubstance )       return QString::fromLatin1("AmountOfSubstance"); else
    if(*this == Unit::LuminoseIntensity )       return QString::fromLatin1("LuminoseIntensity"); else
    if(*this == Unit::Pressure          )       return QString::fromLatin1("Pressure"); else
    if(*this == Unit::Force             )       return QString::fromLatin1("Force"); else
    if(*this == Unit::Work              )       return QString::fromLatin1("Work"); else
    if(*this == Unit::Power             )       return QString::fromLatin1("Power"); else
    return QString();

}

Unit Unit::Length(1);
Unit Unit::Area(2);
Unit Unit::Volume(3);
Unit Unit::Mass(0,1);
Unit Unit::Angle(0,0,0,0,0,0,0,1);

Unit Unit::TimeSpan(0,0,1);
Unit Unit::Velocity(1,0,-1);
Unit Unit::Acceleration(1,0,-2);
Unit Unit::Temperature(0,0,0,0,1);

Unit Unit::ElectricCurrent(0,0,0,1);
Unit Unit::AmountOfSubstance(0,0,0,0,0,1);
Unit Unit::LuminoseIntensity(0,0,0,0,0,0,1);

Unit Unit::Stress  (-1,1,-2);  // kg/m*s^2 or N/m^2 or PSI
Unit Unit::Pressure(-1,1,-2);  // kg/m*s^2 or N/m^2 or PSI

Unit Unit::Force   (1,1,-2);
Unit Unit::Work    (2,1,-2);
Unit Unit::Power   (2,1,-3);
