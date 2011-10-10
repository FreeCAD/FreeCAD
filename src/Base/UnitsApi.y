/* Parser for the FreeCAD  Units language           */
/* (c) 2010 Juergen Riegel  LGPL                    */


/* Represents the many different ways we can access our data */
%{
       #define YYSTYPE double
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
     
     exp:      NUM                			{ $$ = $1;         	}
             | UNIT            			    { $$ = $1;UU=true; 	}
             | NUM UNIT            			{ $$ = $1*$2;UU=true; 	}
             | exp '+' exp        			{ $$ = $1 + $3;    	}
             | exp '-' exp        			{ $$ = $1 - $3;    	}
             | exp '*' exp        			{ $$ = $1 * $3;    	}
             | exp '/' exp        			{ $$ = $1 / $3;    	}
             | '-' exp  %prec NEG 			{ $$ = -$2;        	}
             | exp '^' exp        			{ $$ = pow ($1, $3);}
             | '(' exp ')'        			{ $$ = $2;         	}
             | ACOS  '(' exp ')'  			{ $$ = acos($3);   	}
             | ASIN  '(' exp ')'  			{ $$ = asin($3);   	}
             | ATAN  '(' exp ')'  			{ $$ = atan($3);   	}
             | ATAN2 '(' exp ',' exp ')'    { $$ = atan2($3,$5);}
             | ABS  '(' exp ')'   			{ $$ = fabs($3);   	}
             | EXP  '(' exp ')'   			{ $$ = exp($3);    	}
             | MOD  '(' exp ',' exp ')'		{ $$ = fmod($3,$5); }
             | LOG  '(' exp ')'				{ $$ = log($3);     }
             | LOG10  '(' exp ')'			{ $$ = log10($3);   }
             | POW  '(' exp ',' exp ')'		{ $$ = pow($3,$5);  }
             | SIN  '(' exp ')'   			{ $$ = sin($3);     }
             | SINH '(' exp ')'   			{ $$ = sinh($3);    }
             | TAN  '(' exp ')'   			{ $$ = tan($3);     }
             | TANH  '(' exp ')'   			{ $$ = tanh($3);    }
             | SQRT  '(' exp ')'   			{ $$ = tanh($3);    }
             | COS  '(' exp ')'   			{ $$ = cos($3);    }
             | exp exp             			{ $$ = $1 * $2;    	}
;            


%%
