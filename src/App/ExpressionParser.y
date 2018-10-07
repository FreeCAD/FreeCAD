/* Parser for the FreeCAD  Units language           */
/* (c) 2010 Juergen Riegel  LGPL                    */
/* (c) 2015 Eivind Kvedalen LGPL                    */

/* Represents the many different ways we can access our data */

%{

#define YYSTYPE App::ExpressionParser::semantic_type

       //#define YYSTYPE yystype
       #define yyparse ExpressionParser_yyparse
       #define yyerror ExpressionParser_yyerror
%}

     /* Bison declarations.  */
     %token ONE
     %token NUM
     %token IDENTIFIER
     %token UNIT
     %token INTEGER
     %token CONSTANT
     %token CELLADDRESS
     %token EQ NEQ LT GT GTE LTE AND_OP AND_OP2 OR_OP OR_OP2
     %token RSTRING STRING MINUSSIGN PROPERTY_REF
     %token DOCUMENT OBJECT
     %token EXPONENT
     %type <arguments> items items2
     %type <named_argument> arg
     %type <named_arguments> args
     %type <expr> primary_exp unary_exp multiply_exp additive_exp relational_exp
     %type <expr> equality_exp logical_or_exp logical_and_exp conditional_exp power_exp
     %type <expr> input exp unit_exp indexable num range callable
     %type <expr> list tuple dict dict1
     %type <quantity> UNIT
     %type <string> id_or_cell RSTRING STRING IDENTIFIER CELLADDRESS
     %type <ivalue> INTEGER
     %type <string> PROPERTY_REF
     %type <fvalue> ONE
     %type <fvalue> NUM
     %type <constant> CONSTANT
     %type <path> identifier
     %type <components> path subpath
     %type <component> indexer
     %type <string_or_identifier> document object subname
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

%destructor { delete $$; } <expr>
%destructor { for(auto expr : $$) {delete expr;} } <arguments>
%destructor { for(auto &v : $$) {delete v.second;} } <named_arguments>
%destructor { delete $$.second; } <named_argument>

%start input

%%

input:     exp                			{ ScanResult = $1; valueExpression = true; $$=0;                                       }
     |     unit_exp                             { ScanResult = $1; unitExpression = true; $$=0;                                        }
     ;

primary_exp
        : num                			{ $$ = $1;                                                                        }
        | num unit_exp %prec NUM_AND_UNIT       { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::UNIT, $2);  }
        | STRING                                { $$ = new StringExpression(DocumentObject, $1); }
        | RSTRING                               { $$ = new StringExpression(DocumentObject, $1, true); }
        | identifier                            { $$ = new VariableExpression(DocumentObject, $1);                                }
        | indexable                             { $$ = $1; }
        | callable                              { $$ = $1; }
        ;

indexable : '(' exp ')'     			{ $$ = $2; }
          | tuple                               { $$ = $1; }
          | list                                { $$ = $1; }
          | dict                                { $$ = $1; }
          | indexable indexer                   { $1->addComponent($2); $$ = $1; }
          | indexable '.' id_or_cell            { $1->addComponent(ObjectIdentifier::SimpleComponent($3)); $$ = $1; }
          ;

callable: identifier '(' ')'                    { $$ = new CallableExpression(DocumentObject, new VariableExpression(DocumentObject,$1)); }
        | identifier '(' args ')'               { 
                                                    $$ = CallableExpression::create(DocumentObject, $1, $3); 
                                                    if(!$$) { 
                                                        YYABORT;
                                                    } 
                                                }
        | UNIT '(' args ')'                     {   // This rule exists because of possible name clash of 
                                                    // function and unit, e.g. min
                                                    ObjectIdentifier name(DocumentObject);
                                                    name << ObjectIdentifier::SimpleComponent($1.unitStr);
                                                    $$ = CallableExpression::create(DocumentObject, name, $3);
                                                    if(!$$) { 
                                                        YYABORT;
                                                    } 
                                                }
        | indexable '(' ')'                     { $$ = new CallableExpression(DocumentObject, $1); }
        | indexable '(' args ')'                { $$ = new CallableExpression(DocumentObject, $1, $3); }
        | callable '(' ')'                      { $$ = new CallableExpression(DocumentObject, $1); }
        | callable '(' args ')'                 { $$ = new CallableExpression(DocumentObject, $1, $3); }
        | callable indexer                      { $1->addComponent($2); $$ = $1; }
        | callable '.' id_or_cell               { $1->addComponent(ObjectIdentifier::SimpleComponent($3)); $$ = $1; }
        ;

unary_exp
        : primary_exp                           { $$ = $1; }
        | MINUSSIGN unary_exp %prec NEG         { $$ = new OperatorExpression(DocumentObject, $2, OperatorExpression::NEG, new NumberExpression(DocumentObject, Quantity(-1))); }
        | '+' unary_exp %prec POS               { $$ = new OperatorExpression(DocumentObject, $2, OperatorExpression::POS, new NumberExpression(DocumentObject, Quantity(1))); }
        ;

power_exp
        : unary_exp                             { $$ = $1; }
        | power_exp '^' unary_exp               { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::POW, $3);   }

multiply_exp
        : power_exp                             { $$ = $1; }
        | multiply_exp '*' power_exp        	{ $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::MUL, $3);   }
        | multiply_exp '/' power_exp            { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::DIV, $3);   }
        | multiply_exp '/' unit_exp             { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::DIV, $3);   }
        | multiply_exp '%' power_exp            { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::MOD, $3);   }
        ;

additive_exp
        : multiply_exp                          { $$ = $1; }
        | additive_exp '+' multiply_exp        	{ $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::ADD, $3);   }
        | additive_exp MINUSSIGN multiply_exp   { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::SUB, $3);   }
        ;

relational_exp
	: additive_exp                          { $$ = $1; }
        | relational_exp LT additive_exp        { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::LT, $3);    }
        | relational_exp GT additive_exp        { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::GT, $3);    }
        | relational_exp GTE additive_exp       { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::GTE, $3);   }
        | relational_exp LTE additive_exp       { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::LTE, $3);   }
	;

equality_exp
	: relational_exp                        { $$ = $1; }
	| equality_exp EQ relational_exp        { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::EQ, $3);    }
	| equality_exp NEQ relational_exp       { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::NEQ, $3);   }
	;

logical_and_exp
	: equality_exp                          { $$ = $1; }
	| logical_and_exp AND_OP equality_exp   { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::AND_OP, $3);   }
	| logical_and_exp AND_OP2 equality_exp  { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::AND_OP2, $3);   }

	;

logical_or_exp
	: logical_and_exp                       { $$ = $1; }
	| logical_or_exp OR_OP logical_and_exp  { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::OR_OP, $3);   }
	| logical_or_exp OR_OP2 logical_and_exp { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::OR_OP2, $3);   }
	;

conditional_exp
    : logical_or_exp                             { $$ = $1; }
    | logical_or_exp '?' exp ':' conditional_exp { $$ = new ConditionalExpression(DocumentObject, $1, $3, $5);                     }
    ;

exp
        : conditional_exp                       { $$ = $1; }

id_or_cell : IDENTIFIER                         { $$ = $1; }
           | CELLADDRESS                        { $$ = $1; }
           ;

sep : ',' | ';' ;

items2 : exp sep exp                            { $$.push_back($1); $$.push_back($3); }
       | items2 sep exp                         { $1.push_back($3); $$.swap($1); }
       ;

tuple : '(' sep ')'                             { $$ = new TupleExpression(DocumentObject); }
      | '(' exp sep ')'                         { $$ = new TupleExpression(DocumentObject, $2); }
      | '(' items2 ')'                          { $$ = new TupleExpression(DocumentObject, $2); }
      | '(' items2 sep ')'                      { $$ = new TupleExpression(DocumentObject, $2); }
      ;

items : exp                                     { $$.push_back($1); }
      | items sep exp                           { $1.push_back($3); $$.swap($1); }
      ;

list : '[' ']'                                  { $$ = new ListExpression(DocumentObject); }
     | '[' items ']'                            { $$ = new ListExpression(DocumentObject, $2); }
     | '[' items sep ']'                        { $$ = new ListExpression(DocumentObject, $2); }
     ;

dict1 : '{' exp ':' exp                         { $$ = new DictExpression(DocumentObject, $2, $4); }
      | dict1 sep exp ':' exp                   { static_cast<DictExpression*>($1)->addItem($3,$5); $$ = $1; }
      ;

dict : '{' '}'                                  { $$ = new DictExpression(DocumentObject); }
     | dict1 '}'                                { $$ = $1; }
     | dict1 sep '}'                            { $$ = $1; }
     ;

arg: exp                                        { $$.first.clear(); $$.second = $1; }
   | id_or_cell '='  exp                        { $$.first = $1; $$.second = $3; }
   | '*' exp                                    { $$.first = "*"; $$.second = $2; }
   | range                                      { $$.first.clear(); $$.second = $1; }
   | '*' range                                  { $$.first = '*'; $$.second = $2; }
   | '*' '*' exp                                { $$.first = "**"; $$.second = $3; }
   ;

args: arg                                       { $$.push_back($1); }
    | args sep arg                              { $1.push_back($3); $$.swap($1); }
    ;

num:       ONE                                  { $$ = new NumberExpression(DocumentObject, Quantity($1));                        }
         | NUM                                  { $$ = new NumberExpression(DocumentObject, Quantity($1));                        }
         | INTEGER                              { $$ = new NumberExpression(DocumentObject, Quantity((double)$1));                }
         | CONSTANT                             { $$ = new ConstantExpression(DocumentObject, $1.name, Quantity($1.fvalue));      }
         ;

range: id_or_cell ':' id_or_cell              { $$ = new RangeExpression(DocumentObject, $1, $3);                               }
     ;

unit_exp: UNIT                                  { $$ = new UnitExpression(DocumentObject, $1.scaler, $1.unitStr );                }
        | unit_exp '/' unit_exp                 { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::DIV, $3);   }
        | unit_exp '*' unit_exp                 { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::MUL, $3);   }
        | unit_exp '^' integer                  { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::POW, new NumberExpression(DocumentObject, Quantity((double)$3)));   }
        | unit_exp '^' MINUSSIGN integer        { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::POW, new OperatorExpression(DocumentObject, new NumberExpression(DocumentObject, Quantity((double)$4)), OperatorExpression::NEG, new NumberExpression(DocumentObject, Quantity(-1))));   }
        | '(' unit_exp ')'                      { $$ = $2;                                                                        }
        ;

identifier: path                                { /* Path to property of the current object */
                                                  $$ = ObjectIdentifier(DocumentObject);
                                                  $$.addComponents($1);
                                                }
          | '.' subname '.' subpath             { /* Path to property of a sub-object of the current object*/
                                                  $$ = ObjectIdentifier(DocumentObject);
                                                  $$.setDocumentObjectName(DocumentObject,false,$2);
                                                  $$.addComponents($4);
                                                }
          | '.' subpath                         { /* Path to property of the current document object */
                                                  $$ = ObjectIdentifier(DocumentObject);
                                                  $$.setDocumentObjectName(DocumentObject);
                                                  $$.addComponents($2);
                                                }
          | object '.' subname '.' subpath      { /* Path to property of a sub-object */
                                                  $$ = ObjectIdentifier(DocumentObject);
                                                  $$.setDocumentObjectName($1, true, $3);
                                                  $$.addComponents($5);
                                                  $$.resolveAmbiguity();
                                                }
          | object '.' subpath                  { /* Path to property of a given document object */
                                                  $$ = ObjectIdentifier(DocumentObject);
                                                  $$ << ObjectIdentifier::SimpleComponent($1);
                                                  $$.addComponents($3);
                                                  $$.resolveAmbiguity();
                                                }
          | document '#' object '.' subpath     { /* Path to property from an external document, within a named document object */
                                                  $$ = ObjectIdentifier(DocumentObject);
                                                  $$.setDocumentName($1, true);
                                                  $$.setDocumentObjectName($3, true);
                                                  $$.addComponents($5);
                                                  $$.resolveAmbiguity();
                                                }
     ;

integer: INTEGER { $$ = $1; }
       | ONE { $$ = $1; }
       ;

path: id_or_cell                                       { $$.push_back(ObjectIdentifier::SimpleComponent($1));}
    | path indexer                                     { $1.push_back($2); $$.swap($1);}
    ;

subpath: id_or_cell                                    { $$.push_back(ObjectIdentifier::SimpleComponent($1));}
       | subpath '.' id_or_cell                        { $1.push_back(ObjectIdentifier::SimpleComponent($3)); $$.swap($1); }
       | subpath indexer                               { $1.push_back($2); $$.swap($1);}
       ;

indexer: '[' integer ']'                               { $$ = ObjectIdentifier::ArrayComponent($2);   }
       | '[' integer ':' ']'                           { $$ = ObjectIdentifier::RangeComponent($2); }
       | '[' ':' integer ']'                           { $$ = ObjectIdentifier::RangeComponent(0,$3); }
       | '[' integer ':' integer ']'                   { $$ = ObjectIdentifier::RangeComponent($2,$4);}
       | '[' STRING ']'                                { $$ = ObjectIdentifier::MapComponent(ObjectIdentifier::String($2, true));}
       | '[' id_or_cell ']'                            { $$ = ObjectIdentifier::MapComponent($2);}
       ;

document: STRING                                       { $$ = ObjectIdentifier::String($1, true); }
        | id_or_cell                                   { $$ = ObjectIdentifier::String($1);       }
        ;

object: STRING                                         { $$ = ObjectIdentifier::String($1, true); }
      | id_or_cell                                     { $$ = ObjectIdentifier::String($1);       }
      ;

subname: STRING                                        { $$ = ObjectIdentifier::String($1, true); }
       ;

%%
