/* Parser for the FreeCAD  Units language           */
/* (c) 2010 Juergen Riegel  LGPL                    */
/* (c) 2015 Eivind Kvedalen LGPL                    */
/* (c) 2018 Zheng, Lei      LGPL                    */

/* Represents the many different ways we can access our data */

/* NOTE: this parser requires a patched version of bison 3.0.4. You can find
 * the patch file under this directroy named ExpressionParser.bison-3.0.4.patch.
 * The original patch is published at
 *
 * http://lists.gnu.org/archive/html/bug-bison/2018-03/msg00002.html
 */

%{

%}

%language "C++"

%require "3.0"

%define api.namespace {App::ExpressionParser}
%define api.value.type variant
%define api.token.prefix {TOK_}
%define api.stack.container ExpressionParserStack
%define parse.assert
%define parse.error verbose

%code {

#undef YY_DECL
#define YY_DECL int yylex(parser::semantic_type* yylval, Context &ctx)
namespace App { 
    namespace ExpressionParser {
        template <class T> void stack_prepare (ExpressionParserStack &s) { s.reserve (200); }

        YY_DECL;
    } 
}

}

%param { Context &ctx }

     /* Bison declarations.  */
     %token END  0  "end of input"
     %token ONE
     %token NUM
     %token IDENTIFIER
     %token UNIT
     %token INTEGER
     %token CONSTANT
     %token CELLADDRESS
     %token EQ NEQ LT GT GTE LTE AND_OP OR_OP IS NOT IS_NOT NOT_IN 
     %token AS RAISE TRY EXCEPT FINALLY IMPORT LAMBDA FROM
     %token POW_ASSIGN MUL_ASSIGN FDIV_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN SUB_ASSIGN MINUSSIGN FDIV
     %token PSTRING STRING
     %token EXPAND
     %token NEWLINE INDENT DEDENT
     %token IF ELIF ELSE WHILE FOR BREAK CONTINUE RETURN IN PY_BEGIN PY_END DEF PASS DEL GLOBAL NONLOCAL
     %type <std::string> module
     %type <StringList> id_list
     %type <FlagExpression> item
     %type <FlagExpressionList> items items2
     %type <NamedArgument> arg arg_def
     %type <NamedArgumentList> args arg_defs
     %type <ExpressionList> exp_list
     %type <VarList> target_list target_list2
     %type <ExpressionPtr> primary_exp unary_exp multiply_exp additive_exp relational_exp assignment_exp
     %type <ExpressionPtr> equality_exp or_exp and_exp power_exp assignment_exp2 assignment_exp1
     %type <ExpressionPtr> cond_exp nocond_exp lambda_exp lambda_nocond_exp
     %type <ExpressionPtr> input exp unit_exp indexable indexable2 num range callable target target2
     %type <ExpressionPtr> comp_for list tuple dict dict1 idict idict1 string pstring
     %type <ExpressionPtr> stmt statement if_stmt small_stmt simple_stmt while_stmt try_stmt
     %type <ExpressionPtr> function_stmt for_stmt compound_stmt suite import_stmt1 import_stmt2 import_stmt3
     %type <UnitInfo> UNIT
     %type <std::string> id_or_cell IDENTIFIER CELLADDRESS STRING
     %type <ExpressionString> PSTRING
     %type <Integer> INTEGER
     %type <double> ONE
     %type <double> NUM
     %type <UnitInfo> CONSTANT
     %type <ObjectIdentifier> identifier iden
     %type <ComponentPtr> indexer
     %type <ObjectIdentifier::String> document object
     %type <Integer> integer
     %left ONE NUM INTEGER CONSTANT
     %left IN IS NOT_IN IS_NOT EQ NEQ LT GT GTE LTE
     %left '?' ':'
     %left MINUSSIGN '+'
     %left '*' '/' '%'
     %precedence NUM_AND_UNIT
     %left '^'    /* exponentiation */
     %left NEG     /* negation--unary minus */
     %left NOT
     %left POS     /* unary plus */
     %left EXPAND

%start input

%%

input
    : statement                             { ctx.ScanResult = std::move($1); ctx.valueExpression = true;                                       }
    | unit_exp                              { ctx.ScanResult = std::move($1); ctx.unitExpression = true;                                        }
    | INDENT unit_exp DEDENT                { ctx.ScanResult = std::move($2); ctx.unitExpression = true;                                        }
    ;

primary_exp
    : num                                   { $$ = std::move($1);                                                                        }
    | num unit_exp %prec NUM_AND_UNIT       { $$ = OperatorExpression::create(ctx.obj, std::move($1), OP_UNIT, std::move($2));  }
    | string                                { $$ = std::move($1); }
    | identifier                            { $$ = VariableExpression::create(ctx.obj, std::move($1)); }
    | indexable                             { $$ = std::move($1); }
    | callable                              { $$ = std::move($1); }
    ;

string
    : STRING                                { $$ = StringExpression::create(ctx.obj,std::move($1)); }
    | string STRING                         { static_cast<StringExpression&>(*$1).append(std::move($2)); $$ = std::move($1); }
    ;

pstring
    : PSTRING                               { $$ = StringExpression::create(ctx.obj,std::move($1)); }
    | pstring PSTRING                       { static_cast<StringExpression&>(*$1).append(std::move($2)); $$ = std::move($1); }
    ;

indexable 
    : '(' exp ')'                           { $$ = std::move($2); }
    | tuple                                 { $$ = std::move($1); }
    | list                                  { $$ = std::move($1); }
    | dict                                  { $$ = std::move($1); }
    | idict                                 { $$ = std::move($1); }
    | pstring                               { $$ = std::move($1); }
    | identifier indexer                    { $$ = VariableExpression::create(ctx.obj,std::move($1)); $$->addComponent(std::move($2)); }
    | indexable indexer                     { $1->addComponent(std::move($2)); $$ = std::move($1); }
    | indexable '.' IDENTIFIER              { $1->addComponent(Expression::createComponent($3)); $$ = std::move($1); }
    ;

callable
    : identifier '(' ')'                    { $$ = CallableExpression::create(ctx.obj,std::move($1)); }
    | identifier '(' args ')'               { $$ = CallableExpression::create(ctx.obj, std::move($1), std::move($3.first), std::move($3.second)); }
    | UNIT '(' args ')'                     {   // This rule exists because of possible name clash of 
                                                // function and unit, e.g. min
                                                $$ = CallableExpression::create(ctx.obj, std::move($1.first), std::move($3.first), std::move($3.second));
                                            }
    | indexable '(' ')'                     { $$ = CallableExpression::create(ctx.obj, std::move($1)); }
    | indexable '(' args ')'                { $$ = CallableExpression::create(ctx.obj, std::move($1), std::move($3.first), std::move($3.second)); }
    | callable '(' ')'                      { $$ = CallableExpression::create(ctx.obj, std::move($1)); }
    | callable '(' args ')'                 { $$ = CallableExpression::create(ctx.obj, std::move($1), std::move($3.first), std::move($3.second)); }
    | indexable2                            { $$ = std::move($1); }
    ;

indexable2
    : callable indexer                      { $1->addComponent(std::move($2)); $$ = std::move($1); }
    | callable '.' IDENTIFIER               { $1->addComponent(Expression::createComponent($3)); $$ = std::move($1); }
    ;

unary_exp
    : primary_exp                           { $$ = std::move($1); }
    | MINUSSIGN unary_exp %prec NEG         { $$ = OperatorExpression::create(ctx.obj, std::move($2), OP_NEG, NumberExpression::create(ctx.obj, Quantity(-1))); }
    | NOT unary_exp %prec NEG               { $$ = OperatorExpression::create(ctx.obj, std::move($2), OP_NOT, NumberExpression::create(ctx.obj, Quantity(-1))); }
    | '+' unary_exp %prec POS               { $$ = OperatorExpression::create(ctx.obj, std::move($2), OP_POS, NumberExpression::create(ctx.obj, Quantity(1))); }
    ;

power_exp
    : unary_exp                             { $$ = std::move($1); }
    | power_exp '^' unary_exp               { $$ = OperatorExpression::create(ctx.obj, std::move($1), OP_POW, std::move($3));   }
    | power_exp EXPAND unary_exp            { $$ = OperatorExpression::create(ctx.obj, std::move($1), OP_POW2, std::move($3));   }

multiply_exp
    : power_exp                             { $$ = std::move($1); }
    | multiply_exp '*' power_exp            { $$ = OperatorExpression::create(ctx.obj, std::move($1), OP_MUL, std::move($3));   }
    | multiply_exp '/' power_exp            { $$ = OperatorExpression::create(ctx.obj, std::move($1), OP_DIV, std::move($3));   }
    | multiply_exp '/' unit_exp             { $$ = OperatorExpression::create(ctx.obj, std::move($1), OP_DIV, std::move($3));   }
    | multiply_exp '%' power_exp            { $$ = OperatorExpression::create(ctx.obj, std::move($1), OP_MOD, std::move($3));   }
    ;

additive_exp
    : multiply_exp                          { $$ = std::move($1); }
    | additive_exp '+' multiply_exp         { $$ = OperatorExpression::create(ctx.obj, std::move($1), OP_ADD, std::move($3));   }
    | additive_exp MINUSSIGN multiply_exp   { $$ = OperatorExpression::create(ctx.obj, std::move($1), OP_SUB, std::move($3));   }
    ;

relational_exp
    : additive_exp                          { $$ = std::move($1); }
    | relational_exp LT additive_exp        { $$ = OperatorExpression::create(ctx.obj, std::move($1), OP_LT, std::move($3));    }
    | relational_exp GT additive_exp        { $$ = OperatorExpression::create(ctx.obj, std::move($1), OP_GT, std::move($3));    }
    | relational_exp GTE additive_exp       { $$ = OperatorExpression::create(ctx.obj, std::move($1), OP_GTE, std::move($3));   }
    | relational_exp LTE additive_exp       { $$ = OperatorExpression::create(ctx.obj, std::move($1), OP_LTE, std::move($3));   }
    ;

equality_exp
    : relational_exp                        { $$ = std::move($1); }
    | equality_exp EQ relational_exp        { $$ = OperatorExpression::create(ctx.obj, std::move($1), OP_EQ, std::move($3));    }
    | equality_exp NEQ relational_exp       { $$ = OperatorExpression::create(ctx.obj, std::move($1), OP_NEQ, std::move($3));   }
    | equality_exp IS relational_exp        { $$ = OperatorExpression::create(ctx.obj, std::move($1), OP_IS, std::move($3));    }
    | equality_exp IS_NOT relational_exp    { $$ = OperatorExpression::create(ctx.obj, std::move($1), OP_IS_NOT, std::move($3));    }
    | equality_exp IN relational_exp        { $$ = OperatorExpression::create(ctx.obj, std::move($1), OP_IN, std::move($3));    }
    | equality_exp NOT_IN relational_exp    { $$ = OperatorExpression::create(ctx.obj, std::move($1), OP_NOT_IN, std::move($3));    }
    ;

and_exp
    : equality_exp                          { $$ = std::move($1); }
    | and_exp AND_OP equality_exp           { $$ = OperatorExpression::create(ctx.obj, std::move($1), OP_AND, std::move($3));   }
    ;

or_exp
    : and_exp                               { $$ = std::move($1); }
    | or_exp OR_OP and_exp                  { $$ = OperatorExpression::create(ctx.obj, std::move($1), OP_OR, std::move($3));   }
    ;

nocond_exp
    : or_exp                                { $$ = std::move($1); }
    | lambda_nocond_exp                     { $$ = std::move($1); }
    ;

cond_exp
    : or_exp                                { $$ = std::move($1); }
    | or_exp IF or_exp ELSE exp             { $$ = ConditionalExpression::create(ctx.obj, std::move($3), std::move($1), std::move($5), true); }
    | or_exp '?' exp ':' exp                { $$ = ConditionalExpression::create(ctx.obj, std::move($1), std::move($3), std::move($5));                     }
    ;

lambda_nocond_exp
    : LAMBDA ':' nocond_exp                 { $$ = LambdaExpression::create(ctx.obj, std::move($3)); }
    | LAMBDA arg_defs ':' nocond_exp        { $$ = LambdaExpression::create(ctx.obj, std::move($4), std::move($2.first), std::move($2.second)); }
    ;

lambda_exp
    : LAMBDA ':' exp                        { $$ = LambdaExpression::create(ctx.obj, std::move($3)); }
    | LAMBDA arg_defs ':' exp               { $$ = LambdaExpression::create(ctx.obj, std::move($4), std::move($2.first), std::move($2.second)); }
    ;

exp 
    : cond_exp                              { $$ = std::move($1); }
    | lambda_exp                            { $$ = std::move($1); }

id_list
    : IDENTIFIER                            { $$.push_back(std::move($1)); }
    | id_list ',' IDENTIFIER                { $1.push_back(std::move($3)); $$ = std::move($1); } 
    ;

target 
    : indexable                             { $$ = std::move($1); }
    | identifier                            { $$ = VariableExpression::create(ctx.obj,std::move($1)); }
    ;

target_list
    : target                                { $$.second=-1; $$.first.push_back(std::move($1)); }
    | '*' target                            { $$.second=0; $$.first.push_back(std::move($2)); }
    | target_list ',' target                { $1.first.push_back(std::move($3)); $$ = std::move($1); }
    | target_list ',' '*' target            { 
                                               if($1.second>=0) 
                                                   PARSER_THROW("Multiple catch all target"); 
                                               $1.second = (int)$1.first.size(); 
                                               $1.first.push_back(std::move($4)); 
                                               $$ = std::move($1); 
                                            }
    ;

target2 
    : target                                { $$ = std::move($1); }
    | indexable2                            { $$ = std::move($1); }
    ;

target_list2
    : target2                               { $$.second=-1; $$.first.push_back(std::move($1)); }
    | '*' target2                           { $$.second=0; $$.first.push_back(std::move($2)); }
    | target_list2 ',' target2              { $1.first.push_back(std::move($3)); $$ = std::move($1); }
    | target_list2 ',' '*' target2          { 
                                               if($1.second>=0) 
                                                   PARSER_THROW("Multiple catch all target"); 
                                               $1.second = (int)$1.first.size(); 
                                               $1.first.push_back(std::move($4)); 
                                               $$ = std::move($1); 
                                            }
    ;

assignment_exp1
    : target_list2 '=' exp_list             { $$ = AssignmentExpression::create(ctx.obj,$1.second,std::move($1.first),std::move($3)); }
    ;

exp_list 
    : exp                                   { $$.push_back(std::move($1)); }
    | exp_list ',' exp                      { $1.push_back(std::move($3)); $$ = std::move($1); }
    ;

assignment_exp2
    : target MUL_ASSIGN exp                 { $$ = AssignmentExpression::create(ctx.obj,std::move($1),std::move($3),OP_MUL); }
    | target POW_ASSIGN exp                 { $$ = AssignmentExpression::create(ctx.obj,std::move($1),std::move($3),OP_POW); }
    | target DIV_ASSIGN exp                 { $$ = AssignmentExpression::create(ctx.obj,std::move($1),std::move($3),OP_DIV); }
    | target FDIV_ASSIGN exp                { $$ = AssignmentExpression::create(ctx.obj,std::move($1),std::move($3),OP_FDIV); }
    | target MOD_ASSIGN exp                 { $$ = AssignmentExpression::create(ctx.obj,std::move($1),std::move($3),OP_MOD); }
    | target ADD_ASSIGN exp                 { $$ = AssignmentExpression::create(ctx.obj,std::move($1),std::move($3),OP_ADD); }
    | target SUB_ASSIGN exp                 { $$ = AssignmentExpression::create(ctx.obj,std::move($1),std::move($3),OP_SUB); }
    ;

assignment_exp
    : assignment_exp1                       { $$ = std::move($1); }
    | assignment_exp2                       { $$ = std::move($1); }
    ;

small_stmt
    : assignment_exp                        { $$ = std::move($1); }
    | exp                                   { $$ = std::move($1); }
    | RETURN exp                            { $$ = JumpStatement::create(ctx.obj,JUMP_RETURN,std::move($2)); }
    | RETURN                                { $$ = JumpStatement::create(ctx.obj,JUMP_RETURN); }
    | BREAK                                 { $$ = JumpStatement::create(ctx.obj,JUMP_BREAK); }
    | CONTINUE                              { $$ = JumpStatement::create(ctx.obj,JUMP_CONTINUE); }
    | RAISE                                 { $$ = JumpStatement::create(ctx.obj,JUMP_RAISE); }
    | RAISE exp                             { $$ = JumpStatement::create(ctx.obj,JUMP_RAISE,std::move($2)); }
    | PY_BEGIN                              { $$ = PseudoStatement::create(ctx.obj,PY_BEGIN); }
    | PY_END                                { $$ = PseudoStatement::create(ctx.obj,PY_END); }
    | PASS                                  { $$ = PseudoStatement::create(ctx.obj,PY_PASS); }
    | DEL target_list                       { $$ = DelStatement::create(ctx.obj,std::move($2.first)); }
    | GLOBAL id_list                        { $$ = ScopeStatement::create(ctx.obj,std::move($2)); }
    | NONLOCAL id_list                      { $$ = ScopeStatement::create(ctx.obj,std::move($2),false); }
    | import_stmt1                          { $$ = std::move($1); }
    | import_stmt2                          { $$ = std::move($1); }
    | import_stmt3                          { $$ = std::move($1); }
    ;

simple_stmt
    : small_stmt                            { $$ = std::move($1); }
    | simple_stmt ';' small_stmt            { 
                                               SimpleStatement *stmt = dynamic_cast<SimpleStatement*>($1.get()); 
                                               if(!stmt) {
                                                   $1 = SimpleStatement::create(ctx.obj,std::move($1));
                                                   stmt = static_cast<SimpleStatement*>($1.get());
                                               }
                                               stmt->add(std::move($3)); 
                                               $$ = std::move($1);
                                            }
    | simple_stmt ';'                       { $$ = std::move($1); }
    ;

compound_stmt
    : if_stmt                               { $$ = std::move($1); }
    | if_stmt ELSE ':' suite                { static_cast<IfStatement&>(*$1).addElse(std::move($4)); $$= std::move($1); }
    | while_stmt                            { $$ = std::move($1); }
    | while_stmt ELSE ':' suite             { static_cast<WhileStatement&>(*$1).addElse(std::move($4)); $$= std::move($1); }
    | for_stmt                              { $$ = std::move($1); }
    | for_stmt ELSE ':' suite               { static_cast<ForStatement&>(*$1).addElse(std::move($4)); $$= std::move($1); }
    | function_stmt                         { $$ = std::move($1); } 
    | try_stmt                              { static_cast<TryStatement&>(*$1).check(); $$ = std::move($1); }
    ;

stmt
    : simple_stmt NEWLINE                   { $$ = Statement::create(ctx.obj, std::move($1)); }
    | compound_stmt                         { $$ = Statement::create(ctx.obj, std::move($1)); }
    | stmt simple_stmt NEWLINE              { static_cast<Statement&>(*$1).add(std::move($2)); $$ = std::move($1); }
    | stmt compound_stmt                    { static_cast<Statement&>(*$1).add(std::move($2)); $$ = std::move($1); }
    ;

statement
    : simple_stmt                           { $$ = std::move($1); }
    | INDENT simple_stmt DEDENT             { $$ = std::move($2); }
    | stmt                                  { $$ = std::move($1); }
    ;

suite
    : simple_stmt NEWLINE                   { $$ = std::move($1); }
    | NEWLINE INDENT stmt DEDENT            { $$ = std::move($3); }
    ;

if_stmt
    : IF exp ':' suite                      { $$ = IfStatement::create(ctx.obj,std::move($2),std::move($4)); }
    | if_stmt ELIF exp ':' suite            { static_cast<IfStatement&>(*$1).add(std::move($3),std::move($5)); $$ = std::move($1); }
    ;

while_stmt
    : WHILE exp ':' suite                   { $$ = WhileStatement::create(ctx.obj,std::move($2),std::move($4)); }
    ;

for_stmt
    : FOR target_list IN exp_list ':' suite { $$ = ForStatement::create(ctx.obj,$2.second,std::move($2.first),std::move($4),std::move($6)); }
    ;

try_stmt
    : TRY ':' suite                         { $$ = TryStatement::create(ctx.obj,std::move($3)); }
    | try_stmt EXCEPT ':' suite             { static_cast<TryStatement&>(*$1).add(std::move($4)); $$ = std::move($1); }
    | try_stmt EXCEPT exp ':' suite         { static_cast<TryStatement&>(*$1).add(std::move($5),std::move($3)); $$ = std::move($1); }
    | try_stmt EXCEPT exp AS IDENTIFIER ':' suite   { static_cast<TryStatement&>(*$1).add(std::move($7),std::move($3),std::move($5)); $$ = std::move($1); }
    | try_stmt ELSE ':' suite               { static_cast<TryStatement&>(*$1).addElse(std::move($4)); $$ = std::move($1); }
    | try_stmt FINALLY ':' suite            { static_cast<TryStatement&>(*$1).addFinal(std::move($4)); $$ = std::move($1); }
    ;

arg_def 
    : IDENTIFIER                            { $$.first = std::move($1); }
    | IDENTIFIER '=' exp                    { $$.first = std::move($1); $$.second = std::move($3); }
    | '*' IDENTIFIER                        { $$.first = "*"; $$.first+=$2; }
    | EXPAND IDENTIFIER                     { $$.first = "**"; $$.first+=$2; }
    ;

arg_defs
    : arg_def                               { $$.first.push_back(std::move($1.first)); $$.second.push_back(std::move($1.second)); }
    | arg_defs sep arg_def                  { $1.first.push_back(std::move($3.first)); $1.second.push_back(std::move($3.second)); $$ = std::move($1); }
    ;

function_stmt
    : DEF IDENTIFIER '(' ')' ':' suite      { $$ = FunctionStatement::create(ctx.obj, std::move($2), std::move($6)); }
    | DEF IDENTIFIER '(' arg_defs ')' ':' suite { $$ = FunctionStatement::create(ctx.obj, std::move($2), std::move($7), std::move($4.first), std::move($4.second)); }
    ;

module
    : IDENTIFIER                            { $$ = std::move($1); }
    | module '.' IDENTIFIER                 { $$ += "."; $$ += $3; }
    ;

import_stmt1
    : IMPORT module                         { $$ = ImportStatement::create(ctx.obj, std::move($2)); }
    | IMPORT module AS IDENTIFIER           { $$ = ImportStatement::create(ctx.obj, std::move($2), std::move($4)); }
    | import_stmt1 ',' module               { static_cast<ImportStatement&>(*$1).add(std::move($3)); $$ = std::move($1); }
    | import_stmt1 ',' module AS IDENTIFIER { static_cast<ImportStatement&>(*$1).add(std::move($3), std::move($5)); $$ = std::move($1); }
    ;

import_stmt2
    : FROM module IMPORT IDENTIFIER         { $$ = FromStatement::create(ctx.obj, std::move($2), std::move($4)); }
    | FROM module IMPORT IDENTIFIER AS IDENTIFIER { $$ = FromStatement::create(ctx.obj, std::move($2), std::move($4), std::move($4)); }
    | import_stmt2 ',' IDENTIFIER           { static_cast<FromStatement&>(*$1).add(std::move($3)); $$ = std::move($1); }
    | import_stmt2 ',' IDENTIFIER AS IDENTIFIER { static_cast<FromStatement&>(*$1).add(std::move($3), std::move($5)); $$ = std::move($1); }
    ;

import_stmt3
    : FROM module IMPORT '*'                { $$ = FromStatement::create(ctx.obj, std::move($2), std::string("*")); }
    ;

id_or_cell 
    : IDENTIFIER                            { $$ = std::move($1); }
    | CELLADDRESS                           { $$ = std::move($1); }
    ;

sep : ',' | ';' 

item  
    : exp                                   { $$.first = std::move($1); }
    | range                                 { $$.first = std::move($1); }
    | '*' exp                               { $$.first = std::move($2); $$.second = true; }
    | '*' range                             { $$.first = std::move($2); $$.second = true; }
    ;

items2 
    : item sep item                         { 
                                                $$.first.push_back(std::move($1.first)); 
                                                $$.second.push_back($1.second);
                                                $$.first.push_back(std::move($3.first)); 
                                                $$.second.push_back($3.second);
                                            }
    | items2 sep item                       { 
                                                $1.first.push_back(std::move($3.first)); 
                                                $1.second.push_back(std::move($3.second)); 
                                                $$ = std::move($1); 
                                            }
    ;

tuple 
    : '(' ')'                               { $$ = TupleExpression::create(ctx.obj); }
    | '(' item sep ')'                      { $$ = TupleExpression::create(ctx.obj, std::move($2.first), $2.second); }
    | '(' items2 ')'                        { $$ = TupleExpression::create(ctx.obj, std::move($2.first), std::move($2.second)); }
    | '(' items2 sep ')'                    { $$ = TupleExpression::create(ctx.obj, std::move($2.first), std::move($2.second)); }
    ;

items 
    : item                                  { $$.first.push_back(std::move($1.first)); $$.second.push_back($1.second); }
    | items sep item                        { $1.first.push_back(std::move($3.first)); $1.second.push_back($3.second); $$ = std::move($1); }
    ;

list 
    : '[' ']'                               { $$ = ListExpression::create(ctx.obj); }
    | '[' items ']'                         { $$ = ListExpression::create(ctx.obj, std::move($2.first), std::move($2.second)); }
    | '[' items sep ']'                     { $$ = ListExpression::create(ctx.obj, std::move($2.first), std::move($2.second)); }
    | '[' exp comp_for ']'                  { static_cast<ComprehensionExpression&>(*$3).setExpr(std::move($2)); $$ = std::move($3); }
    ;

comp_for
    : FOR target_list IN or_exp             { $$ = ComprehensionExpression::create(ctx.obj,$2.second,std::move($2.first),std::move($4)); }
    | comp_for FOR target_list IN or_exp    { static_cast<ComprehensionExpression&>(*$1).add($3.second,std::move($3.first),std::move($5)); $$ = std::move($1); }
    | comp_for IF nocond_exp                { static_cast<ComprehensionExpression&>(*$1).addCond(std::move($3)); $$ = std::move($1); }
    ;

dict_expand 
    : '*' | EXPAND 

dict1 
    : '{' exp ':' exp                       { $$ = DictExpression::create(ctx.obj, std::move($2), std::move($4)); }
    | '{' dict_expand exp                   { $$ = DictExpression::create(ctx.obj, std::move($3)); }
    | dict1 sep exp ':' exp                 { static_cast<DictExpression&>(*$1).addItem(std::move($3),std::move($5)); $$ = std::move($1); }
    | dict1 sep dict_expand exp             { static_cast<DictExpression&>(*$1).addItem(std::move($4)); $$ = std::move($1); }
    ;

dict 
    : '{' '}'                               { $$ = DictExpression::create(ctx.obj); }
    | dict1 '}'                             { $$ = std::move($1); }
    | dict1 sep '}'                         { $$ = std::move($1); }
    | '{' exp comp_for '}'                  { static_cast<ComprehensionExpression&>(*$3).setExpr(std::move($2),false); $$= std::move($3); }
    | '{' exp ':' exp comp_for '}'          { static_cast<ComprehensionExpression&>(*$5).setExpr(std::move($2),std::move($4)); $$= std::move($5); }
    ;

idict1 
    : '{' IDENTIFIER '=' exp                { $$ = IDictExpression::create(ctx.obj, std::move($2), std::move($4)); }
    | '{' dict_expand '=' exp               { $$ = IDictExpression::create(ctx.obj, "**", std::move($4)); }
    | idict1 sep IDENTIFIER '=' exp         { static_cast<IDictExpression&>(*$1).addItem(std::move($3),std::move($5)); $$ = std::move($1); }
    | idict1 sep dict_expand '=' exp        { static_cast<IDictExpression&>(*$1).addItem("**",std::move($5)); $$ = std::move($1); }
    ;

idict 
    : idict1 '}'                            { $$ = std::move($1); }
    | idict1 sep '}'                        { $$ = std::move($1); }
    ;

arg
    : exp                                   { $$.second = std::move($1); }
    | IDENTIFIER '=' exp                    { $$.first = std::move($1); $$.second = std::move($3); }
    | '*' exp                               { $$.first = "*"; $$.second = std::move($2); }
    | range                                 { $$.second = std::move($1); }
    | '*' range                             { $$.first = "*"; $$.second = std::move($2); }
    | EXPAND exp                            { $$.first = "**"; $$.second = std::move($2); }
    ;

args
    : arg                                   { $$.first.push_back(std::move($1.first)); $$.second.push_back(std::move($1.second)); }
    | args sep arg                          { $1.first.push_back(std::move($3.first)); $1.second.push_back(std::move($3.second)); $$ = std::move($1); }
    ;

num
    : ONE                                   { $$ = NumberExpression::create(ctx.obj, std::move($1));                        }
    | NUM                                   { $$ = NumberExpression::create(ctx.obj, std::move($1));                        }
    | INTEGER                               { $$ = NumberExpression::create(ctx.obj, (double)$1);                }
    | CONSTANT                              { $$ = ConstantExpression::create(ctx.obj, std::move($1.first), $1.second);      }
    ;

range
    : id_or_cell ':' id_or_cell             { $$ = RangeExpression::create(ctx.obj, std::move($1), std::move($3));                               }
    ;

unit_exp
    : UNIT                                  { $$ = UnitExpression::create(ctx.obj, $1.second, std::move($1.first));                }
    /* | IN                                    { $$ = UnitExpression::create(ctx.obj, Quantity::Inch, "in");                } */
    | unit_exp '/' unit_exp                 { $$ = OperatorExpression::create(ctx.obj, std::move($1), OP_DIV, std::move($3));   }
    | unit_exp '*' unit_exp                 { $$ = OperatorExpression::create(ctx.obj, std::move($1), OP_MUL, std::move($3));   }
    | unit_exp '^' integer                  { $$ = OperatorExpression::create(ctx.obj, std::move($1), OP_POW, NumberExpression::create(ctx.obj, Quantity((double)$3)));   }
    | unit_exp '^' MINUSSIGN integer        { $$ = OperatorExpression::create(ctx.obj, std::move($1), OP_POW, OperatorExpression::create(ctx.obj, NumberExpression::create(ctx.obj, Quantity((double)$4)), OP_NEG, NumberExpression::create(ctx.obj, Quantity(-1))));   }
    | '(' unit_exp ')'                      { $$ = std::move($2);                                                                        }
    ;

identifier 
    : id_or_cell                            { $$ = ObjectIdentifier(ctx.obj); $$ << ObjectIdentifier::SimpleComponent($1); }
    | iden                                  { $$ = std::move($1); }
    ;

iden 
    :  '.' STRING '.' id_or_cell            { /* Path to property of a sub-object of the current object*/
                                                $$ = ObjectIdentifier(ctx.obj,true);
                                                $$.setDocumentObjectName(ctx.obj,false,ObjectIdentifier::String(std::move($2),true),true);
                                                $$.addComponent(ObjectIdentifier::SimpleComponent($4));
                                            }
    | '.' id_or_cell                        { /* Path to property of the current document object */
                                                $$ = ObjectIdentifier(ctx.obj,true);
                                                $$.setDocumentObjectName(ctx.obj);
                                                $$.addComponent(ObjectIdentifier::SimpleComponent($2));
                                            }
    | object '.' STRING '.' id_or_cell      { /* Path to property of a sub-object */
                                                $$ = ObjectIdentifier(ctx.obj);
                                                $$.setDocumentObjectName(std::move($1), true, ObjectIdentifier::String(std::move($3),true),true);
                                                $$.addComponent(ObjectIdentifier::SimpleComponent($5));
                                                $$.resolveAmbiguity();
                                            }
    | object '.' id_or_cell                 { /* Path to property of a given document object */
                                                $$ = ObjectIdentifier(ctx.obj);
                                                $1.checkImport(ctx.obj);
                                                $$.addComponent(ObjectIdentifier::SimpleComponent($1));
                                                $$.addComponent(ObjectIdentifier::SimpleComponent($3));
                                                $$.resolveAmbiguity();
                                            }
    | document '#' object '.' id_or_cell    { /* Path to property from an external document, within a named document object */
                                                $$ = ObjectIdentifier(ctx.obj);
                                                $$.setDocumentName(std::move($1), true);
                                                $$.setDocumentObjectName(std::move($3), true);
                                                $$.addComponent(ObjectIdentifier::SimpleComponent($5));
                                                $$.resolveAmbiguity();
                                            }
    | document '#' object '.' STRING '.' id_or_cell    
                                            {   $$ = ObjectIdentifier(ctx.obj);
                                                $$.setDocumentName(std::move($1), true);
                                                $$.setDocumentObjectName(std::move($3), true, ObjectIdentifier::String(std::move($5),true));
                                                $$.addComponent(ObjectIdentifier::SimpleComponent($7));
                                                $$.resolveAmbiguity();
                                            }
    | iden '.' IDENTIFIER                   { $$= std::move($1); $$.addComponent(ObjectIdentifier::SimpleComponent($3)); }
    ;

integer
    : INTEGER { $$ = std::move($1); }
    | ONE { $$ = std::move($1); }
    ;

indexer
    : '[' exp ']'                           { $$ = Expression::createComponent(std::move($2));   }
    | '[' exp ':' ']'                       { $$ = Expression::createComponent(std::move($2),ExpressionPtr(),ExpressionPtr(),true); }
    | '[' ':' exp ']'                       { $$ = Expression::createComponent(ExpressionPtr(),std::move($3)); }
    | '[' ':' ':' exp ']'                   { $$ = Expression::createComponent(ExpressionPtr(),ExpressionPtr(),std::move($4)); }
    | '[' exp ':' exp ']'                   { $$ = Expression::createComponent(std::move($2),std::move($4));}
    | '[' exp ':' ':' exp ']'               { $$ = Expression::createComponent(std::move($2),ExpressionPtr(),std::move($5)); }
    | '[' ':' exp ':' exp ']'               { $$ = Expression::createComponent(ExpressionPtr(),std::move($3),std::move($5)); }
    | '[' exp ':' exp ':' exp ']'           { $$ = Expression::createComponent(std::move($2),std::move($4),std::move($6));}
    ;

document
    : STRING                                { $$ = ObjectIdentifier::String(std::move($1), true); }
    | IDENTIFIER                            { $$ = ObjectIdentifier::String(std::move($1), false);}
    ;

object
    : STRING                                { $$ = ObjectIdentifier::String(std::move($1), true); }
    | id_or_cell                            { $$ = ObjectIdentifier::String(std::move($1), false);}
    ;

%%
