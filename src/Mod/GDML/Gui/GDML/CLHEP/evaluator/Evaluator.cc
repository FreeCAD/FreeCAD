// -*- C++ -*-
// $Id: Evaluator.cc,v 1.2.4.1 2008/07/17 19:00:45 garren Exp $
// ---------------------------------------------------------------------------

///#include "CLHEP/Evaluator/defs.h"
///#include "CLHEP/Evaluator/Evaluator.h"
#include "defs.h"
#include "Evaluator.h"

#include <iostream>
#include <cmath>	// for pow()
#include "stack.src"
#include "string.src"
#include "hash_map.src"
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>	// for strtod()

//---------------------------------------------------------------------------
// Fix non ISO C++ compliant cast from pointer to function
// to void*, which is a pointer to an object
typedef void (*voidfuncptr)();
struct Item {
  enum { UNKNOWN, VARIABLE, EXPRESSION, FUNCTION } what;
  double variable;
  string expression;
  // Fix non ISO C++ compliant cast from pointer to function
  // to void*, which is a pointer to an object
  //void   *function;
  voidfuncptr function;

  Item()         : what(UNKNOWN),   variable(0),expression(), function(0) {}
  Item(double x) : what(VARIABLE),  variable(x),expression(), function(0) {}
  Item(string x) : what(EXPRESSION),variable(0),expression(x),function(0) {}
  Item(voidfuncptr x) : what(FUNCTION),  variable(0),expression(), function(x) {}
};

typedef char * pchar;
typedef hash_map<string,Item> dic_type;

struct Struct {
  dic_type theDictionary;
  pchar    theExpression;
  pchar    thePosition;
  int      theStatus;
  double   theResult;
};

//---------------------------------------------------------------------------
#define EVAL HepTool::Evaluator

#define REMOVE_BLANKS \
for(pointer=name;;pointer++) if (!isspace(*pointer)) break; \
for(n=strlen(pointer);n>0;n--) if (!isspace(*(pointer+n-1))) break

#define SKIP_BLANKS                      \
for(;;pointer++) {                       \
  c = (pointer > end) ? '\0' : *pointer; \
  if (!isspace(c)) break;                \
}

#define EVAL_EXIT(STATUS,POSITION) endp = POSITION; return STATUS
#define MAX_N_PAR 5

static const char sss[MAX_N_PAR+2] = "012345";

enum { ENDL, LBRA, OR, AND, EQ, NE, GE, GT, LE, LT,
       PLUS, MINUS, MULT, DIV, POW, RBRA, VALUE };

static int engine(pchar, pchar, double &, pchar &, const dic_type &);

static int variable(const string & name, double & result,
		    const dic_type & dictionary)
/***********************************************************************
 *                                                                     *
 * Name: variable                                    Date:    03.10.00 *
 * Author: Evgeni Chernyaev                          Revised:          *
 *                                                                     *
 * Function: Finds value of the variable.                              * 
 *           This function is used by operand().                       *
 *                                                                     *
 * Parameters:                                                         *
 *   name   - name of the variable.                                    *
 *   result - value of the variable.                                   *
 *   dictionary - dictionary of available variables and functions.     *
 *                                                                     *
 ***********************************************************************/
{
  dic_type::const_iterator iter = dictionary.find(name);
  if (iter == dictionary.end())
    return EVAL::ERROR_UNKNOWN_VARIABLE;
  Item item = iter->second;
  switch (item.what) {
  case Item::VARIABLE:
    result = item.variable;
    return EVAL::OK;
  case Item::EXPRESSION: {
    pchar exp_begin = (char *)(item.expression.c_str());
    pchar exp_end   = exp_begin + strlen(exp_begin) - 1;
    if (engine(exp_begin, exp_end, result, exp_end, dictionary) == EVAL::OK)
      return EVAL::OK;
  }
  default:
    return EVAL::ERROR_CALCULATION_ERROR;
  }
}

static int function(const string & name, stack<double> & par,
		    double & result, const dic_type & dictionary) 
/***********************************************************************
 *                                                                     *
 * Name: function                                    Date:    03.10.00 *
 * Author: Evgeni Chernyaev                          Revised:          *
 *                                                                     *
 * Function: Finds value of the function.                              * 
 *           This function is used by operand().                       *
 *                                                                     *
 * Parameters:                                                         *
 *   name   - name of the function.                                    *
 *   par    - stack of parameters.                                     *
 *   result - value of the function.                                   *
 *   dictionary - dictionary of available variables and functions.     *
 *                                                                     *
 ***********************************************************************/
{
  int npar = par.size();
  if (npar > MAX_N_PAR) return EVAL::ERROR_UNKNOWN_FUNCTION;

  dic_type::const_iterator iter = dictionary.find(sss[npar]+name);
  if (iter == dictionary.end()) return EVAL::ERROR_UNKNOWN_FUNCTION;
  Item item = iter->second;

  double pp[MAX_N_PAR];
  for(int i=0; i<npar; i++) { pp[i] = par.top(); par.pop(); }
  errno = 0;
  if (item.function == 0)       return EVAL::ERROR_CALCULATION_ERROR;
  switch (npar) {
  case 0:
    result = ((double (*)())item.function)();
    break;  
  case 1:
    result = ((double (*)(double))item.function)(pp[0]);
    break;  
  case 2:
    result = ((double (*)(double,double))item.function)(pp[1], pp[0]);
    break;  
  case 3:
    result = ((double (*)(double,double,double))item.function)
      (pp[2],pp[1],pp[0]);
    break;  
  case 4:
    result = ((double (*)(double,double,double,double))item.function)
      (pp[3],pp[2],pp[1],pp[0]);
    break;  
  case 5:
    result = ((double (*)(double,double,double,double,double))item.function)
      (pp[4],pp[3],pp[2],pp[1],pp[0]);
    break;  
  }
  return (errno == 0) ? EVAL::OK : EVAL::ERROR_CALCULATION_ERROR;
}

static int operand(pchar begin, pchar end, double & result,
		   pchar & endp, const dic_type & dictionary) 
/***********************************************************************
 *                                                                     *
 * Name: operand                                     Date:    03.10.00 *
 * Author: Evgeni Chernyaev                          Revised:          *
 *                                                                     *
 * Function: Finds value of the operand. The operand can be either     * 
 *           a number or a variable or a function.                     *  
 *           This function is used by engine().                        * 
 *                                                                     *
 * Parameters:                                                         *
 *   begin  - pointer to the first character of the operand.           *
 *   end    - pointer to the last character of the character string.   *
 *   result - value of the operand.                                    *
 *   endp   - pointer to the character where the evaluation stoped.    *
 *   dictionary - dictionary of available variables and functions.     *
 *                                                                     *
 ***********************************************************************/
{
  pchar pointer = begin;
  int   EVAL_STATUS;
  char  c;

  //   G E T   N U M B E R

  if (!isalpha(*pointer)) {
    errno = 0;
    result = strtod(pointer, (char **)(&pointer));
    if (errno == 0) {
      EVAL_EXIT( EVAL::OK, --pointer );
    }else{
      EVAL_EXIT( EVAL::ERROR_CALCULATION_ERROR, begin );
    }
  }

  //   G E T   N A M E

  while(pointer <= end) {
    c = *pointer;
    if (c != '_' && !isalnum(c)) break;
    pointer++;
  }
  c = *pointer;
  *pointer = '\0';
  string name(begin);
  *pointer = c;

  //   G E T   V A R I A B L E

  result = 0.0;
  SKIP_BLANKS;
  if (c != '(') {
    EVAL_STATUS = variable(name, result, dictionary);
    EVAL_EXIT( EVAL_STATUS, (EVAL_STATUS == EVAL::OK) ? --pointer : begin);
  }

  //   G E T   F U N C T I O N

  stack<pchar>  pos;                // position stack 
  stack<double> par;                // parameter stack
  double        value;
  pchar         par_begin = pointer+1, par_end;

  for(;;pointer++) {
    c = (pointer > end) ? '\0' : *pointer;
    switch (c) {
    case '\0':  
      EVAL_EXIT( EVAL::ERROR_UNPAIRED_PARENTHESIS, pos.top() ); 
    case '(':
      pos.push(pointer); break;
    case ',':
      if (pos.size() == 1) {
	par_end = pointer-1;
	EVAL_STATUS = engine(par_begin, par_end, value, par_end, dictionary);
	if (EVAL_STATUS == EVAL::WARNING_BLANK_STRING)
	  { EVAL_EXIT( EVAL::ERROR_EMPTY_PARAMETER, --par_end ); }
	if (EVAL_STATUS != EVAL::OK)
	  { EVAL_EXIT( EVAL_STATUS, par_end ); }
	par.push(value);
	par_begin = pointer + 1;
      }
      break;
    case ')':
      if (pos.size() > 1) {
	pos.pop();
	break;
      }else{
	par_end = pointer-1;
	EVAL_STATUS = engine(par_begin, par_end, value, par_end, dictionary);
	switch (EVAL_STATUS) {
	case EVAL::OK:
	  par.push(value);
	  break;
	case EVAL::WARNING_BLANK_STRING:
	  if (par.size() != 0)
	    { EVAL_EXIT( EVAL::ERROR_EMPTY_PARAMETER, --par_end ); }
	  break;
	default:
	  EVAL_EXIT( EVAL_STATUS, par_end );
	}
	EVAL_STATUS = function(name, par, result, dictionary);
	EVAL_EXIT( EVAL_STATUS, (EVAL_STATUS == EVAL::OK) ? pointer : begin);
      }
    }
  }
}

/***********************************************************************
 *                                                                     *
 * Name: maker                                       Date:    28.09.00 *
 * Author: Evgeni Chernyaev                          Revised:          *
 *                                                                     *
 * Function: Executes basic arithmetic operations on values in the top *
 *           of the stack. Result is placed back into the stack.       *
 *           This function is used by engine().                        * 
 *                                                                     *
 * Parameters:                                                         *
 *   op  - code of the operation.                                      *
 *   val - stack of values.                                            *
 *                                                                     *
 ***********************************************************************/
static int maker(int op, stack<double> & val)
{
  if (val.size() < 2) return EVAL::ERROR_SYNTAX_ERROR;
  double val2 = val.top(); val.pop();
  double val1 = val.top();
  switch (op) {
  case OR:                                // operator ||
    val.top() = (val1 || val2) ? 1. : 0.;
    return EVAL::OK;
  case AND:                               // operator &&
    val.top() = (val1 && val2) ? 1. : 0.;
    return EVAL::OK;
  case EQ:                                // operator ==
    val.top() = (val1 == val2) ? 1. : 0.;
    return EVAL::OK;
  case NE:                                // operator !=
    val.top() = (val1 != val2) ? 1. : 0.;
    return EVAL::OK;
  case GE:                                // operator >=
    val.top() = (val1 >= val2) ? 1. : 0.;
    return EVAL::OK;
  case GT:                                // operator >
    val.top() = (val1 >  val2) ? 1. : 0.;
    return EVAL::OK;
  case LE:                                // operator <=
    val.top() = (val1 <= val2) ? 1. : 0.;
    return EVAL::OK;
  case LT:                                // operator <
    val.top() = (val1 <  val2) ? 1. : 0.;
    return EVAL::OK;
  case PLUS:                              // operator '+'
    val.top() = val1 + val2;
    return EVAL::OK;
  case MINUS:                             // operator '-'
    val.top() = val1 - val2;
    return EVAL::OK;
  case MULT:                              // operator '*'
    val.top() = val1 * val2;
    return EVAL::OK;
  case DIV:                               // operator '/'
    if (val2 == 0.0) return EVAL::ERROR_CALCULATION_ERROR;
    val.top() = val1 / val2;
    return EVAL::OK;
  case POW:                               // operator '^' (or '**')
    errno = 0;
    val.top() = pow(val1,val2);
    if (errno == 0) return EVAL::OK;
  default:
    return EVAL::ERROR_CALCULATION_ERROR;
  }
}

/***********************************************************************
 *                                                                     *
 * Name: engine                                      Date:    28.09.00 *
 * Author: Evgeni Chernyaev                          Revised:          *
 *                                                                     *
 * Function: Evaluates arithmetic expression.                          *
 *                                                                     *
 * Parameters:                                                         *
 *   begin  - pointer to the character string with expression.         *
 *   end    - pointer to the end of the character string (it is needed *
 *            for recursive call of engine(), when there is no '\0').  *
 *   result - result of the evaluation.                                *
 *   endp   - pointer to the character where the evaluation stoped.    *
 *   dictionary - dictionary of available variables and functions.     *
 *                                                                     *
 ***********************************************************************/
static int engine(pchar begin, pchar end, double & result,
		  pchar & endp, const dic_type & dictionary)
{
  static const int SyntaxTable[17][17] = {
    //E  (  || && == != >= >  <= <  +  -  *  /  ^  )  V - current token
    { 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 1 },   // E - previous
    { 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 1 },   // (   token
    { 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },   // ||
    { 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },   // &&
    { 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },   // ==
    { 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },   // !=
    { 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },   // >=
    { 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },   // >
    { 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },   // <=
    { 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },   // <
    { 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },   // +
    { 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },   // -
    { 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },   // *
    { 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },   // /
    { 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },   // ^
    { 3, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0 },   // )
    { 3, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0 }    // V = {.,N,C}
  };
  static const int ActionTable[15][16] = {
    //E  (  || && == != >= >  <= <  +  -  *  /  ^  ) - current operator
    { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1 }, // E - top operator
    {-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3 }, // (   in stack
    { 4, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4 }, // ||
    { 4, 1, 4, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4 }, // &&
    { 4, 1, 4, 4, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4 }, // ==
    { 4, 1, 4, 4, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4 }, // !=
    { 4, 1, 4, 4, 4, 4, 2, 2, 2, 2, 1, 1, 1, 1, 1, 4 }, // >=
    { 4, 1, 4, 4, 4, 4, 2, 2, 2, 2, 1, 1, 1, 1, 1, 4 }, // >
    { 4, 1, 4, 4, 4, 4, 2, 2, 2, 2, 1, 1, 1, 1, 1, 4 }, // <=
    { 4, 1, 4, 4, 4, 4, 2, 2, 2, 2, 1, 1, 1, 1, 1, 4 }, // <
    { 4, 1, 4, 4, 4, 4, 4, 4, 4, 4, 2, 2, 1, 1, 1, 4 }, // +
    { 4, 1, 4, 4, 4, 4, 4, 4, 4, 4, 2, 2, 1, 1, 1, 4 }, // -
    { 4, 1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 2, 2, 1, 4 }, // *
    { 4, 1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 2, 2, 1, 4 }, // /
    { 4, 1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 }  // ^
  };

  stack<int>    op;                      // operator stack
  stack<pchar>  pos;                     // position stack
  stack<double> val;                     // value stack
  double        value;
  pchar         pointer = begin;
  int           iWhat, iCur, iPrev = 0, iTop, EVAL_STATUS;
  char          c;

  op.push(0); pos.push(pointer);         // push EOL to the stack
  SKIP_BLANKS;
  if (c == '\0') { EVAL_EXIT( EVAL::WARNING_BLANK_STRING, begin ); }
  for(;;pointer++) {

    //   N E X T   T O K E N

    c = (pointer > end) ? '\0' : *pointer;
    if (isspace(c)) continue;            // skip space, tab etc.
    switch (c) {
    case '\0': iCur = ENDL; break;
    case '(':  iCur = LBRA; break;
    case '|':
      if (*(pointer+1) == '|') {
	pointer++; iCur = OR; break;
      }else{
        EVAL_EXIT( EVAL::ERROR_UNEXPECTED_SYMBOL, pointer );
      }
    case '&':
      if (*(pointer+1) == '&') {
	pointer++; iCur = AND; break;
      }else{
        EVAL_EXIT( EVAL::ERROR_UNEXPECTED_SYMBOL, pointer );
      }
    case '=':
      if (*(pointer+1) == '=') {
	pointer++; iCur = EQ; break;
      }else{
        EVAL_EXIT( EVAL::ERROR_UNEXPECTED_SYMBOL, pointer );
      }
    case '!':
      if (*(pointer+1) == '=') {
	pointer++; iCur = NE; break;
      }else{
        EVAL_EXIT( EVAL::ERROR_UNEXPECTED_SYMBOL, pointer );
      }
    case '>':
      if (*(pointer+1) == '=') { pointer++; iCur = GE; } else { iCur = GT; }
      break;
    case '<':
      if (*(pointer+1) == '=') { pointer++; iCur = LE; } else { iCur = LT; }
      break;
    case '+':  iCur = PLUS;  break;
    case '-':  iCur = MINUS; break;
    case '*':
      if (*(pointer+1) == '*') { pointer++; iCur = POW; }else{ iCur = MULT; }
      break;
    case '/':  iCur = DIV;  break;
    case '^':  iCur = POW;  break;
    case ')':  iCur = RBRA; break;
    default:
      if (c == '.' || isalnum(c)) {
        iCur = VALUE; break;
      }else{
        EVAL_EXIT( EVAL::ERROR_UNEXPECTED_SYMBOL, pointer );
      }
    }

    //   S Y N T A X   A N A L I S Y S

    iWhat = SyntaxTable[iPrev][iCur];
    iPrev = iCur;
    switch (iWhat) {
    case 0:                             // systax error
      EVAL_EXIT( EVAL::ERROR_SYNTAX_ERROR, pointer );
    case 1:                             // operand: number, variable, function
      EVAL_STATUS = operand(pointer, end, value, pointer, dictionary);
      if (EVAL_STATUS != EVAL::OK) { EVAL_EXIT( EVAL_STATUS, pointer ); }
      val.push(value);
      continue;
    case 2:                             // unary + or unary -
      val.push(0.0);
    case 3: default:                    // next operator
      break;
    }

    //   N E X T   O P E R A T O R

    for(;;) {
      if (op.size() == 0) { EVAL_EXIT( EVAL::ERROR_SYNTAX_ERROR, pointer ); }
      iTop = op.top();
      switch (ActionTable[iTop][iCur]) {
      case -1:                           // syntax error 
	if (op.size() > 1) pointer = pos.top();
	EVAL_EXIT( EVAL::ERROR_UNPAIRED_PARENTHESIS, pointer );
      case 0:                            // last operation (assignment)
        if (val.size() == 1) {
	  result = val.top();
	  EVAL_EXIT( EVAL::OK, pointer );
	}else{
	  EVAL_EXIT( EVAL::ERROR_SYNTAX_ERROR, pointer );
	}
      case 1:                           // push current operator in stack
	op.push(iCur); pos.push(pointer);
	break;
      case 2:                           // execute top operator
        EVAL_STATUS = maker(iTop, val); // put current operator in stack
        if (EVAL_STATUS != EVAL::OK) {
	  EVAL_EXIT( EVAL_STATUS, pos.top() );
	}
	op.top() = iCur; pos.top() = pointer;
	break;
      case 3:                           // delete '(' from stack
        op.pop(); pos.pop();
	break;
      case 4: default:                  // execute top operator and 
        EVAL_STATUS = maker(iTop, val); // delete it from stack
        if (EVAL_STATUS != EVAL::OK) {  // repete with the same iCur 
	  EVAL_EXIT( EVAL_STATUS, pos.top() );
	}
	op.pop(); pos.pop();
        continue;
      }
      break;
    }
  }
}

//---------------------------------------------------------------------------
static void setItem(const char * prefix, const char * name,
		    const Item & item, Struct * s) {

  if (name == 0 || *name == '\0') {
    s->theStatus = EVAL::ERROR_NOT_A_NAME;
    return;
  }

  //   R E M O V E   L E A D I N G   A N D   T R A I L I N G   S P A C E S

  const char * pointer; int n; REMOVE_BLANKS;

  //   C H E C K   N A M E 
 
  if (n == 0) {
    s->theStatus = EVAL::ERROR_NOT_A_NAME;
    return;
  }
  for(int i=0; i<n; i++) {
    char c = *(pointer+i);
    if (c != '_' && !isalnum(c)) {
      s->theStatus = EVAL::ERROR_NOT_A_NAME;
      return;
    }
  }

  //   A D D   I T E M   T O   T H E   D I C T I O N A R Y

  string item_name = prefix + string(pointer,n);
  dic_type::iterator iter = (s->theDictionary).find(item_name);
  if (iter != (s->theDictionary).end()) {
    iter->second = item;
    if (item_name == name) {
      s->theStatus = EVAL::WARNING_EXISTING_VARIABLE;
    }else{
      s->theStatus = EVAL::WARNING_EXISTING_FUNCTION;
    }
  }else{
    (s->theDictionary)[item_name] = item;
    s->theStatus = EVAL::OK;
  }
} 
		    
//---------------------------------------------------------------------------
namespace HepTool {

//---------------------------------------------------------------------------
Evaluator::Evaluator() {
  Struct * s = new Struct();
  p = (void *) s;
  s->theExpression = 0;
  s->thePosition   = 0;
  s->theStatus     = OK;
  s->theResult     = 0.0;
}

//---------------------------------------------------------------------------
Evaluator::~Evaluator() {
  delete (Struct *)(p);
}

//---------------------------------------------------------------------------
double Evaluator::evaluate(const char * expression) {
  Struct * s = (Struct *)(p);
  if (s->theExpression != 0) { delete[] s->theExpression; }
  s->theExpression = 0;
  s->thePosition   = 0;
  s->theStatus     = WARNING_BLANK_STRING;
  s->theResult     = 0.0;
  if (expression != 0) {
    s->theExpression = new char[strlen(expression)+1];
    strcpy(s->theExpression, expression);
    s->theStatus = engine(s->theExpression,
			  s->theExpression+strlen(expression)-1,
			  s->theResult,
			  s->thePosition,
			  s->theDictionary);
  }
  return s->theResult;
}

//---------------------------------------------------------------------------
int Evaluator::status() const {
  return ((Struct *)(p))->theStatus;
}

//---------------------------------------------------------------------------
int Evaluator::error_position() const {
  return ((Struct *)(p))->thePosition - ((Struct *)(p))->theExpression;
}

//---------------------------------------------------------------------------
void Evaluator::print_error() const {
  char prefix[] = "Evaluator : ";
  Struct * s = (Struct *) p;
  switch (s->theStatus) {
  case ERROR_NOT_A_NAME:
    std::cerr << prefix << "invalid name"         << std::endl;
    return;
  case ERROR_SYNTAX_ERROR:
    std::cerr << prefix << "systax error"         << std::endl;
    return;
  case ERROR_UNPAIRED_PARENTHESIS:
    std::cerr << prefix << "unpaired parenthesis" << std::endl;
    return;
  case ERROR_UNEXPECTED_SYMBOL:
    std::cerr << prefix << "unexpected symbol"    << std::endl;
    return;
  case ERROR_UNKNOWN_VARIABLE:
    std::cerr << prefix << "unknown variable"     << std::endl;
    return;
  case ERROR_UNKNOWN_FUNCTION:
    std::cerr << prefix << "unknown function"     << std::endl;
    return;
  case ERROR_EMPTY_PARAMETER: 
    std::cerr << prefix << "empty parameter in function call"
		 << std::endl;
    return;
  case ERROR_CALCULATION_ERROR:
    std::cerr << prefix << "calculation error"    << std::endl;
    return;
  default:
    return;
  }
}

//---------------------------------------------------------------------------
void Evaluator::setVariable(const char * name, double value)
{ setItem("", name, Item(value), (Struct *)p); }

void Evaluator::setVariable(const char * name, const char * expression)
{ setItem("", name, Item(expression), (Struct *)p); }

//---------------------------------------------------------------------------
// Fix non ISO C++ compliant cast from pointer to function
// to void*, which is a pointer to an object
void Evaluator::setFunction(const char * name,
			    double (*fun)())
{ setItem("0", name, Item(reinterpret_cast<voidfuncptr>(fun)), (Struct *)p); }

void Evaluator::setFunction(const char * name,
			    double (*fun)(double))
{ setItem("1", name, Item(reinterpret_cast<voidfuncptr>(fun)), (Struct *)p); }

void Evaluator::setFunction(const char * name,
			    double (*fun)(double,double))
{ setItem("2", name, Item(reinterpret_cast<voidfuncptr>(fun)), (Struct *)p); }

void Evaluator::setFunction(const char * name,
			    double (*fun)(double,double,double))
{ setItem("3", name, Item(reinterpret_cast<voidfuncptr>(fun)), (Struct *)p); }

void Evaluator::setFunction(const char * name,
			    double (*fun)(double,double,double,double))
{ setItem("4", name, Item(reinterpret_cast<voidfuncptr>(fun)), (Struct *)p); }

void Evaluator::setFunction(const char * name,
			    double (*fun)(double,double,double,double,double))
{ setItem("5", name, Item(reinterpret_cast<voidfuncptr>(fun)), (Struct *)p); }

//---------------------------------------------------------------------------
bool Evaluator::findVariable(const char * name) const {
  if (name == 0 || *name == '\0') return false;
  const char * pointer; int n; REMOVE_BLANKS;
  if (n == 0) return false;
  Struct * s = (Struct *)(p);
  return
    ((s->theDictionary).find(string(pointer,n)) == (s->theDictionary).end()) ?
    false : true;
}

//---------------------------------------------------------------------------
bool Evaluator::findFunction(const char * name, int npar) const {
  if (name == 0 || *name == '\0')    return false;
  if (npar < 0  || npar > MAX_N_PAR) return false;
  const char * pointer; int n; REMOVE_BLANKS;
  if (n == 0) return false;
  Struct * s = (Struct *)(p);
  return ((s->theDictionary).find(sss[npar]+string(pointer,n)) ==
	  (s->theDictionary).end()) ? false : true;
}

//---------------------------------------------------------------------------
void Evaluator::removeVariable(const char * name) {
  if (name == 0 || *name == '\0') return;
  const char * pointer; int n; REMOVE_BLANKS;
  if (n == 0) return;
  Struct * s = (Struct *)(p);
  (s->theDictionary).erase(string(pointer,n));
}

//---------------------------------------------------------------------------
void Evaluator::removeFunction(const char * name, int npar) {
  if (name == 0 || *name == '\0')    return;
  if (npar < 0  || npar > MAX_N_PAR) return;
  const char * pointer; int n; REMOVE_BLANKS;
  if (n == 0) return;
  Struct * s = (Struct *)(p);
  (s->theDictionary).erase(sss[npar]+string(pointer,n));
}

//---------------------------------------------------------------------------
void Evaluator::clear() {
  Struct * s = (Struct *) p;
  s->theDictionary.clear();
  s->theExpression = 0;
  s->thePosition   = 0;
  s->theStatus     = OK;
  s->theResult     = 0.0;
}

//---------------------------------------------------------------------------
} // namespace HepTool
