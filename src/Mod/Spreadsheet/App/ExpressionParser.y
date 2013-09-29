/* Parser for the FreeCAD  Units language           */
/* (c) 2010 Juergen Riegel  LGPL                    */
/* (c) 2015 Eivind Kvedalen LGPL                    */

/* Represents the many different ways we can access our data */

%{

class semantic_type {
public:
  struct  {
    Quantity scaler;
    std::string unitStr;
  } quantity;
  Expression * expr;
  Path path;
  std::deque<Path::Component> components;
  int ivalue;
  double fvalue;
  struct {
    std::string name;
    double fvalue;
  } constant;
  std::vector<Expression*> arguments;
  std::string string;
  FunctionExpression::Function func;
  Path::String string_or_identifier;
  semantic_type() {}
};

#define YYSTYPE semantic_type

std::stack<FunctionExpression::Function> functions;                /**< Function identifier */

       //#define YYSTYPE yystype
       #define yyparse ExpressionParser_yyparse
       #define yyerror ExpressionParser_yyerror
%}

     /* Bison declarations.  */
     %token FUNC
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
     %type <fvalue> NUM
     %type <constant> CONSTANT
     %type <expr> num
     %type <expr> range
     %type <path> identifier
     %type <components> path
     %type <func> FUNC
     %type <string_or_identifier> document
     %type <string_or_identifier> object
     %left NUM
     %left INTEGER
     %left CONSTANT
     %left '-' '+'
     %left '*' '/'
     %left '?' ':' EQ NEQ LT GT GTE LTE
     %left NEG     /* negation--unary minus */
     %right '^'    /* exponentiation */
     %right EXPONENT

%destructor { delete $$; } exp cond unit_exp
%destructor { std::vector<Expression*>::const_iterator i = $$.begin(); while (i != $$.end()) { delete *i; ++i; } } args

%start input

%%

input:     exp                			{ ScanResult = $1; valueExpression = true;                                        }
     |     unit_exp                             { ScanResult = $1; unitExpression = true;                                         }
     ;

exp:      num                			{ $$ = $1;                                                                        }
        | STRING                                { $$ = new StringExpression(DocumentObject, $1);                                  }
        | identifier                            { $$ = new VariableExpression(DocumentObject, $1);                                }
        | exp '+' exp        			{ $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::ADD, $3);   }
        | exp '-' exp        			{ $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::SUB, $3);   }
        | exp '*' exp        			{ $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::MUL, $3);   }
        | exp '/' exp        			{ $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::DIV, $3);   }
        | exp '/' unit_exp                      { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::DIV, $3);   }
        | MINUSSIGN exp  %prec NEG 		{ $$ = new OperatorExpression(DocumentObject,
                                                                              new NumberExpression(DocumentObject, -1.0),
                                                                              OperatorExpression::MUL, $2);        	          }
        | exp '^' exp %prec NUM                 { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::POW, $3);   }
        | '(' exp ')'     			{ $$ = $2;                                                                        }
        | FUNC  args ')'  		        { $$ = new FunctionExpression(DocumentObject, $1, $2);                            }
        | cond '?' exp ':' exp                  { $$ = new ConditionalExpression(DocumentObject, $1, $3, $5);                     }
        ;

num: NUM                                        { $$ = new NumberExpression(DocumentObject, $1);                                  }
   | INTEGER                                    { $$ = new NumberExpression(DocumentObject, (double)$1);                          }
   | CONSTANT                                   { $$ = new ConstantExpression(DocumentObject, $1.name, $1.fvalue);                }
   | NUM unit_exp %prec EXPONENT                { $$ = new OperatorExpression(DocumentObject, new NumberExpression(DocumentObject, $1), OperatorExpression::UNIT, $2);                    }
   | INTEGER unit_exp                           { $$ = new OperatorExpression(DocumentObject, new NumberExpression(DocumentObject, (double)$1), OperatorExpression::UNIT, $2);            }
   | CONSTANT unit_exp                          { $$ = new OperatorExpression(DocumentObject, new ConstantExpression(DocumentObject, $1.name, $1.fvalue), OperatorExpression::UNIT, $2);  }
   ;

args: exp                                       { $$.push_back($1);                                                               }
    | range                                     { $$.push_back($1);                                                               }
    | args ',' exp                              { $1.push_back($3);  $$ = $1;                                                     }
    | args ';' exp                              { $1.push_back($3);  $$ = $1;                                                     }
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

unit_exp: UNIT                                          { $$ = new UnitExpression(DocumentObject, $1.scaler, $1.unitStr );                }
        | unit_exp '/' unit_exp                         { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::DIV, $3);   }
        | unit_exp '*' unit_exp                         { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::MUL, $3);   }
        | unit_exp '^' NUM %prec EXPONENT               { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::POW, new NumberExpression(DocumentObject, $3));                 }
        | unit_exp '^' '-' NUM %prec EXPONENT           { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::POW, new NumberExpression(DocumentObject, -$4));                }
        | unit_exp '^' INTEGER %prec EXPONENT           { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::POW, new NumberExpression(DocumentObject, (double)$3));         }
        | unit_exp '^' MINUSSIGN INTEGER %prec EXPONENT { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::POW, new NumberExpression(DocumentObject, -(double)$4));  }
        | '(' unit_exp ')'                              { $$ = $2;                                                                        }
        ;

identifier: path                                { /* Path to property within document object */
                                                  $$ = Path(DocumentObject);
                                                  $$.addComponents($1);
                                                }
          | object '.' path                     { /* Path to property within document object */
                                                  $$ = Path(DocumentObject);
                                                  $$.setDocumentObjectName($1, true);
                                                  $$.addComponents($3);
                                                }
          | document '#' path                   { /* Path to property from an external document, within a named document object */
                                                  $$ = Path(DocumentObject);
                                                  $$.setDocumentName($1, true);
                                                  $$.addComponents($3);
                                                }
          | document '#' object '.' path        { /* Path to property from an external document, within a named document object */
                                                  $$ = Path(DocumentObject);
                                                  $$.setDocumentName($1, true);
                                                  $$.setDocumentObjectName($3, true);
                                                  $$.addComponents($5);
                                                }
     ;

path: IDENTIFIER                                       { $$.push_front(Path::Component::SimpleComponent($1));                         }
    | CELLADDRESS                                      { $$.push_front(Path::Component::SimpleComponent($1));                         }
    | IDENTIFIER '[' INTEGER ']'                       { $$.push_front(Path::Component::ArrayComponent($1, $3));                      }
    | IDENTIFIER '[' INTEGER ']' '.' path              { $6.push_front(Path::Component::ArrayComponent($1, $3)); $$ = $6;             }
    | IDENTIFIER '[' STRING ']'                        { $$.push_front(Path::Component::MapComponent($1, Path::String($3, true)));          }
    | IDENTIFIER '[' IDENTIFIER ']'                    { $$.push_front(Path::Component::MapComponent($1, $3));                        }
    | IDENTIFIER '[' STRING ']' '.' path               { $6.push_front(Path::Component::MapComponent($1, Path::String($3, true))); $$ = $6; }
    | IDENTIFIER '[' IDENTIFIER ']' '.' path           { $6.push_front(Path::Component::MapComponent($1, $3)); $$ = $6;               }
    | IDENTIFIER '.' path                              { $3.push_front(Path::Component::SimpleComponent($1)); $$ = $3;                }
    ;

document: STRING                                       { $$ = Path::String($1, true); }
        | IDENTIFIER                                   { $$ = Path::String($1);       }
        ;

object: STRING                                         { $$ = Path::String($1, true); }
      | CELLADDRESS                                    { $$ = Path::String($1, true); }
      ;

%%
