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
     %token MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN SUB_ASSIGN
     %token RSTRING STRING MINUSSIGN PROPERTY_REF
     %token DOCUMENT OBJECT
     %token EXPONENT
     %token EXPAND
     %token NEWLINE INDENT DEDENT
     %token IF ELIF ELSE WHILE FOR BREAK CONTINUE RETURN IN
     %type <string_list> id_list
     %type <named_argument> item arg
     %type <named_arguments> items items2 args
     %type <expr> primary_exp unary_exp multiply_exp additive_exp relational_exp assignment_exp
     %type <expr> equality_exp logical_or_exp logical_and_exp power_exp assignment_exp2 assignment_exp1
     %type <expr> input exp unit_exp indexable num range callable
     %type <expr> comprehension0 comprehension list tuple dict dict1 idict idict1
     %type <expr> stmt statement if_stmt small_stmt simple_stmt while_stmt for_stmt compound_stmt suite
     %type <quantity> UNIT
     %type <string> id_or_cell RSTRING STRING IDENTIFIER CELLADDRESS
     %type <ivalue> INTEGER
     %type <string> PROPERTY_REF
     %type <string> IF ELIF ELSE WHILE FOR BREAK CONTINUE RETURN IN
     %type <fvalue> ONE
     %type <fvalue> NUM
     %type <constant> CONSTANT
     %type <path> identifier
     %type <components> subpath
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
     %left EXPAND

%destructor { delete $$; } <expr>
/* %destructor { for(auto expr : $$) {delete expr;} } <arguments> */
%destructor { for(auto &v : $$) {delete v.second;} } <named_arguments>
%destructor { delete $$.second; } <named_argument>
%destructor { delete $$.e1; delete $$.e2; } <component>

%start input

%%

input:     statement                            { ScanResult = $1; valueExpression = true; $$=0;                                       }
     |     unit_exp                             { ScanResult = $1; unitExpression = true; $$=0;                                        }
     ;

primary_exp
        : num                			{ $$ = $1;                                                                        }
        | num unit_exp %prec NUM_AND_UNIT       { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::UNIT, $2);  }
        | STRING                                { $$ = new StringExpression(DocumentObject, $1); }
        | RSTRING                               { $$ = new StringExpression(DocumentObject, $1, true); }
        | identifier                            { $$ = new VariableExpression(DocumentObject, $1); }
        | indexable                             { $$ = $1; }
        | callable                              { $$ = $1; }
        | callable indexer                      { $1->addComponent($2); $$ = $1; }
        | callable '.' IDENTIFIER               { $1->addComponent($3); $$ = $1; }
        ;

indexable : '(' exp ')' 			{ $$ = $2; }
          | tuple                               { $$ = $1; }
          | list                                { $$ = $1; }
          | dict                                { $$ = $1; }
          | idict                               { $$ = $1; }
          | identifier indexer                  { $$ = new VariableExpression(DocumentObject,$1); $$->addComponent($2); }
          | indexable indexer                   { $1->addComponent($2); $$ = $1; }
          | indexable '.' IDENTIFIER            { $1->addComponent($3); $$ = $1; }
          ;

callable: identifier '(' ')'                 { 
                                                    $$ = CallableExpression::create(DocumentObject,$1);
                                                    if(!$$) { 
                                                        YYABORT;
                                                    } 
                                                }
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

exp
    : logical_or_exp                             { $$ = $1; }
    | logical_or_exp '?' exp ':' exp             { $$ = new ConditionalExpression(DocumentObject, $1, $3, $5);                     }
    ;

id_list
    : IDENTIFIER                                 { $$.push_back($1); }
    | id_list ',' IDENTIFIER                     { $1.push_back($3); $$.swap($1); } 
    ;

assignment_exp1
	: id_list '=' exp                        { $$ = new AssignmentExpression(DocumentObject,$1,$3); }
	| assignment_exp1 ',' exp                { static_cast<AssignmentExpression*>($1)->add($3); $$ = $1; }
	;

assignment_exp2
	: IDENTIFIER MUL_ASSIGN exp              { $$ = new AssignmentExpression(DocumentObject,$1,OperatorExpression::MUL,$3); }
	| IDENTIFIER DIV_ASSIGN exp              { $$ = new AssignmentExpression(DocumentObject,$1,OperatorExpression::DIV,$3); }
	| IDENTIFIER MOD_ASSIGN exp              { $$ = new AssignmentExpression(DocumentObject,$1,OperatorExpression::MOD,$3); }
	| IDENTIFIER ADD_ASSIGN exp              { $$ = new AssignmentExpression(DocumentObject,$1,OperatorExpression::ADD,$3); }
	| IDENTIFIER SUB_ASSIGN exp              { $$ = new AssignmentExpression(DocumentObject,$1,OperatorExpression::SUB,$3); }
	;

assignment_exp
        : assignment_exp1                        { $$ = $1; }
        | assignment_exp2                        { $$ = $1; }
        ;

small_stmt
        : assignment_exp                         { $$ = $1; }
        | exp                                    { $$ = $1; }
        | RETURN exp                             { $$ = new JumpStatement(DocumentObject,JumpStatement::JUMP_RETURN,$2); }
        | BREAK                                  { $$ = new JumpStatement(DocumentObject,JumpStatement::JUMP_BREAK); }
        | CONTINUE                               { $$ = new JumpStatement(DocumentObject,JumpStatement::JUMP_CONTINUE); }
        ;

simple_stmt
        : small_stmt                             { $$ = $1; }
        | simple_stmt ';' small_stmt             { 
                                                    SimpleStatement *stmt = dynamic_cast<SimpleStatement*>($1); 
                                                    if(!stmt)
                                                        stmt = new SimpleStatement(DocumentObject, $1);
                                                    stmt->add($3); 
                                                    $$ = stmt;
                                                 }
        | simple_stmt ';'                        { $$ = $1; }
        ;

compound_stmt
        : if_stmt                                { $$ = $1; }
        | if_stmt ELSE ':' suite                 { static_cast<IfStatement*>($1)->addElse($4); $$=$1; }
        | while_stmt                             { $$ = $1; }
        | for_stmt                               { $$ = $1; }
        | for_stmt ELSE ':' suite                { static_cast<ForStatement*>($1)->addElse($4); $$=$1; }
        ;

stmt
        : simple_stmt NEWLINE                    { $$ = new Statement(DocumentObject, $1); }
        | compound_stmt                          { $$ = new Statement(DocumentObject, $1); }
        | stmt simple_stmt NEWLINE               { static_cast<Statement*>($1)->add($2); $$ = $1; }
        | stmt compound_stmt                     { static_cast<Statement*>($1)->add($2); $$ = $1; }
        ;

statement
        : simple_stmt                            { $$ = $1; }
        | stmt                                   { $$ = $1; }
        ;
suite
        : simple_stmt NEWLINE                    { $$ = $1; }
        | NEWLINE INDENT stmt DEDENT             { $$ = $3; }
        ;

if_stmt
        : IF exp ':' suite                       { $$ = new IfStatement(DocumentObject,$2,$4); }
        | if_stmt ELIF exp ':' suite             { static_cast<IfStatement*>($$)->addElseIf($3,$5); $$=$1; }
        ;

while_stmt
        : WHILE exp ':' suite                    { $$ = new WhileStatement(DocumentObject,$2,$4); }

for_stmt
        : FOR id_list IN exp ':' suite           { $$ = new ForStatement(DocumentObject,$2,$4,$6); }
        ;

id_or_cell : IDENTIFIER                         { $$ = $1; }
           | CELLADDRESS                        { $$ = $1; }
           ;

sep : ',' | ';' 

item  : exp                                     { $$.first.clear(); $$.second = $1; }
      | range                                   { $$.first.clear(); $$.second = $1; }
      | '*' exp                                 { $$.first = "*"; $$.second = $2; }
      | '*' range                               { $$.first = "*"; $$.second = $2; }
      ;

items2 : item sep item                          { $$.push_back($1); $$.push_back($3); }
       | items2 sep item                        { $1.push_back($3); $$.swap($1); }
       ;

tuple : '(' sep ')'                             { $$ = new TupleExpression(DocumentObject); }
      | '(' item sep ')'                        { $$ = new TupleExpression(DocumentObject, $2); }
      | '(' items2 ')'                          { $$ = new TupleExpression(DocumentObject, $2); }
      | '(' items2 sep ')'                      { $$ = new TupleExpression(DocumentObject, $2); }
      ;

items : item                                    { $$.push_back($1); }
      | items sep item                          { $1.push_back($3); $$.swap($1); }
      ;

list : '[' ']'                                  { $$ = new ListExpression(DocumentObject); }
     | '[' items ']'                            { $$ = new ListExpression(DocumentObject, $2); }
     | '[' items sep ']'                        { $$ = new ListExpression(DocumentObject, $2); }
     | '[' exp comprehension ']'                { static_cast<ComprehensionExpression*>($3)->setExpr($2); $$ = $3; }
     ;

comprehension0
    : FOR id_list IN exp                        { $$ = new ComprehensionExpression(DocumentObject,$2,$4); }
    | comprehension0 FOR id_list IN exp         { static_cast<ComprehensionExpression*>($1)->add($3,$5); $$ = $1; }
    ;

comprehension
    : comprehension0                            { $$ = $1; }
    | comprehension0 IF exp                     { static_cast<ComprehensionExpression*>($1)->setCondition($3); $$ = $1; } 
    ;

dict_expand : '*' | EXPAND 

dict1 : '{' exp ':' exp                         { $$ = new DictExpression(DocumentObject, $2, $4); }
      | '{' dict_expand exp                     { $$ = new DictExpression(DocumentObject, 0, $3); }
      | dict1 sep exp ':' exp                   { static_cast<DictExpression*>($1)->addItem($3,$5); $$ = $1; }
      | dict1 sep dict_expand exp               { static_cast<DictExpression*>($1)->addItem(0,$4); $$ = $1; }
      ;

dict : '{' '}'                                  { $$ = new DictExpression(DocumentObject); }
     | dict1 '}'                                { $$ = $1; }
     | dict1 sep '}'                            { $$ = $1; }
     | '{' exp comprehension '}'                { static_cast<ComprehensionExpression*>($3)->setExpr($2,0,false); $$=$3; }
     | '{' exp ':' exp comprehension '}'        { static_cast<ComprehensionExpression*>($5)->setExpr($2,$4,false); $$=$5; }
     ;

idict1 : '{' IDENTIFIER '=' exp                 { $$ = new IDictExpression(DocumentObject, $2, $4); }
       | '{' dict_expand '=' exp                { $$ = new IDictExpression(DocumentObject, "**", $4); }
       | idict1 sep IDENTIFIER '=' exp          { static_cast<IDictExpression*>($1)->addItem($3,$5); $$ = $1; }
       | idict1 sep dict_expand '=' exp         { static_cast<IDictExpression*>($1)->addItem("**",$5); $$ = $1; }
       ;

idict : idict1 '}'                              { $$ = $1; }
      | idict1 sep '}'                          { $$ = $1; }
      ;

arg: exp                                        { $$.first.clear(); $$.second = $1; }
   | IDENTIFIER '=' exp                         { $$.first = $1; $$.second = $3; }
   | '*' exp                                    { $$.first = "*"; $$.second = $2; }
   | range                                      { $$.first.clear(); $$.second = $1; }
   | '*' range                                  { $$.first = '*'; $$.second = $2; }
   | EXPAND exp                                 { $$.first = "**"; $$.second = $2; }
   ;

args: arg                                       { $$.push_back($1); }
    | args sep arg                              { $1.push_back($3); $$.swap($1); }
    ;

num:       ONE                                  { $$ = new NumberExpression(DocumentObject, Quantity($1));                        }
         | NUM                                  { $$ = new NumberExpression(DocumentObject, Quantity($1));                        }
         | INTEGER                              { $$ = new NumberExpression(DocumentObject, Quantity((double)$1));                }
         | CONSTANT                             { $$ = new ConstantExpression(DocumentObject, $1.name, Quantity($1.fvalue));      }
         ;

range: id_or_cell ':' id_or_cell                { $$ = new RangeExpression(DocumentObject, $1, $3);                               }
     ;

unit_exp: UNIT                                  { $$ = new UnitExpression(DocumentObject, $1.scaler, $1.unitStr );                }
        | IN                                    { $$ = new UnitExpression(DocumentObject, Quantity::Inch, $1);                }
        | unit_exp '/' unit_exp                 { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::DIV, $3);   }
        | unit_exp '*' unit_exp                 { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::MUL, $3);   }
        | unit_exp '^' integer                  { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::POW, new NumberExpression(DocumentObject, Quantity((double)$3)));   }
        | unit_exp '^' MINUSSIGN integer        { $$ = new OperatorExpression(DocumentObject, $1, OperatorExpression::POW, new OperatorExpression(DocumentObject, new NumberExpression(DocumentObject, Quantity((double)$4)), OperatorExpression::NEG, new NumberExpression(DocumentObject, Quantity(-1))));   }
        | '(' unit_exp ')'                      { $$ = $2;                                                                        }
        ;

identifier: id_or_cell                          { /* Path to property of the current object */
                                                  $$ = ObjectIdentifier(DocumentObject) << ObjectIdentifier::SimpleComponent($1);
                                                }
          | '.' subname '.' subpath             { /* Path to property of a sub-object of the current object*/
                                                  $$ = ObjectIdentifier(DocumentObject);
                                                  if(_Reader) {
                                                    $2 = ObjectIdentifier::String(
                                                        PropertyLinkBase::importSubName(*_Reader,$2.getString().c_str()),true);
                                                  }
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
                                                  if(_Reader) {
                                                    $3 = ObjectIdentifier::String(
                                                        PropertyLinkBase::importSubName(*_Reader,$3.getString().c_str()),true);
                                                    if(!$1.isRealString())
                                                        $1 = ObjectIdentifier::String(_Reader->getName($1.getString().c_str()));
                                                  }
                                                  $$.setDocumentObjectName($1, true, $3);
                                                  $$.addComponents($5);
                                                  $$.resolveAmbiguity();
                                                }
          | object '.' subpath                  { /* Path to property of a given document object */
                                                  $$ = ObjectIdentifier(DocumentObject);
                                                  if(_Reader && !$1.isRealString())
                                                      $1 = ObjectIdentifier::String(_Reader->getName($1.getString().c_str()));
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

subpath: id_or_cell                                    { $$.push_back(ObjectIdentifier::SimpleComponent($1));}
       | subpath '.' IDENTIFIER                        { $1.push_back(ObjectIdentifier::SimpleComponent($3)); $$.swap($1); }
       ;

indexer: '[' exp ']'                                   { $$ = Expression::Component($2);   }
       | '[' exp ':' ']'                               { $$ = Expression::Component($2,0,0,true); }
       | '[' ':' exp ']'                               { $$ = Expression::Component(0,$3); }
       | '[' ':' ':' exp ']'                           { $$ = Expression::Component(0,0,$4); }
       | '[' exp ':' exp ']'                           { $$ = Expression::Component($2,$4);}
       | '[' exp ':' ':' exp ']'                       { $$ = Expression::Component($2,0,$5); }
       | '[' ':' exp ':' exp ']'                       { $$ = Expression::Component(0,$3,$5); }
       | '[' exp ':' exp ':' exp ']'                   { $$ = Expression::Component($2,$4,$6);}
       ;

document: STRING                                       { $$ = ObjectIdentifier::String($1, true); }
        | IDENTIFIER                                   { $$ = ObjectIdentifier::String($1);       }
        ;

object: STRING                                         { $$ = ObjectIdentifier::String($1, true); }
      | id_or_cell                                     { $$ = ObjectIdentifier::String($1);       }
      ;

subname: STRING                                        { $$ = ObjectIdentifier::String($1, true); }
       ;

%%
