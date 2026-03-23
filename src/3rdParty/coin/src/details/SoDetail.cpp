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
  \class SoDetail SoDetail.h Inventor/details/SoDetail.h
  \brief The SoDetail class is the superclass for all classes storing
  detailed information about particular shapes.

  \ingroup coin_details

  Detail information about shapes is used in relation to picking
  actions in Coin.  They typically contain the relevant information
  about what particular part of the shape a pick ray intersected with.

*/

// *************************************************************************

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <Inventor/SbName.h>
#include <Inventor/details/SoConeDetail.h>
#include <Inventor/details/SoCubeDetail.h>
#include <Inventor/details/SoCylinderDetail.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/details/SoLineDetail.h>
#include <Inventor/details/SoPointDetail.h>
#include <Inventor/details/SoTextDetail.h>
#ifdef HAVE_NODEKITS
#include <Inventor/details/SoNodeKitDetail.h>
#endif // HAVE_NODEKITS

// *************************************************************************

/*!
  \fn SoDetail * SoDetail::copy(void) const
  Return a deep copy of ourself.

  \DANGEROUS_ALLOC_RETURN
*/

// Note: the following documentation for getTypeId() will also be
// visible for subclasses, so keep it general.  If you write any
// additional documentation for this method, check the other top-level
// classes with getTypeId() documentation to see if it is applicable
// to update those also (it probably is).
/*!
  \fn SoType SoDetail::getTypeId(void) const

  Returns the type identification of a detail derived from a class
  inheriting SoDetail.  This is used for runtime type checking and
  "downward" casting.

  Usage example:

  \code
  void fuhbear(SoDetail * detail)
  {
    if (detail->getTypeId() == SoFaceDetail::getClassTypeId()) {
      // safe downward cast, know the type
      SoFaceDetail * facedetail = (SoFaceDetail *)detail;
      /// [then something] ///
    }
    return; // ignore if not a SoFaceDetail
  }
  \endcode


  For application programmers wanting to extend the library with new
  detail classes: this method needs to be overridden in \e all
  subclasses. This is typically done as part of setting up the full
  type system for extension classes, which is usually accomplished by
  using the predefined macros available through
  Inventor/nodes/SoSubDetail.h: SO_DETAIL_SOURCE and
  SO_DETAIL_INIT_CLASS.
*/

// *************************************************************************

SoType SoDetail::classTypeId STATIC_SOTYPE_INIT;

// *************************************************************************

/*!
  Default constructor.
*/
SoDetail::SoDetail(void)
{
}

/*!
  Destructor.
*/
SoDetail::~SoDetail()
{
}

/*!
  Returns \c TRUE if \a type is derived from (or \e is) this class.
*/
SbBool
SoDetail::isOfType(const SoType type) const
{
  return this->getTypeId().isDerivedFrom(type);
}

/*!
  Returns the type identification for this class.
*/
SoType
SoDetail::getClassTypeId(void)
{
  return SoDetail::classTypeId;
}

// Note: the following documentation for initClass() will also be
// visible for subclasses, so keep it general.
/*!
  Initialize relevant common data for all instances, like the type
  system.
 */
void
SoDetail::initClass(void)
{
  SoDetail::classTypeId =
    SoType::createType(SoType::badType(), SbName("SoDetail"));
  SoDetail::initClasses();
}

/*!
  Call the initClass() methods of all built-in detail classes.

  (The initClass() method of user extension detail classes -- if any
  -- must be called explicitly by the application programmer in the
  application initialization code.)
 */
void
SoDetail::initClasses(void)
{
  SoConeDetail::initClass();
  SoCubeDetail::initClass();
  SoCylinderDetail::initClass();
  SoFaceDetail::initClass();
  SoLineDetail::initClass();
  SoPointDetail::initClass();
  SoTextDetail::initClass();
#ifdef HAVE_NODEKITS
  SoNodeKitDetail::initClass();
#endif // HAVE_NODEKITS
}
