#ifndef COIN_SO_H
#define COIN_SO_H

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

#include <Inventor/SoDB.h>
#include <Inventor/SoInput.h>
#include <Inventor/SoOffscreenRenderer.h>
#include <Inventor/SoOutput.h>
#include <Inventor/SoPath.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/SoPrimitiveVertex.h>

#include <Inventor/misc/SoState.h>

#include <Inventor/SoLists.h>
#include <Inventor/actions/SoActions.h>
#include <Inventor/details/SoDetails.h>
#include <Inventor/elements/SoElements.h>
#include <Inventor/engines/SoEngines.h>
#include <Inventor/errors/SoErrors.h>
#include <Inventor/events/SoEvents.h>
#include <Inventor/fields/SoFields.h>
#include <Inventor/nodes/SoNodes.h>
#include <Inventor/sensors/SoSensorManager.h>
#include <Inventor/sensors/SoSensors.h>

// We choose to not include classes that are "mostly internal":
// bundles, caches, elements, and projectors.
//
// Base classes are not included. They should for the most part be
// included by the other classes using them.
//
// Classes for interaction is not included either, as they can be seen
// as a later add-on to the core library, and it's not unlikely that
// we'll eventually make their inclusion a configure/build
// option. That goes for nodekits, draggers, and manips.
//
// The library can be built without VRML97 nodes, and without the
// thread abstraction classes, so they are also not part of this
// header file.

#endif // !COIN_SO_H
