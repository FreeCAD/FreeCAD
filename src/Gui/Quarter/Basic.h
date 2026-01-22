#pragma once

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

/*
  See Coin's Inventor/C/basic.h for an explanation to the below #define
  setup.
*/

#ifdef QUARTER_DLL_API
#error Leave the QUARTER_DLL_API define alone
#endif // QUARTER_MAKE_DLL

#ifdef QUARTER_INTERNAL
# ifdef QUARTER_NOT_DLL
#  error The QUARTER_DLL define should not be used when building the library - only when building win32 applications.
# endif // QUARTER_NOT_DLL
# ifdef QUARTER_DLL
#  error The QUARTER_NOT_DLL define should not be used when building the library - only when building win32 applications.
# endif // QUARTER_DLL
#endif // QUARTER_INTERNAL

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
# ifdef QUARTER_INTERNAL
#  ifdef QUARTER_MAKE_DLL
#   define QUARTER_DLL_API __declspec(dllexport)
#  endif // QUARTER_MAKE_DLL
# else // !QUARTER_INTERNAL
#  ifdef QUARTER_DLL
#   ifdef QUARTER_NOT_DLL
#    error Define _either_ QUARTER_DLL or QUARTER_NOT_DLL as appropriate for your linkage -- not both at the same time! See the Coin Inventor/C/basic.h for further instructions.
#   endif // QUARTER_NOT_DLL
#   define QUARTER_DLL_API __declspec(dllimport)
#  else // !QUARTER_DLL
#   ifndef QUARTER_NOT_DLL
#    error Define either QUARTER_DLL or QUARTER_NOT_DLL as appropriate for your linkage. See the Coin Inventor/C/basic.h for further instructions.
#   endif // !QUARTER_NOT_DLL
#  endif // !QUARTER_DLL
# endif // !QUARTER_INTERNAL
#endif // Microsoft Windows

#ifndef QUARTER_DLL_API
# define QUARTER_DLL_API
#endif // !QUARTER_DLL_API
