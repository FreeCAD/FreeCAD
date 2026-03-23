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
  \class SoPSVectorOutput SoPSVectorOutput.h Inventor/Annex/HardCopy/SoPSVectorOutput.h
  \brief The SoPSVectorOutput class is used for writing PostScript.

  \ingroup coin_hardcopy

  \since Coin 2.1
  \since TGS provides HardCopy support as a separate extension for TGS Inventor.
*/

#include <Inventor/annex/HardCopy/SoPSVectorOutput.h>
#include <Inventor/SbBasic.h>

class SoPSVectorOutputP {
public:
  SbBool colored;
};

#define PRIVATE(p) (p->pimpl)

/*!
  Constructor.
*/
SoPSVectorOutput::SoPSVectorOutput()
{
  PRIVATE(this) = new SoPSVectorOutputP;
  PRIVATE(this)->colored = TRUE;
}

/*!
  Destructor.
*/
SoPSVectorOutput::~SoPSVectorOutput()
{
  delete PRIVATE(this);
}

/*!
  Sets whether the geometry should be colored.
*/
void
SoPSVectorOutput::setColored(SbBool flag)
{
  PRIVATE(this)->colored = flag;
}

/*!
  Returns whether geometry is colored.
*/
SbBool
SoPSVectorOutput::getColored(void) const
{
  return PRIVATE(this)->colored;
}

#undef PRIVATE
