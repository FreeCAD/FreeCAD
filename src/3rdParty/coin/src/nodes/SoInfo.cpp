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
  \class SoInfo SoInfo.h Inventor/nodes/SoInfo.h
  \brief The SoInfo class is a node for holding text information.

  \ingroup coin_nodes

  This is simply a placeholder for textual information about a
  scene. Typically used for specifying author and copyright
  information in Inventor format files, but can also be used for other
  purposes, of course.

  SoInfo nodes does not influence traversal in any way for any of the
  built-in actions.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    Info {
        string "<Undefined info>"
    }
  \endcode
*/

// *************************************************************************

#include <Inventor/nodes/SoInfo.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************

/*!
  \var SoSFString SoInfo::string
  Text information.
*/

// *************************************************************************

SO_NODE_SOURCE(SoInfo);

/*!
  Constructor.
*/
SoInfo::SoInfo(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoInfo);

  SO_NODE_ADD_FIELD(string, ("<Undefined info>"));
}

/*!
  Destructor.
*/
SoInfo::~SoInfo()
{
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoInfo::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoInfo, SO_FROM_INVENTOR_1|SoNode::VRML1);
}
