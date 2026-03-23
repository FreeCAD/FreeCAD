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

#include "scxml/SbStringConvert.h"

#include <cstdio>
#include <cstring>

#include <Inventor/SbString.h>
#include <Inventor/SbVec2s.h>
#include <Inventor/SbVec2f.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbVec3d.h>
#include <Inventor/SbRotation.h>

using std::strcmp;
using std::strncmp;

SbStringConvert::TypeIdentity
SbStringConvert::typeOf(const SbString & str)
{
  if (strncmp(str.getString(), "Sb", 2) == 0) {
    if (strncmp(str.getString() + 2, "Vec2s(", 6) == 0) {
      return SBVEC2S;
    }
    else if (strncmp(str.getString() + 2, "Vec2f(", 6) == 0) {
      return SBVEC2F;
    }
    else if (strncmp(str.getString() + 2, "Vec3f(", 6) == 0) {
      return SBVEC3F;
    }
    else if (strncmp(str.getString() + 2, "Vec3d(", 6) == 0) {
      return SBVEC3D;
    }
    else if (strncmp(str.getString() + 2, "Rotation(", 9) == 0) {
      return SBROTATION;
    }
  }
  else {
    if (str[0] >= '0' && str[0] <= '9') {
      return NUMERIC;
    }
    if (str[0] == '-' && str[1] >= '0' && str[1] <= '9') {
      return NUMERIC;
    }
    if (strcmp(str.getString(), "TRUE") == 0 ||
        strcmp(str.getString(), "FALSE") == 0) {
      return BOOLEAN;
    }
  }
  return UNKNOWN;
}

#ifdef COIN_TEST_SUITE
#ifdef COIN_INT_TEST_SUITE
#include <Inventor/SbVec3s.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbRotation.h>

BOOST_AUTO_TEST_CASE(SbVec3sfromString) {
  typedef SbVec3s ToTest;
  ToTest foo;
  SbString test = "SbVec3s(1,-2,3)";
  ToTest trueVal(1,-2,3);
  foo = SbStringConvert::fromString<ToTest>(test);
  BOOST_CHECK_MESSAGE(trueVal == foo,
                      std::string("Mismatch between ") +  foo.toString().getString() + " and control " + trueVal.toString().getString());
}

BOOST_AUTO_TEST_CASE(SbVec3sToString) {
  typedef SbVec3s ToTest;
  SbString foo;
  ToTest test(1,-2,3);
  SbString trueVal = "SbVec3s(1, -2, 3)";
  foo = SbStringConvert::toString(test);
  BOOST_CHECK_MESSAGE(trueVal == foo,
                      std::string("Mismatch between ") +  foo.getString() + " and control " + trueVal.getString());
}

BOOST_AUTO_TEST_CASE(SbVec3sfromSbVec3f) {
  typedef SbVec3s ToTest;
  ToTest foo;
  SbString test = "SbVec3f(1,-2,3)";
  ToTest trueVal(1,-2,3);
  SbBool conversionOk = TRUE;
  foo = SbStringConvert::fromString<ToTest>(test, &conversionOk);
  BOOST_CHECK_MESSAGE(conversionOk == FALSE,
                      std::string("Able to create SbVec3s from ") +  test.getString());
}

BOOST_AUTO_TEST_CASE(SbVec3fToString) {
  typedef SbVec3f ToTest;
  SbString foo;
  ToTest test(0.5,-0.25,0.125);
  SbString trueVal = "SbVec3f(0.5, -0.25, 0.125)";
  foo = SbStringConvert::toString(test);
  BOOST_CHECK_MESSAGE(trueVal == foo,
                      std::string("Mismatch between ") +  foo.getString() + " and control " + trueVal.getString());
}

BOOST_AUTO_TEST_CASE(SbVec3fToIntString) {
  typedef SbVec3f ToTest;
  SbString foo;
  ToTest test(1,-2,3);
  SbString trueVal = "SbVec3f(1, -2, 3)";
  foo = SbStringConvert::toString(test);
  BOOST_CHECK_MESSAGE(trueVal == foo,
                      std::string("Mismatch between ") +  foo.getString() + " and control " + trueVal.getString());
}

BOOST_AUTO_TEST_CASE(SbVec3fFromIntString) {
  typedef SbVec3f ToTest;
  ToTest foo;
  SbString test = "SbVec3f(1,-2,3)";
  ToTest trueVal(1,-2,3);
  SbBool conversionOk = TRUE;
  foo = SbStringConvert::fromString<ToTest>(test, &conversionOk);
  BOOST_CHECK_MESSAGE(conversionOk && trueVal == foo,
                      std::string("Mismatch between ") +  foo.toString().getString() + " and control " + trueVal.toString().getString());
}

BOOST_AUTO_TEST_CASE(SbVec3fFromFloatString) {
  typedef SbVec3f ToTest;
  ToTest foo;
  SbString test = "SbVec3f( 1.3  , -2.0  ,   3.3  )";
  ToTest trueVal(1.3,-2.0,3.3);
  SbBool conversionOk = TRUE;
  foo = SbStringConvert::fromString<ToTest>(test, &conversionOk);
  BOOST_CHECK_MESSAGE(conversionOk && trueVal == foo,
                      std::string("Mismatch between ") +  foo.toString().getString() + " and control " + trueVal.toString().getString());
}

BOOST_AUTO_TEST_CASE(SbRotationFromString) {
  typedef SbRotation ToTest;
  ToTest foo;
  SbString test = "SbRotation(0.5,0.5 , 0.5, 0.5 )";
  ToTest trueVal(0.5,0.5,0.5,0.5);
  SbBool conversionOk = FALSE;
  foo = SbStringConvert::fromString<ToTest>(test, &conversionOk);
  BOOST_CHECK_MESSAGE(conversionOk && trueVal == foo,
                      std::string("Mismatch between ") +  foo.toString().getString() + " and control " + trueVal.toString().getString());
}

BOOST_AUTO_TEST_CASE(SbRotationToString) {
  typedef SbRotation ToTest;
  SbString foo;
  ToTest test(0.5,-0.5,-0.5,0.5);
  SbString trueVal = "SbRotation(0.5, -0.5, -0.5, 0.5)";
  foo = SbStringConvert::toString(test);
  BOOST_CHECK_MESSAGE(trueVal == foo,
                      std::string("Mismatch between ") +  foo.getString() + " and control " + trueVal.getString());
}

BOOST_AUTO_TEST_CASE(SbRotationToIntString) {
  typedef SbRotation ToTest;
  SbString foo;
  ToTest test(0,0,0,1);
  SbString trueVal = "SbRotation(0, 0, 0, 1)";
  foo = SbStringConvert::toString(test);
  BOOST_CHECK_MESSAGE(trueVal == foo,
                      std::string("Mismatch between ") +  foo.getString() + " and control " + trueVal.getString());
}

#endif //COIN_INT_TESTSUITE
#endif //COIN_TEST_SUITE
