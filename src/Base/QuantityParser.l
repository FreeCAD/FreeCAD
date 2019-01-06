%{
/* Lexer for the FreeCAD  Units language   */
/* (c) 2013 Juergen Riegel  LGPL           */

/* flex --outfile=QuantityLexer.c QuantityParser.l */

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
"m"        yylval = Quantity::Metre;               return UNIT; // metre
"km"       yylval = Quantity::KiloMetre;           return UNIT; // kilo meter

"l"        yylval = Quantity::Liter;               return UNIT; // Liter      dm^3

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
"MA"       yylval = Quantity::MegaAmpere;          return UNIT; // Mega Ampere

"K"        yylval = Quantity::Kelvin;              return UNIT; // Kelvin         (internal standard thermodynamic temperature)
"mK"       yylval = Quantity::MilliKelvin;         return UNIT; // Kelvin
"\xC2\xB5K"    yylval = Quantity::MicroKelvin;         return UNIT; // Kelvin
"uK"       yylval = Quantity::MicroKelvin;         return UNIT; // Kelvin

"mol"      yylval = Quantity::Mole;                return UNIT; // Mole           (internal standard amount of substance)

"cd"       yylval = Quantity::Candela;             return UNIT; // Candela        (internal standard luminous intensity)

"in"       yylval = Quantity::Inch;                return UNIT; // inch
"\""       yylval = Quantity::Inch;                return UNIT; // inch
"ft"       yylval = Quantity::Foot;                return UNIT; // foot
"'"        yylval = Quantity::Foot;                return UNIT; // foot
"thou"     yylval = Quantity::Thou;                return UNIT; // thou (in/1000)
"mil"      yylval = Quantity::Thou;                return UNIT; // mil  (the thou in US)
"yd"       yylval = Quantity::Yard;                return UNIT; // yard
"mi"       yylval = Quantity::Mile;                return UNIT; // mile



"lb"       yylval = Quantity::Pound;               return UNIT; // pound
"lbm"      yylval = Quantity::Pound;               return UNIT; // pound 
"oz"       yylval = Quantity::Ounce;               return UNIT; // ounce
"st"       yylval = Quantity::Stone;               return UNIT; // Stone
"cwt"      yylval = Quantity::Hundredweights;      return UNIT; // hundredweights

"lbf"      yylval = Quantity::PoundForce;          return UNIT; // pound

"N"        yylval = Quantity::Newton;              return UNIT; // Newton (kg*m/s^2)
"kN"       yylval = Quantity::KiloNewton;          return UNIT; // Newton
"MN"       yylval = Quantity::MegaNewton;          return UNIT; // Newton
"mN"       yylval = Quantity::MilliNewton;         return UNIT; // Newton

"Pa"       yylval = Quantity::Pascal;              return UNIT; // Pascal (kg/m/s^2 or N/m^2)
"kPa"      yylval = Quantity::KiloPascal;          return UNIT; // Pascal
"MPa"      yylval = Quantity::MegaPascal;          return UNIT; // Pascal
"GPa"      yylval = Quantity::GigaPascal;          return UNIT; // Pascal

"Torr"     yylval = Quantity::Torr;                return UNIT; // portion of Pascal ( 101325/760 )
"mTorr"    yylval = Quantity::mTorr;               return UNIT; //
"uTorr"    yylval = Quantity::yTorr;               return UNIT; //
"\xC2\xB5Torr"    yylval = Quantity::yTorr;               return UNIT; //

"psi"      yylval = Quantity::PSI;                 return UNIT; // pounds/in^2
"ksi"      yylval = Quantity::KSI;                 return UNIT; // 1000 x pounds/in^2

"W"        yylval = Quantity::Watt;                return UNIT; // Watt (kg*m^2/s^3)
"VA"       yylval = Quantity::VoltAmpere;          return UNIT; // VoltAmpere (kg*m^2/s^3)

"V"        yylval = Quantity::Volt;                return UNIT; // Volt (kg*m^2/A/s^3)

"J"        yylval = Quantity::Joule;               return UNIT; // Joule (kg*m^2/s^2)
"Nm"       yylval = Quantity::NewtonMeter;         return UNIT; // N*m = Joule
"VAs"      yylval = Quantity::VoltAmpereSecond;    return UNIT; // V*A*s = Joule
"CV"       yylval = Quantity::WattSecond;          return UNIT; //
"Ws"       yylval = Quantity::WattSecond;          return UNIT; // W*s = Joule

"\xC2\xB0" yylval = Quantity::Degree;              return UNIT; // degree         (internal standard angle)
"deg"      yylval = Quantity::Degree;              return UNIT; // degree         (internal standard angle)
"rad"      yylval = Quantity::Radian;              return UNIT; // radian
"gon"      yylval = Quantity::Gon;                 return UNIT; // gon
"M"        yylval = Quantity::AngMinute;           return UNIT; // minute(Angular)
"\xE2\x80\xB2" yylval = Quantity::AngMinute;       return UNIT; // minute(Angular)
"S"        yylval = Quantity::AngSecond;           return UNIT; // second(Angular) 
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
