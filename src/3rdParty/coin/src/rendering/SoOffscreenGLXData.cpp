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
#include <config.h>
#endif // HAVE_CONFIG_H

#ifdef HAVE_GLX

#include "SoOffscreenGLXData.h"

#include <Inventor/errors/SoDebugError.h>

#include "tidbitsp.h"

// *************************************************************************

Display * SoOffscreenGLXData::display = NULL;
static SbBool display_initialized = FALSE;

void
SoOffscreenGLXData::cleanup(void)
{
  display_initialized = FALSE;
  // FIXME: Disabled since this might cause problems according to the
  // comment below (see getDisplay). Not sure if *not* closing the
  // display cannot also lead to problems though... 20060208 kyrah
#if 0
  XCloseDisplay(display);
  display = NULL;
#endif
}

Display *
SoOffscreenGLXData::getDisplay(void)
{
  if (!display_initialized) {
    display_initialized = TRUE;
    coin_atexit((coin_atexit_f*)SoOffscreenGLXData::cleanup, CC_ATEXIT_NORMAL);

    // Keep a single static display-ptr.
    //
    // This resource is never deallocated explicitly (but of course
    // implicitly by the system on application close-down). This to
    // work around some strange problems with the NVidia-driver 29.60
    // on XFree86 v4 when using XCloseDisplay() -- like doublebuffered
    // visuals not working correctly.
    //
    // Also, XCloseDisplay() has been known to cause crashes when
    // running remotely from some old Mesa version on Red Hat Linux
    // 6.2 onto an IRIX6.5 display. It seems likely that this was
    // caused by a bug in that particular old Mesa version.
    //
    // mortene@sim.no

    SoOffscreenGLXData::display = XOpenDisplay(NULL);

    if (!SoOffscreenGLXData::display) {
      SoDebugError::post("SoOffscreenGLXData::SoOffscreenGLXData",
                         "Couldn't connect to X11 DISPLAY.");
      // FIXME: will probably cause a crash later? If so, should be
      // more robust. 20020802 mortene.
      return NULL;
    }
  }

  return SoOffscreenGLXData::display;
}

// Pixels-pr-mm.
SbVec2f
SoOffscreenGLXData::getResolution(void)
{
  Display * d = SoOffscreenGLXData::getDisplay();
  if (!d) {
    return SbVec2f(72.0f / 25.4f, 72.0f / 25.4f); // fall back to 72dpi
  }

  int s = DefaultScreen(d);
  SbVec2f r(((float)DisplayWidth(d, s)) /  ((float)DisplayWidthMM(d, s)),
            ((float)DisplayHeight(d, s)) / ((float)DisplayHeightMM(d, s)));

  return r;
}

#endif // HAVE_GLX
