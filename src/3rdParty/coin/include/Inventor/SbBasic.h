#ifndef COIN_SBBASIC_H
#define COIN_SBBASIC_H

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

#include <Inventor/C/basic.h>
#ifndef NDEBUG
#include <Inventor/C/errors/debugerror.h>
#endif // !NDEBUG

/* ********************************************************************** */
/* Trap people trying to use Inventor headers while compiling C source code.
 * (we get support mail about this from time to time)
 */
#ifndef __cplusplus
#error You are not compiling C++ - maybe your source file is named <file>.c
#endif

/* ********************************************************************** */
/* Include these for Open Inventor compatibility reasons (they are not
 * actually used in Coin.)
 */
#define SoEXTENDER
#define SoINTERNAL

/* ********************************************************************** */

/* Some useful inline template functions:
 *   SbAbs(Val)              - returns absolute value
 *   SbMin(Val1, Val2)       - returns minimum value
 *   SbMax(Val1, Val2)       - returns maximum value
 *   SbClamp(Val, Min, Max)  - returns clamped value
 *   SbSwap(Val1, Val2)      - swaps the two values (no return value)
 *   SbSqr(val)              - returns squared value
 */

template <typename Type>
inline Type SbAbs( Type Val ) {
  return (Val < 0) ? 0 - Val : Val;
}

template <typename Type>
inline Type SbMax( const Type A, const Type B ) {
  return (A < B) ? B : A;
}

template <typename Type>
inline Type SbMin( const Type A, const Type B ) {
  return (A < B) ? A : B;
}

template <typename Type>
inline Type SbClamp( const Type Val, const Type Min, const Type Max ) {
  return (Val < Min) ? Min : (Val > Max) ? Max : Val;
}

template <typename Type>
inline void SbSwap( Type & A, Type & B ) {
  Type T; T = A; A = B; B = T;
}

template <typename Type>
inline Type SbSqr(const Type val) {
  return val * val;
}

/* *********************************************************************** */

// SbDividerChk() - checks if divide-by-zero is attempted, and emits a
// warning if so for debug builds.  inlined like this to not take much
// screenspace in inline functions.

#ifndef NDEBUG
template <typename Type>
inline void SbDividerChk(const char * funcname, Type divider) {
  if (!(divider != static_cast<Type>(0)))
    cc_debugerror_post(funcname, "divide by zero error.", divider);
}
#else
template <typename Type>
inline void SbDividerChk(const char *, Type) {}
#endif // !NDEBUG

/* ********************************************************************** */

/* COMPILER BUG WORKAROUND:

   We've had reports that Sun CC v4.0 is (likely) buggy, and doesn't
   allow a statement like

     SoType SoNode::classTypeId = SoType::badType();

   As a hack we can however get around this by instead writing it as

     SoType SoNode::classTypeId;

   ..as the SoType variable will then be initialized to bitpattern
   0x0000, which equals SoType::badType(). We can *however* not do
   this for the Intel C/C++ compiler, as that does *not* init to the
   0x0000 bitpattern (which may be a separate bug -- I haven't read
   the C++ spec closely enough to decide whether that relied on
   unspecified behavior or not).

   The latest version of the Intel compiler has been tested to still
   have this problem, so I've decided to re-install the old code, but
   in this form:

     SoType SoNode::classTypeId STATIC_SOTYPE_INIT;

   ..so it is easy to revert if somebody complains that the code
   reversal breaks their old Sun CC 4.0 compiler -- see the #define of
   STATIC_SOTYPE_INIT below.

   If that happens, we should work with the reporter, having access to
   the buggy compiler, to make a configure check which sets the
   SUN_CC_4_0_SOTYPE_INIT_BUG #define in include/Inventor/C/basic.h.in.

   (Note that the Sun CC compiler has moved on, and a later version
   we've tested, 5.4, does not have the bug.)

   20050105 mortene.
*/

#define SUN_CC_4_0_SOTYPE_INIT_BUG 0 /* assume compiler is ok for now */

#if SUN_CC_4_0_SOTYPE_INIT_BUG
#define STATIC_SOTYPE_INIT
#else
#define STATIC_SOTYPE_INIT = SoType::badType()
#endif

/* ********************************************************************** */

/*
	Coin wraps many macros in do-while statements to make them usable
	like a statement. At least the Microsoft compiler complains about
	this construct with warning C4127: "conditional expression is constant".
*/

#ifdef _MSC_VER // Microsoft Visual C++
#define WHILE_0 \
	__pragma(warning(push)) \
	__pragma(warning(disable:4127)) \
		while (0) \
	__pragma(warning(pop))
#else
#define WHILE_0 while (0)
#endif

#endif /* !COIN_SBBASIC_H */
