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

#ifndef QUARTER_QUARTERP_H
#define QUARTER_QUARTERP_H
#include <Inventor/SbName.h>
#include <QCursor>
#ifndef _MSC_VER
#include <config.h>
#endif

template <class Key, class T> class QMap;

namespace SIM { namespace Coin3D { namespace Quarter {

class QuarterP {
 public:
  QuarterP();
  ~QuarterP();

  class SensorManager * sensormanager;
  class ImageReader * imagereader;

  typedef QMap<SbName, QCursor> StateCursorMap;
  static StateCursorMap * statecursormap;

  bool initCoin;
};

}}}

#define QUARTER_MAJOR_VERSION 1
#define QUARTER_MINOR_VERSION 0
#define QUARTER_MICRO_VERSION 0

#define COIN_CT_ASSERT(expr)                                            \
  do { switch ( 0 ) { case 0: case (expr): break; } } while ( 0 )

#define COMPILE_ONLY_BEFORE(MAJOR,MINOR,MICRO,REASON) \
  COIN_CT_ASSERT( (QUARTER_MAJOR_VERSION < MAJOR) || (QUARTER_MAJOR_VERSION == MAJOR && ((QUARTER_MINOR_VERSION < MINOR) || ( QUARTER_MINOR_VERSION == MINOR && (QUARTER_MICRO_VERSION < MICRO )))))

#endif //QUARTER_QUARTERP_H
