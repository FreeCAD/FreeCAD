/***************************************************************************
 *   Copyright (c) 2013 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

//* Parser for the FreeCAD Units language           */

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
     %token UNIT ONE NUM MINUSSIGN
     %token ACOS ASIN ATAN ATAN2 COS EXP ABS MOD LOG LOG10 POW SIN SINH TAN TANH SQRT;
     %left MINUSSIGN '+'
     %left '*' '/'
     %left NEG     /* negation--unary minus */
     %right '^'    /* exponentiation */
     %left ONE NUM



%start input

%%

    input:                                  { QuantResult = Quantity(DOUBLE_MIN); /* empty input */ }
            |  num                          { QuantResult = $1     ;            }
            |  unit                         { QuantResult = $1     ;            }
            |  quantity                     { QuantResult = $1     ;            }
            |  quantity quantity            { QuantResult = $1 + $2;            }
            |  quantity quantity quantity   { QuantResult = $1 + $2 + $3;       }
 ;
     num:      NUM                			{ $$ = $1;         	}
             | ONE                			{ $$ = $1;         	}
             | num '+' num        			{ $$ = Quantity($1.getValue() + $3.getValue());    	}
             | num MINUSSIGN num            { $$ = Quantity($1.getValue() - $3.getValue());    	}
             | num '*' num        			{ $$ = Quantity($1.getValue() * $3.getValue());    	}
             | num '/' num        			{ $$ = Quantity($1.getValue() / $3.getValue());    	}
             | MINUSSIGN num  %prec NEG     { $$ = Quantity(-$2.getValue());                    }
             | num '^' num        			{ $$ = Quantity(pow ($1.getValue(), $3.getValue()));}
             | '(' num ')'        			{ $$ = $2;         	}
             | ACOS  '(' num ')'  			{ $$ = Quantity(acos($3.getValue()));   }
             | ASIN  '(' num ')'  			{ $$ = Quantity(asin($3.getValue()));   }
             | ATAN  '(' num ')'  			{ $$ = Quantity(atan($3.getValue()));   }
             | ABS   '(' num ')'  			{ $$ = Quantity(fabs($3.getValue()));   }
             | EXP   '(' num ')'  			{ $$ = Quantity(exp($3.getValue()));    }
             | LOG   '(' num ')'  			{ $$ = Quantity(log($3.getValue()));    }
             | LOG10 '(' num ')'  			{ $$ = Quantity(log10($3.getValue()));  }
             | SIN   '(' num ')'  			{ $$ = Quantity(sin($3.getValue()));    }
             | SINH  '(' num ')'  			{ $$ = Quantity(sinh($3.getValue()));   }
             | TAN   '(' num ')'  			{ $$ = Quantity(tan($3.getValue()));    }
             | TANH  '(' num ')'  			{ $$ = Quantity(tanh($3.getValue()));   }
             | SQRT  '(' num ')'  			{ $$ = Quantity(sqrt($3.getValue()));   }
             | COS   '(' num ')'  			{ $$ = Quantity(cos($3.getValue()));    }
;

    unit:       UNIT                        { $$ = $1;         	                }
            |   ONE '/' unit                { $$ = Quantity(1.0)/$3;  	        }
            |   unit '*' unit        	    { $$ = $1 * $3;    	                }
            |   unit '/' unit        	    { $$ = $1 / $3;    	                }
            |   unit '^' num        	    { $$ = $1.pow ($3);                 }
            |   '(' unit ')'                { $$ = $2;                          }
;
    quantity:   num unit                    { $$ = $1*$2;    	                }
            |   num '/' unit                { $$ = Quantity($1)/$3;  	        }
;


%%
