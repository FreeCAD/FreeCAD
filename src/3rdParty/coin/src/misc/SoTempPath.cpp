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
  \class SoTempPath SoTempPath.h Inventor/misc/SoTempPath.h
  \brief The SoTempPath class is used to store temporary paths.

  \ingroup coin_general

  The path simply turns off auditing in the constructor, and leaves
  the user with the responsibility of keeping the path valid.
*/

#include <Inventor/misc/SoTempPath.h>

/*!
  Constructor.
*/
SoTempPath::SoTempPath(const int approxlength)
  : SoFullPath(approxlength)
{
  this->auditPath(FALSE);
  this->nodes.addReferences(FALSE);
}

/*!
  Append a node (specified by \a node and parent child \a index) to the path.
  This method is only available in SoTempPath, since it will not
  consider auditing or hidden children.
*/
void
SoTempPath::simpleAppend(SoNode * const node, const int index)
{
  // this will make SoPath rescan the path for hidden children the
  // next time getLength() is called.
  this->firsthiddendirty = TRUE;

  // just append node and index
  this->nodes.append(node);
  this->indices.append(index);
}

/*!  
  Replace the tail of this path. The node is specified by \a node
  and parent child \a index. This method is only available in
  SoTempPath, since it will not consider auditing or hidden children.
*/
void 
SoTempPath::replaceTail(SoNode * const node, const int index)
{
  // this will make SoPath rescan the path for hidden children the
  // next time getLength() is called.
  this->firsthiddendirty = TRUE;

  // just replace the last node and index
  const int i = this->nodes.getLength() - 1;
  this->nodes.set(i, (SoBase*) node);
  this->indices[i] = index;
}
