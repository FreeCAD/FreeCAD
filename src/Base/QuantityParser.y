/* Parser for the FreeCAD  Units language           */
/* (c) 2013 Juergen Riegel  LGPL                    */


/* Represents the many different ways we can access our data */
%{
        #define YYSTYPE Quantity
        #define yyparse Quantity_yyparse
        #define yyerror Quantity_yyerror
        #ifndef  DOUBLE_MAX
        # define DOUBLE_MAX 1.7976931348623157E+308    /* max decimal value of a "double"*/
        #endif
        #ifndef  DOUBLE_MIN
        # define DOUBLE_MIN 2.2250738585072014E-308    /* min decimal value of a "double"*/
        #endif

%}

     /* Bison declarations.  */
     %token UNIT NUM
     %token ACOS ASIN ATAN ATAN2 COS EXP ABS MOD LOG LOG10 POW SIN SINH TAN TANH SQRT;
     %left '-' '+'
     %left '*' '/'
     %left NEG     /* negation--unary minus */
     %right '^'    /* exponentiation */



%start input

%%

    input:                                  { QuantResult = Quantity(DOUBLE_MIN); /* empty input */ }
            |  num                          { QuantResult = $1     ;            }
            |  unit                         { QuantResult = $1     ;            }
            |  quantity                     { QuantResult = $1     ;            }
            |  quantity quantity            { QuantResult = $1 + $2;            }
 ;   
     num:      NUM                			{ $$ = $1;         	}
             | num '+' num        			{ $$ = $1.getValue() + $3.getValue();    	}
             | num '-' num        			{ $$ = $1.getValue() - $3.getValue();    	}
             | num '*' num        			{ $$ = $1.getValue() * $3.getValue();    	}
             | num '/' num        			{ $$ = $1.getValue() / $3.getValue();    	}
             | '-' num  %prec NEG 			{ $$ = -$2.getValue();        	}
             | num '^' num        			{ $$ = pow ($1.getValue(), $3.getValue());}
             | '(' num ')'        			{ $$ = $2;         	}
             | ACOS  '(' num ')'  			{ $$ = acos($3.getValue());   	}
             | ASIN  '(' num ')'  			{ $$ = asin($3.getValue());   	}
             | ATAN  '(' num ')'  			{ $$ = atan($3.getValue());   	}
             | ATAN2 '(' num ',' num ')'    { $$ = atan2($3.getValue(),$5.getValue());}
             | ABS  '(' num ')'   			{ $$ = fabs($3.getValue());   	}
             | EXP  '(' num ')'   			{ $$ = exp($3.getValue());    	}
             | MOD  '(' num ',' num ')'		{ $$ = fmod($3.getValue(),$5.getValue()); }
             | LOG  '(' num ')'				{ $$ = log($3.getValue());     }
             | LOG10  '(' num ')'			{ $$ = log10($3.getValue());   }
             | POW  '(' num ',' num ')'		{ $$ = pow($3.getValue(),$5.getValue());  }
             | SIN  '(' num ')'   			{ $$ = sin($3.getValue());     }
             | SINH '(' num ')'   			{ $$ = sinh($3.getValue());    }
             | TAN  '(' num ')'   			{ $$ = tan($3.getValue());     }
             | TANH  '(' num ')'   			{ $$ = tanh($3.getValue());    }
             | SQRT  '(' num ')'   			{ $$ = tanh($3.getValue());    }
             | COS  '(' num ')'   			{ $$ = cos($3.getValue());    }
;            

    unit:       UNIT                        { $$ = $1;         	                }
            |   unit '*' unit        	    { $$ = $1 * $3;    	                }
            |   unit '/' unit        		{ $$ = $1 / $3;    	                }
            |   unit '^' num        	    { $$ = $1.pow ($3);                 }
            |   '(' unit ')'                { $$ = $2;                          }
;
    quantity:   num unit                    { $$ = $1*$2;    	                }
;            


%%
