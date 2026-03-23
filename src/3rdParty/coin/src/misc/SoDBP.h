#ifndef COIN_SODBP_H
#define COIN_SODBP_H

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifndef COIN_INTERNAL
#error this is a private header file
#endif /* !COIN_INTERNAL */

#include <Inventor/SoDB.h>
#include <Inventor/SbString.h>

#include "misc/SbHash.h"

class SoSensor;
class SbRWMutex;

// *************************************************************************

class SoDB_HeaderInfo {
public:
  SoDB_HeaderInfo(const SbString & hs, const SbBool bin, const float ver,
                  SoDBHeaderCB * pre, SoDBHeaderCB * post, void * ud)
    : headerstring(hs), isbinary(bin), ivversion(ver),
        preload_cb(pre), postload_cb(post), userdata(ud)
    { }

  SbString headerstring;
  SbBool isbinary;
  float ivversion;
  SoDBHeaderCB * preload_cb, * postload_cb;
  void * userdata;
};

typedef SbHash<uint32_t, int16_t> UInt32ToInt16Map;

// *************************************************************************

class SoDBP {
public:
  struct EnvVars {
    static const char * COIN_PROFILER;
    static const char * COIN_PROFILER_OVERLAY;
  };

  static void variableArgsSanityCheck(void);

  static void clean(void);
  static void removeRealTimeFieldCB(void);
  static void updateRealTimeFieldCB(void * data, SoSensor * sensor);
  static void listWin32ProcessModules(void);

#ifdef COIN_THREADSAFE
  static SbRWMutex * globalmutex;
#endif // COIN_THREADSAFE
  static SbList<SoDB_HeaderInfo *> * headerlist;
  static SoSensorManager * sensormanager;
  static SoTimerSensor * globaltimersensor;
  static UInt32ToInt16Map * converters;
  static int notificationcounter;
  static SbBool isinitialized;

  static SbBool is3dsFile(SoInput * in);
  static SoSeparator * read3DSFile(SoInput * in);

  static void progress(const SbName & itemid,
                       float fraction,
                       SbBool interruptible);

  struct ProgressCallbackInfo {
    SoDB::ProgressCallbackType * func;
    void * userdata;

    int operator==(const ProgressCallbackInfo & a) {
      return (a.func == this->func) && (a.userdata == this->userdata);
    }
  };
  static SbList<struct ProgressCallbackInfo> * progresscblist;
};

#endif // !COIN_SODBP_H
