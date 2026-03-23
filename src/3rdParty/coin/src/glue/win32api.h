#ifndef COIN_WIN32API_H
#define COIN_WIN32API_H

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

/* This is an internal set of convenience wrappers around Win32 API
   functions that are unnecessary robust for our purpose.

   To simplify our client code within the library, we therefore catch
   error conditions, report as exact information about the error as
   possible, and then assert on them within the wrappers. The client
   code can therefore ignore possible problems.
*/

/*************************************************************************/

#ifndef COIN_INTERNAL
#error this is a private header file
#endif /* COIN_INTERNAL */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifndef HAVE_WIN32_API
/* Just in case we can get in trouble for declaring functions that are
   not implemented. */
#error Do not include this file unless the contents will actually be used.
#endif /* HAVE_WIN32_API */

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif // HAVE_WINDOWS_H

/*************************************************************************/

#include <Inventor/C/basic.h>

/*************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

  typedef int (WINAPI * GetTextFace_t)(HDC, int, LPTSTR);
  typedef void (WINAPI * LocalFree_t)(HLOCAL);
  typedef void (WINAPI * GetVersionEx_t)(LPOSVERSIONINFO);
  typedef HGDIOBJ (WINAPI * SelectObject_t)(HDC, HGDIOBJ);
  typedef int (WINAPI * GetObject_t)(HGDIOBJ, int, LPVOID);
 
  struct cc_win32_api {
    GetTextFace_t GetTextFace;
    LocalFree_t LocalFree;
    GetVersionEx_t GetVersionEx;
    SelectObject_t SelectObject;
    GetObject_t GetObject;
  };

  /* Force singleton access. */
  const struct cc_win32_api * cc_win32(void);

  /* Convenient wrapping of converting and printing out a Win32 API
     error code as a text string. The "lasterror" argument should be
     the result from GetLastError(). */
  void cc_win32_print_error(const char * callerfuncname,
                            const char * apifuncname,
                            DWORD lasterror);
  
#ifdef __cplusplus
}
#endif

#endif /* !COIN_WIN32API_H */
