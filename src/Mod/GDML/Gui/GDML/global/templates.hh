//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
//
// $Id: templates.hh,v 1.13 2008/08/15 12:15:53 gcosmo Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// 
// -*- C++ -*-
//
// -----------------------------------------------------------------------
// This file should define some platform dependent features and some
// useful utilities.
// -----------------------------------------------------------------------

// =======================================================================
// Gabriele Cosmo - Created: 5th September 1995
// Gabriele Cosmo - Minor change: 08/02/1996
// Gabriele Cosmo - Added DBL_MIN, FLT_MIN, DBL_DIG,
//                        DBL_MAX, FLT_DIG, FLT_MAX  : 12/04/1996
// Gabriele Cosmo - Removed boolean enum definition : 29/11/1996
// Gunter Folger  - Added G4SwapPtr() and G4SwapObj() : 31/07/1997
// Gabriele Cosmo - Adapted signatures of min(), max() to
//                  STL's ones, thanks to E.Tcherniaev : 31/07/1997
// Gabriele Cosmo,
// Evgueni Tcherniaev - Migrated to CLHEP: 04/12/1997 
// =======================================================================

#ifndef templates_h
#define templates_h 1

#include <limits>
#include <climits>

//
// If HIGH_PRECISION is defined to TRUE (ie. != 0) then the type "Float"
// is typedefed to "double". If it is FALSE (ie. 0) it is typedefed
// to "float".
//
#ifndef HIGH_PRECISION
#define HIGH_PRECISION 1
#endif

#if HIGH_PRECISION
typedef double Float;
#else
typedef float Float;
#endif

// Following values have been taken from limits.h
// and temporarly defined for portability on HP-UX.

#ifndef DBL_MIN   /* Min decimal value of a double */
#define DBL_MIN   std::numeric_limits<double>::min()  // 2.2250738585072014e-308
#endif

#ifndef DBL_DIG   /* Digits of precision of a double */
#define DBL_DIG   std::numeric_limits<double>::digits10   // 15
#endif

#ifndef DBL_MAX   /* Max decimal value of a double */
#define DBL_MAX   std::numeric_limits<double>::max()  // 1.7976931348623157e+308
#endif

#ifndef DBL_EPSILON
#define DBL_EPSILON std::numeric_limits<double>::epsilon()
#endif                                                // 2.2204460492503131e-16

#ifndef FLT_MIN   /* Min decimal value of a float */
#define FLT_MIN   std::numeric_limits<float>::min()   // 1.17549435e-38F
#endif

#ifndef FLT_DIG   /* Digits of precision of a float */
#define FLT_DIG   std::numeric_limits<float>::digits10     // 6
#endif

#ifndef FLT_MAX   /* Max decimal value of a float */
#define FLT_MAX   std::numeric_limits<float>::max()   // 3.40282347e+38F
#endif

#ifndef FLT_EPSILON
#define FLT_EPSILON std::numeric_limits<float>::epsilon()
#endif                                                // 1.192092896e-07F

#ifndef MAXFLOAT   /* Max decimal value of a float */
#define MAXFLOAT  std::numeric_limits<float>::max()   // 3.40282347e+38F
#endif

//---------------------------------

template <class T>
inline void G4SwapPtr(T*& a, T*& b) {
  T* tmp= a;
  a = b;
  b = tmp;
}

template <class T>
inline void G4SwapObj(T* a, T* b) {
  T tmp= *a;
  *a = *b;
  *b = tmp;
}

//-----------------------------

#ifndef G4_SQR_DEFINED
  #define G4_SQR_DEFINED
  #ifdef sqr
    #undef sqr
  #endif

template <class T>
inline T sqr(const T& x)
{
  return x*x;
}
#endif

#ifdef G4_ABS_DEFINED
  #ifdef abs
    #undef abs
  #endif
template <class T>
inline T std::abs(const T& a)
{
  return a < 0 ? -a : a;
}
#endif

#endif // templates_h
