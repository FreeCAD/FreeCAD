/* Parser for the FreeCAD  Units language           */
/* (c) 2010 Juergen Riegel  LGPL                    */


/* Represents the many different ways we can access our data */
%{
       #define YYSTYPE UnitsSignature
       #define yyparse Unit_yyparse
       #define yyerror Unit_yyerror
%}

     /* Bison declarations.  */
     %token ACOS ASIN ATAN ATAN2 COS EXP ABS MOD LOG LOG10 POW SIN SINH TAN TANH SQRT;
     %token UNIT NUM 
     %left '-' '+'
     %left '*' '/'
     %left NEG     /* negation--unary minus */
     %right '^'    /* exponentiation */



%start input

%%

    input:     exp                			{ ScanResult = $1     ;    }
 ;     

     exp:      num                          { $$ = $1;}
             | num unit                     { $$ = $1 * $2;}
             | unit                         { $$ = $1;}
     
     num:      NUM                			{ $$ = $1;         	}
             | num '+' num        			{ $$ = $1 + $3;    	}
             | num '-' num        			{ $$ = $1 - $3;    	}
             | num '*' num        			{ $$ = $1 * $3;    	}
             | num '/' num        			{ $$ = $1 / $3;    	}
             | '-' num  %prec NEG 			{ $$ = $2.neg();  	}
             | num '^' num        			{ $$ = $1.pow ($3);}
             | '(' num ')'        			{ $$ = $2;         	}

     unit:    UNIT                			{ $$ = $1;         	}
             | unit '*' unit       			{ $$ = $1 * $3;    	}
             | unit '/' unit       			{ $$ = $1 / $3;    	}
             | unit '^' NUM        			{ $$ = $1.pow ($3);}
             | '(' unit ')'        			{ $$ = $2;         	}
;            


%%
