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

/*!
  \class SoInteraction SoInteraction.h Inventor/SoInteraction.h
  \brief The SoInteraction class takes care of initializing internal classes.

  \ingroup coin_general

  SoInteraction is present for the sole purpose of providing an
  interface to the initialization methods of the classes in Coin which
  are somehow related to user interaction, like the draggers and
  manipulators.

  It is unlikely that the application programmer should need to worry
  about this class, as SoInteraction::init() is called by the GUI
  specific initialization methods.

 */

#include <Inventor/SoInteraction.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <Inventor/SoDB.h>
#include <Inventor/nodes/SoAntiSquish.h>
#include <Inventor/nodes/SoExtSelection.h>
#include <Inventor/nodes/SoSurroundScale.h>

#include <Inventor/nodekits/SoNodeKit.h>
#ifdef HAVE_NODEKITS
#include <Inventor/nodekits/SoInteractionKit.h>
#endif // HAVE_NODEKITS

#ifdef HAVE_DRAGGERS
#include <Inventor/draggers/SoDragger.h>
#endif // HAVE_DRAGGERS

#ifdef HAVE_MANIPULATORS
#include <Inventor/manips/SoCenterballManip.h>
#include <Inventor/manips/SoClipPlaneManip.h>
#include <Inventor/manips/SoDirectionalLightManip.h>
#include <Inventor/manips/SoHandleBoxManip.h>
#include <Inventor/manips/SoJackManip.h>
#include <Inventor/manips/SoPointLightManip.h>
#include <Inventor/manips/SoSpotLightManip.h>
#include <Inventor/manips/SoTabBoxManip.h>
#include <Inventor/manips/SoTrackballManip.h>
#include <Inventor/manips/SoTransformBoxManip.h>
#include <Inventor/manips/SoTransformerManip.h>
#endif // HAVE_MANIPULATORS

#include "tidbitsp.h"

static SbBool interaction_isinitialized = FALSE;

static void interaction_cleanup(void)
{
  interaction_isinitialized = FALSE;
}

/*!
  Calls the initClass() method of these classes: SoAntiSquish,
  SoSelection, SoExtSelection, SoSurroundScale, SoInteractionKit,
  SoDragger, SoClipPlaneManip, SoDirectionalLightManip,
  SoPointLightManip, SoSpotLightManip, SoTransformManip,
  SoCenterballManip, SoHandleBoxManip, SoJackManip, SoTabBoxManip,
  SoTrackballManip, SoTransformBoxManip, SoTransformerManip.

  Note that this method calls SoDB::init() and SoNodeKit::init() to
  make sure all classes that the interaction functionality depends on
  have been initialized.


  Application programmers should usually not have to invoke this method
  directly from application code, as it is indirectly called from the
  GUI-binding libraries' init()-functions.  Only if you are using your own
  GUI-binding (and not one of Kongsberg Oil & Gas Technologies SoQt, SoGtk,
  SoXt, SoWin, Sc21 etc. libraries) do you have to explicitly call
  SoInteraction::init().

  \code
  int main(int argc, char ** argv )
  {
    // SoQt::init() calls SoDB::init(), SoNodeKit::init() and
    // SoInteraction::init().
    QWidget * window = SoQt::init( argv[0] );

    SoSeparator * root = make_scenegraph();
    root->ref();
    /// [... etc ...] ///
  \endcode

 */
void
SoInteraction::init(void)
{
  if (interaction_isinitialized) return;

  if (!SoDB::isInitialized()) SoDB::init();

  SoAntiSquish::initClass();
  SoSelection::initClass();
  SoExtSelection::initClass();
  SoSurroundScale::initClass();

  SoNodeKit::init();
#ifdef HAVE_NODEKITS
  SoInteractionKit::initClass();
#endif // HAVE_NODEKITS

#ifdef HAVE_DRAGGERS
  SoDragger::initClass();
#endif // HAVE_DRAGGERS

#ifdef HAVE_MANIPULATORS
  SoClipPlaneManip::initClass();
  SoDirectionalLightManip::initClass();
  SoPointLightManip::initClass();
  SoSpotLightManip::initClass();
  SoTransformManip::initClass();
  SoCenterballManip::initClass();
  SoHandleBoxManip::initClass();
  SoJackManip::initClass();
  SoTabBoxManip::initClass();
  SoTrackballManip::initClass();
  SoTransformBoxManip::initClass();
  SoTransformerManip::initClass();
#endif // HAVE_MANIPULATORS

  interaction_isinitialized = TRUE;
  coin_atexit((coin_atexit_f*)interaction_cleanup, CC_ATEXIT_NORMAL);
}
