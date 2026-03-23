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

#ifdef HAVE_NODEKITS

/*!
  \class SoNodeKitDetail SoNodeKitDetail.h Inventor/details/SoNodeKitDetail.h
  \brief The SoNodeKitDetail class is yet to be documented.

  \ingroup coin_details

  When a pick action is executed and geometry within a nodekit is hit,
  the nodekit generates an SoNodeKitDetail object which contains
  information about the specific part inside the nodekit hit by the pick
  ray.

  \sa SoRayPickAction
*/

#include <Inventor/details/SoNodeKitDetail.h>
#include <Inventor/nodekits/SoBaseKit.h>

/*!
  \var SoBaseKit * SoNodeKitDetail::myNodeKit
  The nodekit generating this details object.
*/
/*!
  \var SoNode * SoNodeKitDetail::myPart
  Node inside nodekit which was hit.
*/
/*!
  \var SbName SoNodeKitDetail::myPartName
  Catalog name of nodekit part which was hit.
*/


SO_DETAIL_SOURCE(SoNodeKitDetail);

/*!
  Constructor.
*/
SoNodeKitDetail::SoNodeKitDetail(void)
  : myNodeKit(NULL), myPart(NULL), myPartName("")
{
}

/*!
  Destructor.
*/
SoNodeKitDetail::~SoNodeKitDetail()
{
  if (this->myNodeKit) myNodeKit->unref();
  if (this->myPart) myPart->unref();
}

/*!
  \copybrief SoDetail::initClass(void)
*/
void
SoNodeKitDetail::initClass(void)
{
  SoNodeKitDetail::classTypeId =
    SoType::createType(inherited::getClassTypeId(),
                       SbName("SoNodeKitDetail"));
}

// Doc in superclass.
SoDetail *
SoNodeKitDetail::copy(void) const
{
  SoNodeKitDetail * copy = new SoNodeKitDetail();
  copy->myNodeKit = this->myNodeKit;
  copy->myPart = this->myPart;
  copy->myPartName = this->myPartName;

  // got to ref once more if set
  if (this->myNodeKit) this->myNodeKit->ref();
  if (this->myPart) this->myPart->ref();

  return copy;
}

/*!
  Set the pointer indicating which nodekit generated this detail object.
*/
void
SoNodeKitDetail::setNodeKit(SoBaseKit * kit)
{
  if (this->myNodeKit) this->myNodeKit->unref();
  this->myNodeKit = kit;
  if (kit) kit->ref();
}

/*!
  Returns a pointer to the nodekit generating this details object.
*/
SoBaseKit *
SoNodeKitDetail::getNodeKit(void) const
{
  return this->myNodeKit;
}

/*!
  Set the pointer indicating which node inside the nodekit was hit
  by a pick.
*/
void
SoNodeKitDetail::setPart(SoNode * part)
{
  if (this->myPart) this->myPart->unref();
  this->myPart = part;
  if (part) part->ref();
}

/*!
  Return node inside nodekit which was hit.
*/
SoNode *
SoNodeKitDetail::getPart(void) const
{
  return this->myPart;
}

/*!
  Set catalog name of node part which was hit.
*/
void
SoNodeKitDetail::setPartName(const SbName & name)
{
  this->myPartName = name;
}

/*!
  Return catalog name of nodekit part which was hit.
*/
const SbName &
SoNodeKitDetail::getPartName(void) const
{
  return this->myPartName;
}

#endif // HAVE_NODEKITS
