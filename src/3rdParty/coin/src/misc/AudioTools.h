#ifndef COIN_AUDIOTOOLS_H
#define COIN_AUDIOTOOLS_H

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

#ifndef COIN_INTERNAL
#error this is a private header file
#endif /* !COIN_INTERNAL */

// *************************************************************************

#include <Inventor/SbBasic.h>

// *************************************************************************

const char * coin_get_openal_error(int errcode);
int coin_debug_audio(void);

void coin_sound_enable_traverse(void);
SbBool coin_sound_should_traverse(void);

// *************************************************************************

#define SOUND_NOT_ENABLED_BY_DEFAULT_STRING \
        "The main reason for considering sound on this platform " \
        "experimental is that we have encountered various problems with " \
        "OpenAL (www.openal.org) on platforms other than Win32. If you " \
        "still want to use sound in Coin, please consider getting the " \
        "latest version of OpenAL from cvs only if you have " \
        "problems. Common problems are stuttering sound and the " \
        "occasional crash. If you run into problems, please try running " \
        "the various test-programs that comes with the OpenAL " \
        "distribution. Specifically, try running the " \
        "linux/test/teststream.c sample and verify that everything " \
        "sounds OK. "

// *************************************************************************

#endif // COIN_AUDIOTOOLS_H
