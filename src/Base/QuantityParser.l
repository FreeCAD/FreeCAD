%{
/* Lexer for the FreeCAD  Units language   */
/* (c) 2013 Juergen Riegel  LGPL           */


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

%x C_COMMENT

%% /*** Filter language Part ***/

"\["            { BEGIN(C_COMMENT); }
<C_COMMENT>"\]" { BEGIN(INITIAL); }
<C_COMMENT>.    { ;}

 
[ \t]       ;
[\n]+       ;

[-+()=/*^]  { return *yytext; }

 "nm"       yylval = Quantity::NanoMeter;           return UNIT; // nano meter
 "ym"       yylval = Quantity::MicroMeter;          return UNIT; // micro meter
 "mm"       yylval = Quantity::MilliMeter;          return UNIT; // milli meter    (internal standard length)
 "cm"       yylval = Quantity::CentiMeter;          return UNIT; // centi meter
 "dm"       yylval = Quantity::DeciMeter;           return UNIT; // deci meter
 "m"        yylval = Quantity::Meter;               return UNIT; // meter
 "km"       yylval = Quantity::KiloMeter;           return UNIT; // kilo meter

 "l"        yylval = Quantity::Liter;               return UNIT; // Liter      dm^3
 
 "yg"       yylval = Quantity::MicroGram;           return UNIT; // micro gram
 "mg"       yylval = Quantity::MilliGram;           return UNIT; // milli gram
 "g"        yylval = Quantity::Gram;                return UNIT; // gram
 "kg"       yylval = Quantity::KiloGram;            return UNIT; // kilo gram      (internal standard for mass) 
 "t"        yylval = Quantity::Ton;                 return UNIT; // ton
 
 "s"        yylval = Quantity::Second;              return UNIT; // second         (internal standard time)
 "min"      yylval = Quantity::Minut;               return UNIT; // minute
 "h"        yylval = Quantity::Hour;                return UNIT; // hour  
 
 "A"        yylval = Quantity::Ampere;              return UNIT; // Ampere         (internal standard electric current)
 "mA"       yylval = Quantity::MilliAmpere;         return UNIT; // milli Ampere         
 "kA"       yylval = Quantity::KiloAmpere;          return UNIT; // kilo Ampere         
 "MA"       yylval = Quantity::MegaAmpere;          return UNIT; // Mega Ampere         
 
 "K"        yylval = Quantity::Kelvin;              return UNIT; // Kelvin         (internal standard thermodynamic temperature)
 "mK"       yylval = Quantity::MilliKelvin;         return UNIT; // Kelvin         
 "yK"       yylval = Quantity::MicroKelvin;         return UNIT; // Kelvin         
 
 "mol"      yylval = Quantity::Mole;                return UNIT; // Mole           (internal standard amount of substance)        
 
 "cd"       yylval = Quantity::Candela;             return UNIT; // Candela        (internal standard luminous intensity)        
 
 "in"       yylval = Quantity::Inch;                return UNIT; // inch
 "\""       yylval = Quantity::Inch;                return UNIT; // inch
 "fo"       yylval = Quantity::Foot;                return UNIT; // foot
 "'"        yylval = Quantity::Foot;                return UNIT; // foot
 "th"       yylval = Quantity::Thou;                return UNIT; // thou
 "yr"       yylval = Quantity::Yard;                return UNIT; // yard
 "mile"     yylval = Quantity::Mile;                return UNIT; // mile
 
 
 "lb"       yylval = Quantity::Pound;               return UNIT; // pound
 "oz"       yylval = Quantity::Ounce;               return UNIT; // ounce
 "st"       yylval = Quantity::Stone;               return UNIT; // Stone
 "cwt"      yylval = Quantity::Hundredweights;      return UNIT; // hundredweights
 
 "N"        yylval = Quantity::Newton;              return UNIT; // Newton (kg*m/s^2)
 "kN"       yylval = Quantity::KiloNewton;          return UNIT; // Newton 
 "MN"       yylval = Quantity::MegaNewton;          return UNIT; // Newton 
 "mN"       yylval = Quantity::MilliNewton;         return UNIT; // Newton 
 
 "Pa"       yylval = Quantity::Pascal;              return UNIT; // Pascal (kg/m*s^2 or N/m^2) 
 "kPa"      yylval = Quantity::KiloPascal;          return UNIT; // Pascal 
 "MPa"      yylval = Quantity::MegaPascal;          return UNIT; // Pascal  
 "GPa"      yylval = Quantity::GigaPascal;          return UNIT; // Pascal  
 
 "psi"      yylval = Quantity::PSI;                 return UNIT; // pounds/in^2 
 
 "W"        yylval = Quantity::Watt;                return UNIT; // Watt (kg*m^2/s^3) 
 "VA"       yylval = Quantity::VoltAmpere;          return UNIT; // VoltAmpere (kg*m^2/s^3) 
 
 "J"        yylval = Quantity::Joul;                return UNIT; // Joule (kg*m^2/s^2) 
 "Nm"       yylval = Quantity::NewtonMeter;         return UNIT; // Joule (kg*m^2/s^2) 
 "VAs"      yylval = Quantity::VoltAmpereSecond;    return UNIT; // Joule (kg*m^2/s^2) 
 "CV"       yylval = Quantity::WattSecond;          return UNIT; // Joule (kg*m^2/s^2) 
 "Ws"       yylval = Quantity::WattSecond;          return UNIT; // Joule (kg*m^2/s^2) 
 
 "deg"      yylval = Quantity::Degree;              return UNIT; // degree         (internal standard angle)
 "rad"      yylval = Quantity::Radian;              return UNIT; // radian         
 "gon"      yylval = Quantity::Gon;                 return UNIT; // gon         

{DIGIT}+["."","]{DIGIT}*[eE][-+]?[0-9]+     {for(char* c=yytext;*c!='\0';c++)if(*c==',')*c='.'; yylval = atof( yytext ); return NUM;}
{DIGIT}+["."","]{DIGIT}*                    {for(char* c=yytext;*c!='\0';c++)if(*c==',')*c='.'; yylval = atof( yytext ); return NUM;}
{DIGIT}+                                    {yylval = atof( yytext ); return NUM;}

"pi"                   {yylval = M_PI          ; return NUM;} // constant pi         
"e"                    {yylval = M_E           ; return NUM;} // constant e         
                     
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

