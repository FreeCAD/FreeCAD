%{
/* Lexer for the FreeCAD  Units language   */
/* (c) 2010 Juergen Riegel  LGPL           */


/* This disables inclusion of unistd.h, which is not available under Visual C++
 * on Win32. The C++ scanner uses STL streams instead. */
#define YY_NO_UNISTD_H

%}

/*** Flex Declarations and Options ***/

/* change the name of the scanner class. */
%option prefix="UnitsApi"

/* the manual says "somewhat more optimized" */
%option batch

/* no support for include files is planned */
%option noyywrap nounput 

DIGIT    [0-9]
ID       [a-z][a-z0-9]*


%% /*** Filter language Part ***/

 
[ \t]       ;
[\n]+       ;

[-+()=/*^]  { return *yytext; }

"mm"        yylval = 1.0;        return UNIT; // millimeter      (internal standard length)
"m"         yylval = 1000.0;     return UNIT; // meter
"cm"        yylval = 10.0;       return UNIT; // centimeter
"dm"        yylval = 100.0;      return UNIT; // decimeter
"km"        yylval = 1000000.0;  return UNIT; // kilometer

"in"        yylval = 25.4;       return UNIT; // inch
"\""        yylval = 25.4;       return UNIT; // inch
"fo"        yylval = 304.8;      return UNIT; // foot
"'"         yylval = 304.8;      return UNIT; // foot
"th"        yylval = 0.0254;     return UNIT; // thou
"yr"        yylval = 914.4;      return UNIT; // yard

"kg"        yylval = 1.0;        return UNIT; // kilogram        (internal standard mass)
"g"         yylval = 0.001;      return UNIT; // gram
"mg"        yylval = 0.000001;   return UNIT; // milligram
"t"         yylval = 1000.0;     return UNIT; // ton

"lb"        yylval = 0.45359237; return UNIT; // pound
"oz"        yylval = 0.45359237; return UNIT; // ounce
"st"        yylval = 6.35029318; return UNIT; // Stone
"cwt"       yylval = 50.80234544;return UNIT; // hundredweights

"deg"       yylval = 1.0;        return UNIT; // degree         (internal standard angle)
"rad"       yylval = 180/M_PI;   return UNIT; // radian         
"gon"       yylval = 360.0/400.0;return UNIT; // gon         

"s"         yylval = 1.0;        return UNIT; // second         (internal standard time)
"min"       yylval = 60.0;       return UNIT; // minute
"h"         yylval = 3600.0;     return UNIT; // hour  

"A"         yylval = 1.0;        return UNIT; // Ampere         (internal standard electric current)
"K"         yylval = 1.0;        return UNIT; // Kelvin         (internal standard thermodynamic temperature)
"cd"        yylval = 1.0;        return UNIT; // Candela        (internal standard luminous intensity)        
"mol"       yylval = 1.0;        return UNIT; // Mole           (internal standard amount of substance)        

"yl"        yylval = 1.0;        return UNIT; // microliter mm^3(derived  standard volume)
"ml"        yylval = 1000.0;     return UNIT; // milliliter cm^3    
"l"         yylval = 1000000.0;  return UNIT; // Liter      dm^3




{DIGIT}+"."{DIGIT}*    {yylval = atof( yytext ); return NUM;}
{DIGIT}+               {yylval = atof( yytext ); return NUM;}

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


