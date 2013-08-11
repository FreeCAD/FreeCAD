/* Parser for the FreeCAD  Units language           */
/* (c) 2013 Juergen Riegel  LGPL                    */


/* Represents the many different ways we can access our data */
%{
       #define YYSTYPE Quantity
       #define yyparse Quantity_yyparse
       #define yyerror Quantity_yyerror
%}

     /* Bison declarations.  */
     %token UNIT NUM
     %left '-' '+'
     %left '*' '/'
     %left NEG     /* negation--unary minus */
     %right '^'    /* exponentiation */



%start input

%%

    input:     exp                			{ ScanResult = $1     ;    }
 ;     
     
     exp:      NUM                			{ $$ = $1;         	}
             | UNIT            			    { $$ = $1;       	}
             | NUM UNIT            			{ $$ = $1*$2;    	}
             | exp '+' exp        			{ $$ = $1 + $3;    	}
             | exp '-' exp        			{ $$ = $1 - $3;    	}
             | exp '*' exp        			{ $$ = $1 * $3;    	}
             | exp '/' exp        			{ $$ = $1 / $3;    	}
             | '-' exp  %prec NEG 			{ $$ = -$2;        	}
             | exp '^' NUM        			{ $$ = $1.pow($3);  }
             | '(' exp ')'        			{ $$ = $2;         	}
;            


%%
