/* Parser for the FreeCAD Units language           */
/* (c) 2010 Jürgen Riegel   LGPL                   */
/* (c) 2015 Eivind Kvedalen LGPL                   */

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
     %type <expr> input exp unit_exp cond indexable
     %type <quantity> UNIT
     %type <string> id_or_cell STRING IDENTIFIER CELLADDRESS
     %type <ivalue> INTEGER
     %type <string> PROPERTY_REF
     %type <fvalue> ONE
     %type <fvalue> NUM
     %type <constant> CONSTANT
     %type <expr> num
     %type <expr> range
     %type <path> identifier iden
     %type <component> indexer
     %type <func> FUNC
     %type <string_or_identifier> document
     %type <string_or_identifier> object
     %type <ivalue> integer
     %left ONE NUM INTEGER CONSTANT
     %left EQ NEQ LT GT GTE LTE
     %left '?' ':'
     %left MINUSSIGN '+'
     %left '*' '/' '%'
     %precedence NUM_AND_UNIT
     %left '^'    /* exponentiation */
     %left EXPONENT
     %left NEG     /* negation--unary minus */
     %left POS     /* unary plus */

%destructor { delete $$; } num range exp cond unit_exp indexable
%destructor { delete $$; } <component>
%destructor { std::vector<Expression*>::const_iterator i = $$.begin(); while (i != $$.end()) { delete *i; ++i; } } args

%start input

%%

input:     exp                			{ ScanResult = $1; valueExpression = true;                                        }
     |     unit_exp                     { ScanResult = $1; unitExpression = true;                                         }
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
        | exp '%' exp        			{ $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::MOD, $3);   }
        | exp '/' unit_exp                      { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::DIV, $3);   }
        | exp '^' exp                           { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::POW, $3);   }
        | indexable       			    { $$ = $1;                                                                        }
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

range: id_or_cell ':' id_or_cell                { $$ = new RangeExpression(DocumentObject, $1, $3);                               }
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

integer: INTEGER { $$ = $1; }
       | ONE { $$ = $1; }
       ;

id_or_cell
    : IDENTIFIER                            { $$ = std::move($1); }
    | CELLADDRESS                           { $$ = std::move($1); }
    ;

identifier
    : id_or_cell                            { $$ = ObjectIdentifier(DocumentObject); $$ << ObjectIdentifier::SimpleComponent($1); }
    | iden                                  { $$ = std::move($1); }
    ;

iden
    :  '.' STRING '.' id_or_cell            { /* Path to property of a sub-object of the current object*/
                                                $$ = ObjectIdentifier(DocumentObject,true);
                                                $$.setDocumentObjectName(DocumentObject,false,ObjectIdentifier::String(std::move($2),true),true);
                                                $$.addComponent(ObjectIdentifier::SimpleComponent($4));
                                            }
    | '.' id_or_cell                        { /* Path to property of the current document object */
                                                $$ = ObjectIdentifier(DocumentObject,true);
                                                $$.setDocumentObjectName(DocumentObject);
                                                $$.addComponent(ObjectIdentifier::SimpleComponent($2));
                                            }
    | object '.' STRING '.' id_or_cell      { /* Path to property of a sub-object */
                                                $$ = ObjectIdentifier(DocumentObject);
                                                $$.setDocumentObjectName(std::move($1), true, ObjectIdentifier::String(std::move($3),true),true);
                                                $$.addComponent(ObjectIdentifier::SimpleComponent($5));
                                                $$.resolveAmbiguity();
                                            }
    | object '.' id_or_cell                 { /* Path to property of a given document object */
                                                $$ = ObjectIdentifier(DocumentObject);
                                                $1.checkImport(DocumentObject);
                                                $$.addComponent(ObjectIdentifier::SimpleComponent($1));
                                                $$.addComponent(ObjectIdentifier::SimpleComponent($3));
                                                $$.resolveAmbiguity();
                                            }
    | document '#' object '.' id_or_cell    { /* Path to property from an external document, within a named document object */
                                                $$ = ObjectIdentifier(DocumentObject);
                                                $$.setDocumentName(std::move($1), true);
                                                $$.setDocumentObjectName(std::move($3), true);
                                                $$.addComponent(ObjectIdentifier::SimpleComponent($5));
                                                $$.resolveAmbiguity();
                                            }
    | document '#' object '.' STRING '.' id_or_cell
                                            {   $$ = ObjectIdentifier(DocumentObject);
                                                $$.setDocumentName(std::move($1), true);
                                                $$.setDocumentObjectName(std::move($3), true, ObjectIdentifier::String(std::move($5),true));
                                                $$.addComponent(ObjectIdentifier::SimpleComponent($7));
                                                $$.resolveAmbiguity();
                                            }
    | iden '.' IDENTIFIER                   { $$= std::move($1); $$.addComponent(ObjectIdentifier::SimpleComponent($3)); }
    ;

indexer
    : '[' exp ']'                           { $$ = Expression::createComponent($2);   }
    | '[' exp ':' ']'                       { $$ = Expression::createComponent($2,0,0,true); }
    | '[' ':' exp ']'                       { $$ = Expression::createComponent(0,$3); }
    | '[' ':' ':' exp ']'                   { $$ = Expression::createComponent(0,0,$4); }
    | '[' exp ':' exp ']'                   { $$ = Expression::createComponent($2,$4);}
    | '[' exp ':' ':' exp ']'               { $$ = Expression::createComponent($2,0,$5); }
    | '[' ':' exp ':' exp ']'               { $$ = Expression::createComponent(0,$3,$5); }
    | '[' exp ':' exp ':' exp ']'           { $$ = Expression::createComponent($2,$4,$6);}
    ;

indexable
    : '(' exp ')'                           { $$ = $2; }
    | identifier indexer                    { $$ = new VariableExpression(DocumentObject,$1); $$->addComponent($2); }
    | indexable indexer                     { $1->addComponent(std::move($2)); $$ = $1; }
    | indexable '.' IDENTIFIER              { $1->addComponent(Expression::createComponent($3)); $$ = $1; }
    ;

document
    : STRING                                { $$ = ObjectIdentifier::String(std::move($1), true); }
    | IDENTIFIER                            { $$ = ObjectIdentifier::String(std::move($1), false, true);}
    ;

object
    : STRING                                { $$ = ObjectIdentifier::String(std::move($1), true); }
    | id_or_cell                            { $$ = ObjectIdentifier::String(std::move($1), false);}
    ;

%%
