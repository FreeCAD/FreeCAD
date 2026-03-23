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

#ifdef HAVE_MANIPULATORS

// This file is used to avoid duplicating sourcecode for the almost
// identical replaceManip() functions over the different manipulator
// classes.

/***************************************************************************/

#include <Inventor/manips/SoTransformManip.h>
#include <Inventor/manips/SoDirectionalLightManip.h>
#include <Inventor/manips/SoSpotLightManip.h>
#include <Inventor/manips/SoPointLightManip.h>
#include <Inventor/manips/SoClipPlaneManip.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/nodekits/SoBaseKit.h>
#include <Inventor/SoNodeKitPath.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/SoFullPath.h>

/***************************************************************************/

// FIXME: can we use a macro for the documentation as well? Check to
// see if Doxygen can handle it. 20000427 mortene.

/*!
  \fn SbBool SoTransformManip::replaceManip(SoPath * path, SoTransform * newone) const

  Replaces this manipulator from the position specified by \a path
  with \a newnode. If \a newnode is \c NULL, an SoTransform will be
  created for you.
*/

/*!
  \fn SbBool SoDirectionalLightManip::replaceManip(SoPath * path, SoDirectionalLight * newone) const

  Replaces this manipulator from the position specified by \a path
  with \a newnode. If \a newnode is \c NULL, an SoDirectionalLight
  will be created for you.
*/

/*!
  \fn SbBool SoSpotLightManip::replaceManip(SoPath * path, SoSpotLight * newone) const

  Replaces this manipulator from the position specified by \a path
  with \a newnode. If \a newnode is \c NULL, an SoSpotLight
  will be created for you.
*/

/*!
  \fn SbBool SoPointLightManip::replaceManip(SoPath * path, SoPointLight * newone) const

  Replaces this manipulator from the position specified by \a path
  with \a newnode. If \a newnode is \c NULL, an SoPointLight
  will be created for you.
*/

/*!
  \fn SbBool SoClipPlaneManip::replaceManip(SoPath * path, SoClipPlane * newone) const

  Replaces this manipulator from the position specified by \a path
  with \a newnode. If \a newnode is \c NULL, an SoClipPlane
  will be created for you.
*/

/***************************************************************************/

// FIXME: this should, as far as I can tell, be possible to write as a
// single function without doing it as a macro. 20020805 mortene.

#define SOMANIP_REPLACEMANIPBODY(_class_, _parentclass_) \
SbBool \
_class_::replaceManip(SoPath * path, _parentclass_ * newone) const \
{ \
  SoFullPath * fullpath = (SoFullPath *) path; \
  SoNode * fulltail = fullpath->getTail(); \
 \
  if (fulltail != (SoNode *)this) { \
    SoDebugError::post("_class_::replaceManip", \
                       "child to replace is not this manip (but %s at %p)", \
                       fulltail->getTypeId().getName().getString(), fulltail); \
    return FALSE; \
  } \
 \
  SbBool constructed = FALSE; \
  if (newone == NULL) { \
    newone = new _parentclass_; \
    constructed = TRUE; \
  } \
 \
  this->transferFieldValues(this, newone); \
 \
  if (path->getTail()->isOfType(SoBaseKit::getClassTypeId())) { \
    SoBaseKit * kit = (SoBaseKit *) ((SoNodeKitPath *)path)->getTail(); \
    SbString partname = kit->getPartString(path); \
    if (partname == "" || !kit->setPart(partname, newone)) { \
      SoDebugError::postWarning("_class_::replaceManip", \
                                "failed to replace manip %p with node %p" \
                                "in kit %p (partname='%s')", \
                                this, newone, kit, partname.getString()); \
      if (constructed) { \
        newone->ref(); \
        newone->unref(); \
      } \
      return FALSE; \
    } \
  } \
  else { \
    if (fullpath->getLength() < 2) { \
      SoDebugError::post("_class_::replaceManip", "path is too short"); \
      if (constructed) { \
        newone->ref(); \
        newone->unref(); \
      } \
      return FALSE; \
    } \
 \
    SoNode * parent = fullpath->getNodeFromTail(1); \
 \
    if (!parent->isOfType(SoGroup::getClassTypeId())) { \
      SoDebugError::post("_class_::replaceNode", \
                         "parent node %p is not an SoGroup, but %s", \
                         parent, parent->getTypeId().getName().getString()); \
      if (constructed) { \
        newone->ref(); \
        newone->unref(); \
      } \
      return FALSE; \
    } \
 \
    ((SoGroup*)parent)->replaceChild((SoNode*)this, newone); \
  } \
 \
  return TRUE; \
}

/***************************************************************************/

SOMANIP_REPLACEMANIPBODY(SoTransformManip, SoTransform)
SOMANIP_REPLACEMANIPBODY(SoDirectionalLightManip, SoDirectionalLight)
SOMANIP_REPLACEMANIPBODY(SoSpotLightManip, SoSpotLight)
SOMANIP_REPLACEMANIPBODY(SoPointLightManip, SoPointLight)
SOMANIP_REPLACEMANIPBODY(SoClipPlaneManip, SoClipPlane)

#undef SOMANIP_REPLACEMANIPBODY

#endif // HAVE_MANIPULATORS
