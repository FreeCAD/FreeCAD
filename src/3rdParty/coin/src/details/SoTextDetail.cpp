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
  \class SoTextDetail SoTextDetail.h Inventor/details/SoTextDetail.h
  \brief The SoTextDetail stores information about a character in a string.

  \ingroup coin_details

  Instances of this class are used for storing information about hit
  points on textual 2D or 3D geometry after pick operations, and for
  storing information returned to tessellation callbacks.

  \sa SoRayPickAction, SoCallbackAction
  \sa SoText3, SoText2, SoAsciiText
*/
// FIXME: write a full test and usage example to see if this class is
// actually used properly by its "client" shape nodes. 20011128 mortene.

#include <Inventor/details/SoTextDetail.h>
#include <Inventor/SbName.h>

SO_DETAIL_SOURCE(SoTextDetail);

/*!
  Constructor sets up an "empty" detail; all indices are set to -1 to
  indicate this.
*/
SoTextDetail::SoTextDetail(void)
  : stringindex(-1),
    charindex(-1),
    part(-1)
{
}

/*!
  Destructor. This class does not allocate any extra resources, so no
  actions are taken.
*/
SoTextDetail::~SoTextDetail()
{
}

/*!
  \copybrief SoDetail::initClass(void)
*/
void
SoTextDetail::initClass(void)
{
  SO_DETAIL_INIT_CLASS(SoTextDetail, SoDetail);
}

// Doc in superclass.
SoDetail *
SoTextDetail::copy(void) const
{
  SoTextDetail *copy = new SoTextDetail;
  *copy = *this;
  return copy;
}

/*!
  Returns the index of the string where a character was hit /
  generated, from a set of multiple strings.

  \sa SoMFString
*/
int
SoTextDetail::getStringIndex(void) const
{
  return this->stringindex;
}

/*!
  Returns the index of the character in the string which was hit.

  \sa getStringIndex()
*/
int
SoTextDetail::getCharacterIndex(void) const
{
  return this->charindex;
}

/*!
  For SoText3, returns the part id of the text geometry. The id
  numbers matches those specified in the SoText3::Part enumeration.
*/
int
SoTextDetail::getPart(void) const
{
  return this->part;
}

/*!
  Internal method for "client" shape nodes to initialize this
  SoTextDetail instance.
*/
void
SoTextDetail::setStringIndex(const int idx)
{
  this->stringindex = idx;
}

/*!
  Internal method for "client" shape nodes to initialize this
  SoTextDetail instance.
*/
void
SoTextDetail::setCharacterIndex(const int idx)
{
  this->charindex = idx;
}

/*!
  Internal method for "client" shape nodes to initialize this
  SoTextDetail instance.
*/
void
SoTextDetail::setPart(const int partarg)
{
  this->part = partarg;
}
