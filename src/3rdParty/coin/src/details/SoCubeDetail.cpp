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
  \class SoCubeDetail SoCubeDetail.h Inventor/details/SoCubeDetail.h
  \brief The SoCubeDetail class contains information about the parts of a SoCube shape.

  \ingroup coin_details

  Instances of this class are used for storing information about hit
  points on cone geometry after pick operations, and for storing
  information returned to tessellation callbacks.

  The part indices are ordered from 0 to 5 as [ front, back, left,
  right, top, bottom ].

  \sa SoCube, SoRayPickAction, SoCallbackAction
*/

/*!
  \var int SoCubeDetail::part
  \COININTERNAL
*/

#include <Inventor/details/SoCubeDetail.h>
#include <Inventor/SbName.h>

SO_DETAIL_SOURCE(SoCubeDetail);

/*!
  Constructor.
 */
SoCubeDetail::SoCubeDetail(void)
  : part(0)
{
}

/*!
  Destructor.
 */
SoCubeDetail::~SoCubeDetail()
{
}

/*!
  \copybrief SoDetail::initClass(void)
*/
void
SoCubeDetail::initClass(void)
{
  SO_DETAIL_INIT_CLASS(SoCubeDetail, SoDetail);
}

// doc in super
SoDetail *
SoCubeDetail::copy(void) const
{
  SoCubeDetail * copy = new SoCubeDetail;
  copy->part = this->part;
  return copy;
}

/*!
  Set the part of a cube which was selected. A cube has of course six
  different conceptual parts -- its sides.
  */
void
SoCubeDetail::setPart(const int partarg)
{
  this->part = partarg;
}

/*!
  Returns selected cube part.
 */
int
SoCubeDetail::getPart(void) const
{
  return this->part;
}
