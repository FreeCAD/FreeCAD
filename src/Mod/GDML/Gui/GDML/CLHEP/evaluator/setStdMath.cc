// -*- C++ -*-
// $Id: setStdMath.cc,v 1.2 2003/08/13 20:00:10 garren Exp $
// ----------------------------------------------------------------------

///#include "CLHEP/Evaluator/defs.h"
///#include "CLHEP/Evaluator/Evaluator.h"
#include "defs.h"
#include "Evaluator.h"

#include <cmath>	// for sqrt and pow

using namespace std;

static double eval_abs  (double a)           { return (a < 0) ? -a : a; } 
static double eval_min  (double a, double b) { return (a < b) ?  a : b; } 
static double eval_max  (double a, double b) { return (a > b) ?  a : b; } 
static double eval_sqrt (double a)           { return sqrt(a); } 
static double eval_pow  (double a, double b) { return pow(a,b); } 
static double eval_sin  (double a)           { return sin(a); } 
static double eval_cos  (double a)           { return cos(a); } 
static double eval_tan  (double a)           { return tan(a); } 
static double eval_asin (double a)           { return asin(a); } 
static double eval_acos (double a)           { return acos(a); } 
static double eval_atan (double a)           { return atan(a); } 
static double eval_atan2(double a, double b) { return atan2(a,b); } 
static double eval_sinh (double a)           { return sinh(a); } 
static double eval_cosh (double a)           { return cosh(a); } 
static double eval_tanh (double a)           { return tanh(a); } 
static double eval_exp  (double a)           { return exp(a); } 
static double eval_log  (double a)           { return log(a); } 
static double eval_log10(double a)           { return log10(a); } 

namespace HepTool {

void Evaluator::setStdMath() {

  //   S E T   S T A N D A R D   C O N S T A N T S

  setVariable("pi",     3.14159265358979323846);
  setVariable("e",      2.7182818284590452354);
  setVariable("gamma",  0.577215664901532861);
  setVariable("radian", 1.0);
  setVariable("rad",    1.0);
  setVariable("degree", 3.14159265358979323846/180.);
  setVariable("deg",    3.14159265358979323846/180.);

  //   S E T   S T A N D A R D   F U N C T I O N S

  setFunction("abs",   eval_abs);
  setFunction("min",   eval_min);
  setFunction("max",   eval_max);
  setFunction("sqrt",  eval_sqrt);
  setFunction("pow",   eval_pow);
  setFunction("sin",   eval_sin);
  setFunction("cos",   eval_cos);
  setFunction("tan",   eval_tan);
  setFunction("asin",  eval_asin);
  setFunction("acos",  eval_acos);
  setFunction("atan",  eval_atan);
  setFunction("atan2", eval_atan2);
  setFunction("sinh",  eval_sinh);
  setFunction("cosh",  eval_cosh);
  setFunction("tanh",  eval_tanh);
  setFunction("exp",   eval_exp);
  setFunction("log",   eval_log);
  setFunction("log10", eval_log10);
}

} // namespace HepTool
