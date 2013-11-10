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

"nm"        yylval = Quantity(1.0e-6    ,Unit(1));      return UNIT; // nano meter
"ym"        yylval = Quantity(1.0e-3    ,Unit(1));      return UNIT; // micro meter
"mm"        yylval = Quantity(1.0       ,Unit(1));      return UNIT; // milli meter
"cm"        yylval = Quantity(10.0      ,Unit(1));      return UNIT; // centi meter
"dm"        yylval = Quantity(100.0     ,Unit(1));      return UNIT; // deci meter
"m"         yylval = Quantity(1.0e3     ,Unit(1));      return UNIT; // meter
"km"        yylval = Quantity(1.0e6     ,Unit(1));      return UNIT; // kilo meter
"l"         yylval = Quantity(1000000.0 ,Unit(3));      return UNIT; // Liter      dm^3

 "yg"        yylval = Quantity(1.0e-9    ,Unit(0,1));    return UNIT; // milli gram
 "mg"        yylval = Quantity(1.0e-6    ,Unit(0,1));    return UNIT; // milli gram
 "g"         yylval = Quantity(1.0e-3    ,Unit(0,1));    return UNIT; // gram
 "kg"        yylval = Quantity(1.0       ,Unit(0,1));    return UNIT; // kilo gram      (internal standard for mass) 
 "t"         yylval = Quantity(1000.0    ,Unit(0,1));    return UNIT; // ton

 "s"         yylval = Quantity(1.0       ,Unit(0,0,1));  return UNIT; // second         (internal standard time)
 "min"       yylval = Quantity(60.0      ,Unit(0,0,1));  return UNIT; // minute
 "h"         yylval = Quantity(3600.0    ,Unit(0,0,1));  return UNIT; // hour  

 "A"         yylval = Quantity(1.0       ,Unit(0,0,0,1));    return UNIT; // Ampere         (internal standard electric current)
 "mA"        yylval = Quantity(0.001     ,Unit(0,0,0,1));    return UNIT; // milli Ampere         
 "kA"        yylval = Quantity(1000.0    ,Unit(0,0,0,1));    return UNIT; // kilo Ampere         
 "MA"        yylval = Quantity(1.0e6     ,Unit(0,0,0,1));    return UNIT; // Mega Ampere         

 "K"         yylval = Quantity(1.0       ,Unit(0,0,0,0,1));  return UNIT; // Kelvin         (internal standard thermodynamic temperature)
 "mK"        yylval = Quantity(0.001     ,Unit(0,0,0,0,1));  return UNIT; // Kelvin         
 "yK"        yylval = Quantity(0.000001  ,Unit(0,0,0,0,1));  return UNIT; // Kelvin         

 "mol"       yylval = Quantity(1.0       ,Unit(0,0,0,0,0,1));    return UNIT; // Mole           (internal standard amount of substance)        

 "cd"        yylval = Quantity(1.0       ,Unit(0,0,0,0,0,0,1));  return UNIT; // Candela        (internal standard luminous intensity)        

 "deg"       yylval = Quantity(1.0           ,Unit(0,0,0,0,0,0,0,1));    return UNIT; // degree         (internal standard angle)
 "rad"       yylval = Quantity(180/M_PI      ,Unit(0,0,0,0,0,0,0,1));    return UNIT; // radian         
 "gon"       yylval = Quantity(360.0/400.0   ,Unit(0,0,0,0,0,0,0,1));    return UNIT; // gon         

 "in"        yylval = Quantity(25.4      ,Unit(1));      return UNIT; // inch
 "\""        yylval = Quantity(25.4      ,Unit(1));      return UNIT; // inch
 "fo"        yylval = Quantity(304.8     ,Unit(1));      return UNIT; // foot
 "'"         yylval = Quantity(304.8        ,Unit(1));      return UNIT; // foot
 "th"        yylval = Quantity(0.0254       ,Unit(1));      return UNIT; // thou
 "yr"       yylval = Quantity(914.4         ,Unit(1));      return UNIT; // yard


 "lb"       yylval = Quantity(0.45359237    ,Unit(0,1));   return UNIT; // pound
 "oz"       yylval = Quantity(0.45359237    ,Unit(0,1));   return UNIT; // ounce
 "st"       yylval = Quantity(6.35029318    ,Unit(0,1));   return UNIT; // Stone
 "cwt"      yylval = Quantity(50.80234544   ,Unit(0,1));   return UNIT; // hundredweights

 "N"        yylval = Quantity(1000.0        ,Unit(1,1,-2));     return UNIT; // Newton (kg*m/s^2)
 
 "Pa"       yylval = Quantity(0.001         ,Unit(-1,1,-2));    return UNIT; // Pascal (kg/m*s^2) 
 "psi"      yylval = Quantity(0.145038      ,Unit(-1,1,-2));    return UNIT; // pounds/in^2 

 "W"        yylval = Quantity(1e+6          ,Unit(2,1,-3));     return UNIT; // Watt (kg*m^2/s^3) 
 "VA"       yylval = Quantity(1e+6          ,Unit(2,1,-3));     return UNIT; // VoltAmpere (kg*m^2/s^3) 

 "J"        yylval = Quantity(1e+6          ,Unit(2,1,-2));     return UNIT; // Joule (kg*m^2/s^2) 
 "Nm"       yylval = Quantity(1e+6          ,Unit(2,1,-2));     return UNIT; // Joule (kg*m^2/s^2) 
 "VAs"      yylval = Quantity(1e+6          ,Unit(2,1,-2));     return UNIT; // Joule (kg*m^2/s^2) 
 "CV"       yylval = Quantity(1e+6          ,Unit(2,1,-2));     return UNIT; // Joule (kg*m^2/s^2) 
 "Ws"       yylval = Quantity(1e+6          ,Unit(2,1,-2));     return UNIT; // Joule (kg*m^2/s^2) 

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

