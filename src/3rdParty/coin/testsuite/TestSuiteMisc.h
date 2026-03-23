#ifndef COIN_TESTSUITE_MISC
#define COIN_TESTSUITE_MISC

/**************************************************************************\
* Copyright (c) Kongsberg Oil & Gas Technologies AS
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*
* Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
*
* Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
*
* Neither the name of the copyright holder nor the names of its
* contributors may be used to endorse or promote products derived from
* this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
* HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\**************************************************************************/

#include <string>
#include <ostream>
#include <Inventor/SbBasic.h>
#include <Inventor/SbTypeInfo.h>
#include "CoinTest.h"

#ifndef TEST_SUITE_THOROUGHNESS
/*
  TEST_SUITE_THOROUGHNESS levels are:
  1 Just quick tests
  2 More thorough
  3 Expansive
*/
#define TEST_SUITE_THOROUGHNESS 1
#endif //TEST_SUITE_THOROUGHNESS

#if TEST_SUITE_THOROUGHNESS == 1
#define TEST_SUITE_QUICK
#elif TEST_SUITE_THOROUGHNESS == 2
#define TEST_SUITE_THOROUG
#elif TEST_SUITE_THOROUGHNESS == 3
#define TEST_SUITE_EXPANSIVE
#endif //TEST_SUITE_THOROUGHNESS

// Test for almostEquality
/*
  Note the difference between last parameter as float or int Last
  parameter as float, means relative tolerance, last parameter as int,
  means maximal number of orders difference in an ordered set of all
  floats
*/
inline bool floatEquals(float Ain, float Bin, unsigned int maxUlps)
{
    // Make sure maxUlps is non-negative and small enough that the
    // default NAN won't compare as equal to anything.
    assert(maxUlps > 0 && maxUlps < 4 * 1024 * 1024);
    union {
      float f32;
      uint32_t i32;
    } A,B;
    A.f32 = Ain;
    B.f32 = Bin;
    // Make A.i32 lexicographically ordered as a twos-complement int
    if (A.i32 < 0)
        A.i32 = 0x80000000 - A.i32;
    // Make B.i32 lexicographically ordered as a twos-complement int
    if (B.i32 < 0)
        B.i32 = 0x80000000 - B.i32;
    unsigned int intDiff = SbAbs(A.i32 - B.i32);
    if (intDiff <= maxUlps)
        return true;
    return false;
}

inline bool floatEquals(float Ain, float Bin, int maxUlps)
{
  assert(maxUlps>0);
  return floatEquals(Ain, Bin, static_cast<unsigned int>(maxUlps));
}

inline
bool
floatEquals(float a, float b, float tol)
{
  float tol2 = tol*tol;
  if (fabs(a)<tol2&&fabs(b)<tol2)
    return true;
  return fabs(b-a)/fabs(a)<tol;
}

#define COIN_TESTCASE_CHECK_FLOAT(X,Y) BOOST_CHECK_MESSAGE(floatEquals((X), (Y), 1), std::string("unexpected value: expected ") + ::CoinTest::stringify((Y)) +", got " + ::CoinTest::stringify((X)) + " difference is: " + ::CoinTest::stringify((X)-(Y)))

namespace SIM { namespace Coin { namespace TestSuite {

namespace internal {
template <unsigned int Dimensions>
struct fCompare {
  template <typename T, typename S, typename U>
  static bool cmp(const T & v1, const S & v2, U tolerance = 64) {
    for (int i=0;i<SbTypeInfo<T>::Dimensions;++i) {
      if (!floatEquals(static_cast<float>(v1[i]),static_cast<float>(v2[i]),tolerance))
        return false;
    }
    return true;
  }
};

template <>
struct fCompare<1> {
  template <typename T, typename S, typename U>
  static bool cmp(const T & v1, const S & v2, U tolerance = 64) {
    return floatEquals(static_cast<float>(v1),static_cast<float>(v2),tolerance);
  }
};

template <typename T, typename S, typename U>
bool
fuzzyCompare(const T & v1, const S & v2, U tolerance = 64) 
{
  static_assert(static_cast<int>(SbTypeInfo<T>::Dimensions) == static_cast<int>(SbTypeInfo<S>::Dimensions),
                "SbTypeInfo dimension mismatch");
  return fCompare<SbTypeInfo<T>::Dimensions>::cmp(v1,v2,tolerance);
}

template <unsigned int Dimensions>
struct to {
  template<typename T>
  static std::string 
  String(const T & v) 
  {
    return v.toString().getString();
  }
};

template <>
struct to<1> {
  template<typename T>
  static std::string 
  String(const T & v) 
  {
    return ::CoinTest::stringify(v);
  }
};
} //namespace internal

 template<typename T, typename S, typename U> 
   bool
   inline 
   check_compare(const T & v1, const S & v2, const std::string & txt, U tolerance = 64)
   {
     using namespace internal;
     bool cmp=fuzzyCompare(v1,v2, tolerance);
     BOOST_CHECK_MESSAGE(cmp, txt+": "+to<SbTypeInfo<T>::Dimensions>::String(v1) + " != "+ to<SbTypeInfo<T>::Dimensions>::String(v2) );
     return cmp;
   }
}}}


/*
 * The following ostream << operators are needed for the testsuite macros
 * for when they report on failures with our custom datatypes.
 * Expand as needed.
 *
 * Note that the custom classes are predeclared in the prototype to keep
 * this header file tidy as it grows...
 */

std::ostream & operator << (std::ostream & os, const class SbVec4ub & vec);
std::ostream & operator << (std::ostream & os, const class SbVec4us & vec);
std::ostream & operator << (std::ostream & os, const class SbVec4ui32 & vec);

std::ostream & operator << (std::ostream & os, const class SbColor4f & col4);

#endif // !COIN_TESTSUITE_MISC
