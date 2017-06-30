// -*- C++ -*-
// $Id: Evaluator.h,v 1.1.1.1 2003/07/15 20:15:05 garren Exp $
// ---------------------------------------------------------------------------

#ifndef HEP_EVALUATOR_H
#define HEP_EVALUATOR_H

namespace HepTool {

/**
 * Evaluator of arithmetic expressions with an extendable dictionary.
 * Example:
 * @code
 *   #include "CLHEP/Evaluator/Evaluator.h"
 *   HepTool::Evaluator eval;
 *   eval.setStdMath();
 *   double res = eval.evaluate("sin(30*degree)");
 *   if (eval.status() != HepTool::Evaluator::OK) eval.print_error();
 * @endcode
 *
 * @author Evgeni Chernyaev <Evgueni.Tcherniaev@cern.ch>
 * @ingroup evaluator
 */
class Evaluator {
 public: 

  /**
   * List of possible statuses.
   * Status of the last operation can be obtained with status().
   * In case if status() is an ERROR the corresponding error message
   * can be printed with print_error().
   * 
   * @see status
   * @see error_position
   * @see print_error
   */
  enum {
    OK,                         /**< Everything OK */
    WARNING_EXISTING_VARIABLE,  /**< Redefinition of existing variable */
    WARNING_EXISTING_FUNCTION,  /**< Redefinition of existing function */
    WARNING_BLANK_STRING,       /**< Empty input string */
    ERROR_NOT_A_NAME,           /**< Not allowed sysmbol in the name of variable or function */
    ERROR_SYNTAX_ERROR,         /**< Systax error */
    ERROR_UNPAIRED_PARENTHESIS, /**< Unpaired parenthesis */
    ERROR_UNEXPECTED_SYMBOL,    /**< Unexpected sysbol */
    ERROR_UNKNOWN_VARIABLE,     /**< Non-existing variable */
    ERROR_UNKNOWN_FUNCTION,     /**< Non-existing function */
    ERROR_EMPTY_PARAMETER,      /**< Function call has empty parameter */
    ERROR_CALCULATION_ERROR     /**< Error during calculation */
  };

  /**
   * Constructor.
   */
  Evaluator();

  /**
   * Destructor.
   */
  ~Evaluator(); 

  /**
   * Evaluates the arithmetic expression given as character string. 
   * The expression may consist of numbers, variables and functions
   * separated by arithmetic (+, - , /, *, ^, **) and logical
   * operators (==, !=, >, >=, <, <=, &&, ||).
   *
   * @param  expression input expression.
   * @return result of the evaluation.
   * @see status
   * @see error_position
   * @see print_error
   */
  double evaluate(const char * expression);

  /**
   * Returns status of the last operation with the evaluator.
   */
  int status() const;

  /**
   * Returns position in the input string where the problem occured.
   */
  int error_position() const; 

  /**
   * Prints error message if status() is an ERROR.
   */
  void print_error() const;

  /**
   * Adds to the dictionary a variable with given value. 
   * If a variable with such a name already exist in the dictionary,
   * then status will be set to WARNING_EXISTING_VARIABLE.
   *
   * @param name name of the variable.
   * @param value value assigned to the variable.
   */
  void setVariable(const char * name, double value);

  /**
   * Adds to the dictionary a variable with an arithmetic expression
   * assigned to it.
   * If a variable with such a name already exist in the dictionary,
   * then status will be set to WARNING_EXISTING_VARIABLE.
   *
   * @param name name of the variable.
   * @param expression arithmetic expression.
   */
  void setVariable(const char * name, const char * expression);

  /**
   * Adds to the dictionary a function without parameters.
   * If such a function already exist in the dictionary,
   * then status will be set to WARNING_EXISTING_FUNCTION.
   *
   * @param name function name.
   * @param fun pointer to the real function in the user code. 
   */
  void setFunction(const char * name, double (*fun)());

  /**
   * Adds to the dictionary a function with one parameter.
   * If such a function already exist in the dictionary,
   * then status will be set to WARNING_EXISTING_FUNCTION.
   *
   * @param name function name.
   * @param fun pointer to the real function in the user code. 
   */
  void setFunction(const char * name, double (*fun)(double));

  /**
   * Adds to the dictionary a function with two parameters.
   * If such a function already exist in the dictionary,
   * then status will be set to WARNING_EXISTING_FUNCTION.
   *
   * @param name function name.
   * @param fun pointer to the real function in the user code. 
   */
  void setFunction(const char * name, double (*fun)(double,double));

  /**
   * Adds to the dictionary a function with three parameters.
   * If such a function already exist in the dictionary,
   * then status will be set to WARNING_EXISTING_FUNCTION.
   *
   * @param name function name.
   * @param fun pointer to the real function in the user code. 
   */
  void setFunction(const char * name, double (*fun)(double,double,double));

  /**
   * Adds to the dictionary a function with four parameters.
   * If such a function already exist in the dictionary,
   * then status will be set to WARNING_EXISTING_FUNCTION.
   *
   * @param name function name.
   * @param fun pointer to the real function in the user code. 
   */
  void setFunction(const char * name,
		   double (*fun)(double,double,double,double));

  /**
   * Adds to the dictionary a function with five parameters.
   * If such a function already exist in the dictionary,
   * then status will be set to WARNING_EXISTING_FUNCTION.
   *
   * @param name function name.
   * @param fun pointer to the real function in the user code. 
   */
  void setFunction(const char * name,
                   double (*fun)(double,double,double,double,double));

  /**
   * Finds the variable in the dictionary.
   * 
   * @param  name name of the variable.
   * @return true if such a variable exists, false otherwise.
   */
  bool findVariable(const char * name) const;

  /**
   * Finds the function in the dictionary.
   * 
   * @param  name name of the function to be unset.
   * @param  npar number of parameters of the function.  
   * @return true if such a function exists, false otherwise.
   */
  bool findFunction(const char * name, int npar) const;

  /**
   * Removes the variable from the dictionary.
   * 
   * @param name name of the variable.
   */
  void removeVariable(const char * name);

  /**
   * Removes the function from the dictionary.
   * 
   * @param name name of the function to be unset.
   * @param npar number of parameters of the function.  
   */
  void removeFunction(const char * name, int npar);

  /**
   * Clear all settings.
   */
  void clear();

  /**
   * Sets standard mathematical functions and constants.
   */
  void setStdMath();

  /**
   * Sets system of units. Default is the SI system of units.
   * To set the CGS (Centimeter-Gram-Second) system of units
   * one should call:
   *   setSystemOfUnits(100., 1000., 1.0, 1.0, 1.0, 1.0, 1.0);
   *
   * To set system of units accepted in the GEANT4 simulation toolkit
   * one should call:
   * @code
   *   setSystemOfUnits(1.e+3, 1./1.60217733e-25, 1.e+9, 1./1.60217733e-10,
   *                    1.0, 1.0, 1.0);
   * @endcode
   *
   * The basic units in GEANT4 are:
   * @code
   *   millimeter              (millimeter = 1.)
   *   nanosecond              (nanosecond = 1.)
   *   Mega electron Volt      (MeV        = 1.)
   *   positron charge         (eplus      = 1.)
   *   degree Kelvin           (kelvin     = 1.)
   *   the amount of substance (mole       = 1.)
   *   luminous intensity      (candela    = 1.)
   *   radian                  (radian     = 1.)
   *   steradian               (steradian  = 1.)
   * @endcode
   */
  void setSystemOfUnits(double meter    = 1.0,
                        double kilogram = 1.0,
                        double second   = 1.0,
                        double ampere   = 1.0,
                        double kelvin   = 1.0,
                        double mole     = 1.0,
                        double candela  = 1.0);

private: 
  void * p;                                 // private data 
  Evaluator(const Evaluator &);             // copy constructor is not allowed
  Evaluator & operator=(const Evaluator &); // assignment is not allowed
};

} // namespace HepTool

#endif /* HEP_EVALUATOR_H */
