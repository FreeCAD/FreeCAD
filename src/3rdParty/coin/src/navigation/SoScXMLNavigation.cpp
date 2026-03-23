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

#include <Inventor/navigation/SoScXMLNavigation.h>

/*!
  \class SoScXMLNavigation SoScXMLNavigation.h Inventor/scxml/SoScXMLNavigation.h
  \brief Static class for some static init/cleanup/synchronization functions.

  \ingroup coin_navigation
*/

#include <cassert>
#include "tidbitsp.h"

#include <Inventor/misc/CoinResources.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/navigation/SoScXMLRotateTarget.h>
#include <Inventor/navigation/SoScXMLPanTarget.h>
#include <Inventor/navigation/SoScXMLZoomTarget.h>
#include <Inventor/navigation/SoScXMLDollyTarget.h>
#include <Inventor/navigation/SoScXMLSpinTarget.h>
#include <Inventor/navigation/SoScXMLSeekTarget.h>
#include <Inventor/navigation/SoScXMLMiscTarget.h>
#include <Inventor/navigation/SoScXMLFlightControlTarget.h>
#include <Inventor/navigation/SoScXMLMotionTarget.h>
#include <Inventor/C/threads/mutex.h>
#include "threads/threadsutilp.h"
#include "navigation/common-xml.cpp"
#include "navigation/examiner-xml.cpp"
#include "navigation/plane-xml.cpp"

class SoScXMLNavigation::PImpl {
public:
  static void * syncmutex;
};

void * SoScXMLNavigation::PImpl::syncmutex = NULL;

void
SoScXMLNavigation::initClasses(void)
{
  assert(PImpl::syncmutex == NULL);
  CC_MUTEX_CONSTRUCT(PImpl::syncmutex);

  SoScXMLNavigationTarget::initClass();
  SoScXMLRotateTarget::initClass();
  SoScXMLSpinTarget::initClass();
  SoScXMLPanTarget::initClass();
  SoScXMLZoomTarget::initClass();
  SoScXMLDollyTarget::initClass();
  SoScXMLSeekTarget::initClass();
  SoScXMLMiscTarget::initClass();
  SoScXMLFlightControlTarget::initClass();
  SoScXMLMotionTarget::initClass();

  CoinResources::set("coin:scxml/navigation/common.xml",
                     SbByteBuffer(sizeof(common_xml)-1,&common_xml[0]));

  CoinResources::set("coin:scxml/navigation/examiner.xml",
                     SbByteBuffer(sizeof(examiner_xml)-1,&examiner_xml[0]));

  CoinResources::set("coin:scxml/navigation/plane.xml",
                     SbByteBuffer(sizeof(plane_xml)-1,&plane_xml[0]));

  // launch services
  SoScXMLRotateTarget::constructSingleton();
  SoScXMLPanTarget::constructSingleton();
  SoScXMLZoomTarget::constructSingleton();
  SoScXMLDollyTarget::constructSingleton();
  SoScXMLSpinTarget::constructSingleton();
  SoScXMLSeekTarget::constructSingleton();
  SoScXMLMiscTarget::constructSingleton();
  SoScXMLFlightControlTarget::constructSingleton();
  SoScXMLMotionTarget::constructSingleton();

  coin_atexit(reinterpret_cast<coin_atexit_f*>(SoScXMLNavigation::cleanClasses), CC_ATEXIT_NORMAL);
}

void
SoScXMLNavigation::cleanClasses(void)
{
  SoScXMLMotionTarget::destructSingleton();
  SoScXMLFlightControlTarget::destructSingleton();
  SoScXMLMiscTarget::destructSingleton();
  SoScXMLSeekTarget::destructSingleton();
  SoScXMLSpinTarget::destructSingleton();
  SoScXMLDollyTarget::destructSingleton();
  SoScXMLZoomTarget::destructSingleton();
  SoScXMLPanTarget::destructSingleton();
  SoScXMLRotateTarget::destructSingleton();

  SoScXMLRotateTarget::cleanClass();
  SoScXMLSpinTarget::cleanClass();
  SoScXMLPanTarget::cleanClass();
  SoScXMLDollyTarget::cleanClass();
  SoScXMLZoomTarget::cleanClass();
  SoScXMLSeekTarget::cleanClass();
  SoScXMLMiscTarget::cleanClass();
  SoScXMLFlightControlTarget::cleanClass();
  SoScXMLMotionTarget::cleanClass();
  SoScXMLNavigationTarget::cleanClass();

  CC_MUTEX_DESTRUCT(PImpl::syncmutex);
  PImpl::syncmutex = NULL;
}

void
SoScXMLNavigation::syncLock(void)
{
  CC_MUTEX_LOCK(PImpl::syncmutex);
}

void
SoScXMLNavigation::syncUnlock(void)
{
  CC_MUTEX_UNLOCK(PImpl::syncmutex);
}
