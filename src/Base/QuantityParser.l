%{
/***************************************************************************
 *   Copyright (c) 2013 Jürgen Riegel <juergen.riegel@web.de>              *
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

/* Lexer for the FreeCAD  Units language   */

/* use this file to generate the file 'QuantityLexer.c' using the program flex
 * the command for this operation is:
 * flex --outfile=QuantityLexer.c QuantityParser.l
 * (flex for Windows is available here:
 *  https://sourceforge.net/projects/winflexbison/
 *  (you must then change 'flex' to 'win_flex' in the command)) */

/* This disables inclusion of unistd.h, which is not available under Visual C++
 * on Win32. The C++ scanner uses STL streams instead. */
#define YY_NO_UNISTD_H

%}

/*** Flex Declarations and Options ***/


/* the manual says "somewhat more optimized" */
%option batch
%option never-interactive


/* no support for include files is planned */
%option noyywrap nounput

DIGIT    [0-9]
ID       [a-z][a-z0-9]*
EXPO     [eE][-+]?[0-9]+
DGRP     '\.'[0-9][0-9][0-9]
CGRP     '\,'[0-9][0-9][0-9]

%x C_COMMENT

%% /*** Filter language Part ***/

"\["            { BEGIN(C_COMMENT); }
<C_COMMENT>"\]" { BEGIN(INITIAL); }
<C_COMMENT>.    { ;}


[ \t]       ;
[\n]+       ;

[+()=/*^]  { return *yytext; }

"-"                                                return MINUSSIGN;
"\xe2\x88\x92"                                     return MINUSSIGN;

"nm"       yylval = Quantity::NanoMetre;           return UNIT; // nano meter
"um"       yylval = Quantity::MicroMetre;          return UNIT; // micro meter
"\xC2\xB5m"    yylval = Quantity::MicroMetre;          return UNIT; // micro meter    (greek micro in UTF8)
"mm"       yylval = Quantity::MilliMetre;          return UNIT; // milli meter    (internal standard length)
"cm"       yylval = Quantity::CentiMetre;          return UNIT; // centi meter
"dm"       yylval = Quantity::DeciMetre;           return UNIT; // deci meter
"m"        yylval = Quantity::Metre;               return UNIT; // Metre
"km"       yylval = Quantity::KiloMetre;           return UNIT; // kilo meter

"l"        yylval = Quantity::Liter;               return UNIT; // Liter  (dm^3)
"ml"       yylval = Quantity::MilliLiter;          return UNIT; // milli Liter

"Hz"       yylval = Quantity::Hertz;               return UNIT; // Hertz
"kHz"      yylval = Quantity::KiloHertz;           return UNIT; // kilo Hertz
"MHz"      yylval = Quantity::MegaHertz;           return UNIT; // mega Hertz
"GHz"      yylval = Quantity::GigaHertz;           return UNIT; // giga Hertz
"THz"      yylval = Quantity::TeraHertz;           return UNIT; // tera Hertz

"ug"       yylval = Quantity::MicroGram;           return UNIT; // micro gram
"\xC2\xB5g"    yylval = Quantity::MicroGram;           return UNIT; // micro gram
"mg"       yylval = Quantity::MilliGram;           return UNIT; // milli gram
"g"        yylval = Quantity::Gram;                return UNIT; // gram
"kg"       yylval = Quantity::KiloGram;            return UNIT; // kilo gram      (internal standard for mass)
"t"        yylval = Quantity::Ton;                 return UNIT; // Metric Tonne

"s"        yylval = Quantity::Second;              return UNIT; // second         (internal standard time)
"min"      yylval = Quantity::Minute;              return UNIT; // minute
"h"        yylval = Quantity::Hour;                return UNIT; // hour

"A"        yylval = Quantity::Ampere;              return UNIT; // Ampere         (internal standard electric current)
"mA"       yylval = Quantity::MilliAmpere;         return UNIT; // milli Ampere
"kA"       yylval = Quantity::KiloAmpere;          return UNIT; // kilo Ampere
"MA"       yylval = Quantity::MegaAmpere;          return UNIT; // mega Ampere

"K"        yylval = Quantity::Kelvin;              return UNIT; // Kelvin         (internal standard thermodynamic temperature)
"mK"       yylval = Quantity::MilliKelvin;         return UNIT; // milli Kelvin
"\xC2\xB5K"    yylval = Quantity::MicroKelvin;         return UNIT; // micro Kelvin
"uK"       yylval = Quantity::MicroKelvin;         return UNIT; // micro Kelvin

"mol"      yylval = Quantity::Mole;                return UNIT; // Mole           (internal standard amount of substance)
"mmol"     yylval = Quantity::MilliMole;           return UNIT; // Milli Mole

"cd"       yylval = Quantity::Candela;             return UNIT; // Candela        (internal standard luminous intensity)

"in"       yylval = Quantity::Inch;                return UNIT; // inch
"\""       yylval = Quantity::Inch;                return UNIT; // inch
"ft"       yylval = Quantity::Foot;                return UNIT; // foot
"'"        yylval = Quantity::Foot;                return UNIT; // foot
"thou"     yylval = Quantity::Thou;                return UNIT; // thou (in/1000)
"mil"      yylval = Quantity::Thou;                return UNIT; // mil  (the thou in US)
"yd"       yylval = Quantity::Yard;                return UNIT; // yard
"mi"       yylval = Quantity::Mile;                return UNIT; // mile

"mph"      yylval = Quantity::MilePerHour;         return UNIT; // mile per hour
"sqft"     yylval = Quantity::SquareFoot;          return UNIT; // square foot
"cft"      yylval = Quantity::CubicFoot;           return UNIT; // cubic foot

"lb"       yylval = Quantity::Pound;               return UNIT; // pound
"lbm"      yylval = Quantity::Pound;               return UNIT; // pound
"oz"       yylval = Quantity::Ounce;               return UNIT; // ounce
"st"       yylval = Quantity::Stone;               return UNIT; // Stone
"cwt"      yylval = Quantity::Hundredweights;      return UNIT; // hundredweights

"lbf"      yylval = Quantity::PoundForce;          return UNIT; // pound

"N"        yylval = Quantity::Newton;              return UNIT; // Newton (kg*m/s^2)
"mN"       yylval = Quantity::MilliNewton;         return UNIT; // milli Newton
"kN"       yylval = Quantity::KiloNewton;          return UNIT; // kilo Newton
"MN"       yylval = Quantity::MegaNewton;          return UNIT; // mega Newton

"Pa"       yylval = Quantity::Pascal;              return UNIT; // Pascal (kg/m/s^2 or N/m^2)
"kPa"      yylval = Quantity::KiloPascal;          return UNIT; // kilo Pascal
"MPa"      yylval = Quantity::MegaPascal;          return UNIT; // mega Pascal
"GPa"      yylval = Quantity::GigaPascal;          return UNIT; // giga Pascal

"bar"      yylval = Quantity::Bar;                 return UNIT; // 1 bar = 100 kPa
"mbar"     yylval = Quantity::MilliBar;            return UNIT; // milli Bar

"Torr"     yylval = Quantity::Torr;                return UNIT; // portion of Pascal ( 101325/760 )
"mTorr"    yylval = Quantity::mTorr;               return UNIT; //
"uTorr"    yylval = Quantity::yTorr;               return UNIT; //
"\xC2\xB5Torr"    yylval = Quantity::yTorr;               return UNIT; //

"psi"      yylval = Quantity::PSI;                 return UNIT; // pounds/in^2
"ksi"      yylval = Quantity::KSI;                 return UNIT; // 1000 x pounds/in^2
"Mpsi"     yylval = Quantity::MPSI;                return UNIT; // 1000 ksi

"W"        yylval = Quantity::Watt;                return UNIT; // Watt (kg*m^2/s^3)
"mW"       yylval = Quantity::MilliWatt;           return UNIT; // milli Watt
"kW"       yylval = Quantity::KiloWatt;            return UNIT; // kilo Watt
"VA"       yylval = Quantity::VoltAmpere;          return UNIT; // VoltAmpere (kg*m^2/s^3)

"V"        yylval = Quantity::Volt;                return UNIT; // Volt (kg*m^2/A/s^3)
"kV"       yylval = Quantity::KiloVolt;            return UNIT; // kilo Volt
"mV"       yylval = Quantity::MilliVolt;           return UNIT; // milli Volt

"MS"       yylval = Quantity::MegaSiemens;         return UNIT; // mega Siemens
"kS"       yylval = Quantity::KiloSiemens;         return UNIT; // kilo Siemens
"S"        yylval = Quantity::Siemens;             return UNIT; // Siemens (A^2*s^3/kg/m^2)
"mS"       yylval = Quantity::MilliSiemens;        return UNIT; // milli Siemens
"\xC2\xB5S"     yylval = Quantity::MicroSiemens;   return UNIT; // micro Siemens
"uS"       yylval = Quantity::MicroSiemens;        return UNIT; // micro Siemens

"Ohm"      yylval = Quantity::Ohm;                 return UNIT; // Ohm (kg*m^2/A^2/s^3)
"kOhm"     yylval = Quantity::KiloOhm;             return UNIT; // kilo Ohm
"MOhm"     yylval = Quantity::MegaOhm;             return UNIT; // mega Ohm

"C"        yylval = Quantity::Coulomb;             return UNIT; // Coulomb (A*s)

"T"        yylval = Quantity::Tesla;               return UNIT; // Tesla (kg/s^2/A)
"G"        yylval = Quantity::Gauss;               return UNIT; // Gauss (1 G = 1e-4 T)

"Wb"       yylval = Quantity::Weber;               return UNIT; // Weber (kg*m^2/s^2/A)

"F"        yylval = Quantity::Farad;               return UNIT; // Farad (s^4*A^2/m^2/kg)
"mF"       yylval = Quantity::MilliFarad;          return UNIT; // milli Farad
"\xC2\xB5F"    yylval = Quantity::MicroFarad;      return UNIT; // micro Farad
"uF"       yylval = Quantity::MicroFarad;          return UNIT; // micro Farad
"nF"       yylval = Quantity::NanoFarad;           return UNIT; // nano Farad
"pF"       yylval = Quantity::PicoFarad;           return UNIT; // pico Farad

"H"        yylval = Quantity::Henry;               return UNIT; // Henry (kg*m^2/s^2/A^2)
"mH"       yylval = Quantity::MilliHenry;          return UNIT; // milli Henry
"\xC2\xB5H"    yylval = Quantity::MicroHenry;      return UNIT; // micro Henry
"uH"       yylval = Quantity::MicroHenry;          return UNIT; // micro Henry
"nH"       yylval = Quantity::NanoHenry;           return UNIT; // nano Henry

"J"        yylval = Quantity::Joule;               return UNIT; // Joule (kg*m^2/s^2)
"mJ"       yylval = Quantity::MilliJoule;          return UNIT; // milli Joule
"kJ"       yylval = Quantity::KiloJoule;           return UNIT; // kilo Joule
"Nm"       yylval = Quantity::NewtonMeter;         return UNIT; // N*m = Joule
"VAs"      yylval = Quantity::VoltAmpereSecond;    return UNIT; // V*A*s = Joule
"CV"       yylval = Quantity::WattSecond;          return UNIT; //
"Ws"       yylval = Quantity::WattSecond;          return UNIT; // W*s = Joule
"kWh"      yylval = Quantity::KiloWattHour;        return UNIT; // 1 kWh = 3.6e6 J
"eV"       yylval = Quantity::ElectronVolt;        return UNIT; // 1 eV = 1.602176634e-19 J
"keV"      yylval = Quantity::KiloElectronVolt;    return UNIT;
"MeV"      yylval = Quantity::MegaElectronVolt;    return UNIT;
"cal"      yylval = Quantity::Calorie;             return UNIT; // 1 cal = 4.1868 J
"kcal"     yylval = Quantity::KiloCalorie;         return UNIT;

"\xC2\xB0" yylval = Quantity::Degree;              return UNIT; // degree         (internal standard angle)
"deg"      yylval = Quantity::Degree;              return UNIT; // degree         (internal standard angle)
"rad"      yylval = Quantity::Radian;              return UNIT; // radian
"gon"      yylval = Quantity::Gon;                 return UNIT; // gon
"M"        yylval = Quantity::AngMinute;           return UNIT; // minute(Angular)
"\xE2\x80\xB2" yylval = Quantity::AngMinute;       return UNIT; // minute(Angular)
"AS"       yylval = Quantity::AngSecond;           return UNIT; // second(Angular)
"\xE2\x80\xB3" yylval = Quantity::AngSecond;       return UNIT; // second(Angular)

"1"        yylval = Quantity(1.0);                 return ONE;
{DIGIT}+"."?{DIGIT}*{EXPO}?                         {  yylval = Quantity(num_change(yytext,'.',','));return NUM;  }
"."?{DIGIT}+{EXPO}?                                 {  yylval = Quantity(num_change(yytext,'.',','));return NUM;  }
{DIGIT}+","?{DIGIT}*{EXPO}?                         {  yylval = Quantity(num_change(yytext,',','.'));return NUM;  }
","?{DIGIT}+{EXPO}?                                 {  yylval = Quantity(num_change(yytext,',','.'));return NUM;  }


"pi"                   {yylval = Quantity(M_PI)          ; return NUM;} // constant pi
"e"                    {yylval = Quantity(M_E)           ; return NUM;} // constant e

"acos"                 return ACOS;
"asin"                 return ASIN;
"atan"                 return ATAN;
"atan2"                return ATAN2;
"cos"                  return COS;
"exp"                  return EXP;
"abs"                  return ABS;
"mod"                  return MOD;
"log"                  return LOG;
"log10"                return LOG10;
"pow"                  return POW;
"sin"                  return SIN;
"sinh"                 return SINH;
"tan"                  return TAN;
"tanh"                 return TANH;
"sqrt"                 return SQRT;

.                      return *yytext;
