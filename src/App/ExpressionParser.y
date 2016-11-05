/* Parser for the FreeCAD  Units language           */
/* (c) 2010 Juergen Riegel  LGPL                    */
/* (c) 2015 Eivind Kvedalen LGPL                    */

/* Represents the many different ways we can access our data */

%{

#define YYSTYPE App::ExpressionParser::semantic_type

std::stack<FunctionExpression::Function> functions;                /**< Function identifier */

       //#define YYSTYPE yystype
       #define yyparse ExpressionParser_yyparse
       #define yyerror ExpressionParser_yyerror
%}

     /* Bison declarations.  */
     %token FUNC
     %token ONE
     %token NUM
     %token IDENTIFIER
     %token UNIT
     %token INTEGER
     %token CONSTANT
     %token CELLADDRESS
     %token EQ NEQ LT GT GTE LTE
     %token STRING MINUSSIGN PROPERTY_REF
     %token DOCUMENT OBJECT
     %token EXPONENT
     %type <arguments> args
     %type <expr> input exp unit_exp cond
     %type <quantity> UNIT
     %type <string> STRING IDENTIFIER CELLADDRESS
     %type <ivalue> INTEGER
     %type <string> PROPERTY_REF
     %type <fvalue> ONE
     %type <fvalue> NUM
     %type <constant> CONSTANT
     %type <expr> num
     %type <expr> range
     %type <path> identifier
     %type <components> path subpath
     %type <func> FUNC
     %type <string_or_identifier> document
     %type <string_or_identifier> object
     %type <ivalue> integer
     %left ONE NUM INTEGER CONSTANT
     %left EQ NEQ LT GT GTE LTE
     %left '?' ':'
     %left MINUSSIGN '+'
     %left '*' '/'
     %precedence NUM_AND_UNIT
     %left '^'    /* exponentiation */
     %left EXPONENT
     %left NEG     /* negation--unary minus */
     %left POS     /* unary plus */

%destructor { delete $$; } exp cond unit_exp
%destructor { std::vector<Expression*>::const_iterator i = $$.begin(); while (i != $$.end()) { delete *i; ++i; } } args

%start input

%%

input:     exp                			{ ScanResult = $1; valueExpression = true;                                        }
     |     unit_exp                             { ScanResult = $1; unitExpression = true;                                         }
     ;

exp:      num                			{ $$ = $1;                                                                        }
        | num unit_exp %prec NUM_AND_UNIT       { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::UNIT, $2);  }
        | STRING                                { $$ = new StringExpression(DocumentObject, $1);                                  }
        | identifier                            { $$ = new VariableExpression(DocumentObject, $1);                                }
        | MINUSSIGN exp %prec NEG               { $$ = new OperatorExpression(DocumentObject, $2, OperatorExpression::NEG, new NumberExpression(DocumentObject, Quantity(-1))); }
        | '+' exp %prec POS                     { $$ = new OperatorExpression(DocumentObject, $2, OperatorExpression::POS, new NumberExpression(DocumentObject, Quantity(1))); }
        | exp '+' exp        			{ $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::ADD, $3);   }
        | exp MINUSSIGN exp                     { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::SUB, $3);   }
        | exp '*' exp        			{ $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::MUL, $3);   }
        | exp '/' exp        			{ $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::DIV, $3);   }
        | exp '/' unit_exp                      { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::DIV, $3);   }
        | exp '^' exp                           { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::POW, $3);   }
        | '(' exp ')'     			{ $$ = $2;                                                                        }
        | FUNC  args ')'  		        { $$ = new FunctionExpression(DocumentObject, $1, $2);                   }
        | cond '?' exp ':' exp                  { $$ = new ConditionalExpression(DocumentObject, $1, $3, $5);                     }
        ;

num:       ONE                                  { $$ = new NumberExpression(DocumentObject, Quantity($1));                        }
         | NUM                                  { $$ = new NumberExpression(DocumentObject, Quantity($1));                        }
         | INTEGER                              { $$ = new NumberExpression(DocumentObject, Quantity((double)$1));                }
         | CONSTANT                             { $$ = new ConstantExpression(DocumentObject, $1.name, Quantity($1.fvalue));      }

args: exp                                       { $$.push_back($1);                                                               }
    | range                                     { $$.push_back($1);                                                               }
    | args ',' exp                              { $1.push_back($3);  $$ = $1;                                                     }
    | args ';' exp                              { $1.push_back($3);  $$ = $1;                                                     }
    | args ',' range                            { $1.push_back($3);  $$ = $1;                                                     }
    | args ';' range                            { $1.push_back($3);  $$ = $1;                                                     }
    ;

range: CELLADDRESS ':' CELLADDRESS              { $$ = new RangeExpression(DocumentObject, $1, $3);                               }
     | CELLADDRESS ':' IDENTIFIER               { $$ = new RangeExpression(DocumentObject, $1, $3);                               }
     | IDENTIFIER ':' CELLADDRESS               { $$ = new RangeExpression(DocumentObject, $1, $3);                               }
     | IDENTIFIER ':' IDENTIFIER                { $$ = new RangeExpression(DocumentObject, $1, $3);                               }
     ;

cond: exp EQ exp                                { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::EQ, $3);    }
    | exp NEQ exp                               { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::NEQ, $3);   }
    | exp LT exp                                { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::LT, $3);    }
    | exp GT exp                                { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::GT, $3);    }
    | exp GTE exp                               { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::GTE, $3);   }
    | exp LTE exp                               { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::LTE, $3);   }
    ;

unit_exp: UNIT                                  { $$ = new UnitExpression(DocumentObject, $1.scaler, $1.unitStr );                }
        | unit_exp '/' unit_exp                 { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::DIV, $3);   }
        | unit_exp '*' unit_exp                 { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::MUL, $3);   }
        | unit_exp '^' integer                  { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::POW, new NumberExpression(DocumentObject, Quantity((double)$3)));   }
        | unit_exp '^' MINUSSIGN integer        { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::POW, new OperatorExpression(DocumentObject, new NumberExpression(DocumentObject, Quantity((double)$4)), OperatorExpression::NEG, new NumberExpression(DocumentObject, Quantity(-1))));   }
        | '(' unit_exp ')'                      { $$ = $2;                                                                        }
        ;

identifier: path                                { /* Path to property within document object */
                                                  $$ = ObjectIdentifier(DocumentObject);
                                                  $$.addComponents($1);
                                                }
          | object '.' path                     { /* Path to property within document object */
                                                  $$ = ObjectIdentifier(DocumentObject);
                                                  $$.setDocumentObjectName($1, true);
                                                  $$.addComponents($3);
                                                }
          | document '#' path                   { /* Path to property from an external document, within a named document object */
                                                  $$ = ObjectIdentifier(DocumentObject);
                                                  $$.setDocumentName($1, true);
                                                  $$.addComponents($3);
                                                }
          | document '#' object '.' path        { /* Path to property from an external document, within a named document object */
                                                  $$ = ObjectIdentifier(DocumentObject);
                                                  $$.setDocumentName($1, true);
                                                  $$.setDocumentObjectName($3, true);
                                                  $$.addComponents($5);
                                                }
     ;

integer: INTEGER { $$ = $1; }
       | ONE { $$ = $1; }
       ;


path: IDENTIFIER                                       { $$.push_front(ObjectIdentifier::Component::SimpleComponent($1));                         }
    | CELLADDRESS                                      { $$.push_front(ObjectIdentifier::Component::SimpleComponent($1));                         }
    | IDENTIFIER '[' integer ']'                       { $$.push_front(ObjectIdentifier::Component::ArrayComponent($1, $3));                      }
    | IDENTIFIER '[' integer ']' '.' subpath              { $6.push_front(ObjectIdentifier::Component::ArrayComponent($1, $3)); $$ = $6;             }
    | IDENTIFIER '[' STRING ']'                        { $$.push_front(ObjectIdentifier::Component::MapComponent($1, ObjectIdentifier::String($3, true)));          }
    | IDENTIFIER '[' IDENTIFIER ']'                    { $$.push_front(ObjectIdentifier::Component::MapComponent($1, $3));                        }
    | IDENTIFIER '[' STRING ']' '.' subpath               { $6.push_front(ObjectIdentifier::Component::MapComponent($1, ObjectIdentifier::String($3, true))); $$ = $6; }
    | IDENTIFIER '[' IDENTIFIER ']' '.' subpath           { $6.push_front(ObjectIdentifier::Component::MapComponent($1, $3)); $$ = $6;               }
    | IDENTIFIER '.' subpath                              { $3.push_front(ObjectIdentifier::Component::SimpleComponent($1)); $$ = $3;                }
    ;

subpath: IDENTIFIER                                       { $$.push_front(ObjectIdentifier::Component::SimpleComponent($1));                         }
    | STRING                                              { $$.push_front(ObjectIdentifier::Component::SimpleComponent($1));                         }
    | CELLADDRESS                                      { $$.push_front(ObjectIdentifier::Component::SimpleComponent($1));                         }
    | IDENTIFIER '[' integer ']'                       { $$.push_front(ObjectIdentifier::Component::ArrayComponent($1, $3));                      }
    | IDENTIFIER '[' integer ']' '.' subpath              { $6.push_front(ObjectIdentifier::Component::ArrayComponent($1, $3)); $$ = $6;             }
    | IDENTIFIER '[' STRING ']'                        { $$.push_front(ObjectIdentifier::Component::MapComponent($1, ObjectIdentifier::String($3, true)));          }
    | IDENTIFIER '[' IDENTIFIER ']'                    { $$.push_front(ObjectIdentifier::Component::MapComponent($1, $3));                        }
    | IDENTIFIER '[' STRING ']' '.' subpath               { $6.push_front(ObjectIdentifier::Component::MapComponent($1, ObjectIdentifier::String($3, true))); $$ = $6; }
    | IDENTIFIER '[' IDENTIFIER ']' '.' subpath           { $6.push_front(ObjectIdentifier::Component::MapComponent($1, $3)); $$ = $6;               }
    | IDENTIFIER '.' subpath                              { $3.push_front(ObjectIdentifier::Component::SimpleComponent($1)); $$ = $3;                }
    ;

document: STRING                                       { $$ = ObjectIdentifier::String($1, true); }
        | IDENTIFIER                                   { $$ = ObjectIdentifier::String($1);       }
        ;

object: STRING                                         { $$ = ObjectIdentifier::String($1, true); }
      | CELLADDRESS                                    { $$ = ObjectIdentifier::String($1, true); }
      ;

%%
