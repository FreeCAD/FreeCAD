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


"mm"        yylval = UnitsSignature(1.0);        return UNIT; // millimeter      (internal standard length)
"m"         yylval = UnitsSignature(1000.0);     return UNIT; // meter


{DIGIT}+"."{DIGIT}*    {yylval = UnitsSignature(atof( yytext )); return NUM;}
{DIGIT}+               {yylval = UnitsSignature(atof( yytext )); return NUM;}

"pi"                   {yylval = UnitsSignature(M_PI)          ; return NUM;} // constant pi         
"e"                    {yylval = UnitsSignature(M_E)           ; return NUM;} // constant e         
                     
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


