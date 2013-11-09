%{
/* Lexer for the FreeCAD  Expression language   */
/* (c) 2010 Juergen Riegel  LGPL           */
/* (c) 2013 Eivind Kvedalen LGPL           */


/* This disables inclusion of unistd.h, which is not available under Visual C++
 * on Win32. The C++ scanner uses STL streams instead. */
#define YY_NO_UNISTD_H

%}

/*** Flex Declarations and Options ***/

/* change the name of the scanner class. */
%option prefix="ExpressionParser"

/* the manual says "somewhat more optimized" */
%option batch

/* no support for include files is planned */
%option noyywrap nounput 

DIGIT    [0-9]
ID       [a-z][a-z0-9]*


%% /*** Filter language Part ***/

 
[ \t]       ;
[\n]+       ;

[-+()=/*^,]  { return *yytext; }

"mm"                   scaler = 1.0;        unitstr = yytext; unit = Unit(1);                      return UNIT; // millimeter      (internal standard length)
"m"                    scaler = 1000.0;     unitstr = yytext; unit = Unit(1);                      return UNIT; // meter
"cm"                   scaler = 10.0;       unitstr = yytext; unit = Unit(1);                      return UNIT; // centimeter
"dm"                   scaler = 100.0;      unitstr = yytext; unit = Unit(1);                      return UNIT; // decimeter
"km"                   scaler = 1000000.0;  unitstr = yytext; unit = Unit(1);                      return UNIT; // kilometer

"in"                   scaler = 25.4;       unitstr = yytext; unit = Unit(1);                      return UNIT; // inch
"\""                   scaler = 25.4;       unitstr = yytext; unit = Unit(1);                      return UNIT; // inch
"fo"                   scaler = 304.8;      unitstr = yytext; unit = Unit(1);                      return UNIT; // foot
"'"                    scaler = 304.8;      unitstr = yytext; unit = Unit(1);                      return UNIT; // foot
"th"                   scaler = 0.0254;     unitstr = yytext; unit = Unit(1);                      return UNIT; // thou
"yr"                   scaler = 914.4;      unitstr = yytext; unit = Unit(1);                      return UNIT; // yard

"kg"                   scaler = 1.0;        unitstr = yytext; unit = Unit(0, 1);                   return UNIT; // kilogram        (internal standard mass)
"g"                    scaler = 0.001;      unitstr = yytext; unit = Unit(0, 1);                   return UNIT; // gram
"mg"                   scaler = 0.000001;   unitstr = yytext; unit = Unit(0, 1);                   return UNIT; // milligram
"t"                    scaler = 1000.0;     unitstr = yytext; unit = Unit(0, 1);                   return UNIT; // ton

"lb"                   scaler = 0.45359237; unitstr = yytext; unit = Unit(0, 1);                   return UNIT; // pound
"oz"                   scaler = 0.45359237; unitstr = yytext; unit = Unit(0, 1);                   return UNIT; // ounce
"st"                   scaler = 6.35029318; unitstr = yytext; unit = Unit(0, 1);                   return UNIT; // Stone
"cwt"                  scaler = 50.80234544;unitstr = yytext; unit = Unit(0, 1);                   return UNIT; // hundredweights

"deg"                  scaler = 1.0;        unitstr = yytext; unit = Unit(0, 0, 0, 0, 0, 0, 0, 1); return UNIT; // degree         (internal standard angle)
"rad"                  scaler = 180/M_PI;   unitstr = yytext; unit = Unit(0, 0, 0, 0, 0, 0, 0, 1); return UNIT; // radian
"gon"                  scaler = 360.0/400.0;unitstr = yytext; unit = Unit(0, 0, 0, 0, 0, 0, 0, 1); return UNIT; // gon

"ys"                   scaler = 0.000001;   unitstr = yytext; unit = Unit(0, 0, 1);                return UNIT; // microsecond
"ms"                   scaler = 0.001;      unitstr = yytext; unit = Unit(0, 0, 1);                return UNIT; // millisecond
"s"                    scaler = 1.0;        unitstr = yytext; unit = Unit(0, 0, 1);                return UNIT; // second         (internal standard time)
"min"                  scaler = 60.0;       unitstr = yytext; unit = Unit(0, 0, 1);                return UNIT; // minute
"h"                    scaler = 3600.0;     unitstr = yytext; unit = Unit(0, 0, 1);                return UNIT; // hour

"A"                    scaler = 1.0;        unitstr = yytext; unit = Unit(0, 0, 0, 1);             return UNIT; // Ampere         (internal standard electric current)
"K"                    scaler = 1.0;        unitstr = yytext; unit = Unit(0, 0, 0, 0, 1);          return UNIT; // Kelvin         (internal standard thermodynamic temperature)
"cd"                   scaler = 1.0;        unitstr = yytext; unit = Unit(0, 0, 0, 0, 0, 0, 1);    return UNIT; // Candela        (internal standard luminous intensity)
"mol"                  scaler = 1.0;        unitstr = yytext; unit = Unit(0, 0, 0, 0, 0, 1);       return UNIT; // Mole           (internal standard amount of substance)

"yl"                   scaler = 1.0;        unitstr = yytext; unit = Unit(3);                      return UNIT; // microliter mm^3(derived  standard volume)
"ml"                   scaler = 1000.0;     unitstr = yytext; unit = Unit(3);                      return UNIT; // milliliter cm^3
"l"                    scaler = 1000000.0;  unitstr = yytext; unit = Unit(3);                      return UNIT; // Liter      dm^3

{DIGIT}+"."{DIGIT}*    {yylval = new NumberExpression( DocumentObject, atof( yytext ) ); return NUM;}
{DIGIT}+               {yylval = new NumberExpression( DocumentObject, atof( yytext ) ); return NUM;}

"pi"                   {yylval = new NumberExpression( DocumentObject, M_PI )         ; return NUM;} // constant pi
"e"                    {yylval = new NumberExpression( DocumentObject, M_E )          ; return NUM;} // constant e
                     
"acos"                 return ACOS;
"asin"                 return ASIN;
"atan"                 return ATAN;
"atan2"                return ATAN2;
"cos"                  return COS;
"cosh"                 return COSH;
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

(([a-zA-Z_][a-zA-Z_0-9]*\.)?$?[a-zA-Z]+$?[0-9]+) { yylval = new VariableExpression( DocumentObject, yytext ); return LABEL; }

([a-zA-Z_][a-zA-Z_0-9]*(\.[a-zA-Z_][a-zA-Z_0-9]*)*) { yylval = new VariableExpression( DocumentObject, yytext ); return LABEL; }
