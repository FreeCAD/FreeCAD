/* Parser for the FreeCAD  Units language           */
/* (c) 2010 Juergen Riegel  LGPL                    */
/* (c) 2013 Eivind Kvedalen LGPL           */

/* Represents the many different ways we can access our data */
%{
       #define YYSTYPE Expression*
       #define yyparse ExpressionParser_yyparse
       #define yyerror ExpressionParser_yyerror
%}

     /* Bison declarations.  */
     %token ACOS ASIN ATAN ATAN2 COS COSH EXP ABS MOD LOG LOG10 POW SIN SINH TAN TANH SQRT;
     %token NUM
     %token LABEL
     %token UNIT
     %left '-' '+'
     %left '*' '/'
     %left NEG     /* negation--unary minus */
     %right '^'    /* exponentiation */
     %left NUM

%start input

%%

input:     exp                			{ ScanResult = $1; valueExpression = true;                                        }
     |     unit_exp                             { ScanResult = $1; unitExpression = true;                                         }
     ;
     
exp:      NUM                			{ $$ = $1;                                                                        }
        | NUM unit_exp                          { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::MUL, $2);   }
        | LABEL                                 { $$ = $1;                                                                        }
        | exp '+' exp        			{ $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::ADD, $3);   }
        | exp '-' exp        			{ $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::SUB, $3);   }
        | exp '*' exp        			{ $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::MUL, $3);   }
        | exp '/' exp        			{ $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::DIV, $3);   }
        | exp '/' unit_exp                      { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::DIV, $3);   }
        | '-' exp  %prec NEG 			{ $$ = new OperatorExpression(DocumentObject,
                                                                              new NumberExpression(DocumentObject, -1.0),
                                                                              OperatorExpression::MUL, $2);        	          }
        | exp '^' exp        			{ $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::POW, $3);   }
        | '(' exp ')'     			{ $$ = $2;                                                                        }
        | func                 			{ $$ = $1;                                                                        }
        ;

func:     ACOS  '(' exp ')'  			{ $$ = new FunctionExpression(DocumentObject, FunctionExpression::ACOS, $3);   	  }
        | ASIN  '(' exp ')'  			{ $$ = new FunctionExpression(DocumentObject, FunctionExpression::ASIN, $3);   	  }
        | ATAN  '(' exp ')'  			{ $$ = new FunctionExpression(DocumentObject, FunctionExpression::ATAN, $3);   	  }
        | ATAN2 '(' exp ',' exp ')'             { $$ = new FunctionExpression(DocumentObject, FunctionExpression::ATAN2, $3, $5); }
        | ABS  '(' exp ')'   			{ $$ = new FunctionExpression(DocumentObject, FunctionExpression::ABS, $3);   	  }
        | EXP  '(' exp ')'   			{ $$ = new FunctionExpression(DocumentObject, FunctionExpression::EXP, $3);       }
        | MOD '(' exp ',' exp ')'               { $$ = new FunctionExpression(DocumentObject, FunctionExpression::MOD, $3, $5);   }
        | LOG  '(' exp ')'			{ $$ = new FunctionExpression(DocumentObject, FunctionExpression::LOG, $3);   	  }
        | LOG10  '(' exp ')'			{ $$ = new FunctionExpression(DocumentObject, FunctionExpression::LOG10, $3);     }
        | SIN  '(' exp ')'   			{ $$ = new FunctionExpression(DocumentObject, FunctionExpression::SIN, $3);       }
        | SINH '(' exp ')'   			{ $$ = new FunctionExpression(DocumentObject, FunctionExpression::SINH, $3);   	  }
        | POW '(' exp ',' exp ')'               { $$ = new FunctionExpression(DocumentObject, FunctionExpression::POW, $3, $5);   }
        | TAN  '(' exp ')'   			{ $$ = new FunctionExpression(DocumentObject, FunctionExpression::TAN, $3);   	  }
        | TANH  '(' exp ')'   			{ $$ = new FunctionExpression(DocumentObject, FunctionExpression::TANH, $3);   	  }
        | SQRT  '(' exp ')'   			{ $$ = new FunctionExpression(DocumentObject, FunctionExpression::SQRT, $3);      }
        | COS  '(' exp ')'   			{ $$ = new FunctionExpression(DocumentObject, FunctionExpression::COS, $3);   	  }
        | COSH  '(' exp ')'   			{ $$ = new FunctionExpression(DocumentObject, FunctionExpression::COSH, $3);   	  }
        ;

unit_exp: UNIT                                  { $$ = new UnitExpression(DocumentObject, unit, unitstr, scaler);                 }
        | unit_exp '/' unit_exp                 { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::DIV, $3);   }
        | unit_exp '*' unit_exp                 { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::MUL, $3);   }
        | unit_exp '^' NUM                      { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::POW, $3);   }
        | unit_exp '^' '-' NUM %prec NEG        { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::POW, $4);
                                                  static_cast<NumberExpression*>($4)->negate();
                                                }
        | '(' unit_exp ')'                      { $$ = $2;                                                                        }
        ;

%%
