#ifndef COIN_SBLINEPROJECTOR_H
#define COIN_SBLINEPROJECTOR_H

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

#include <Inventor/projectors/SbProjector.h>

#include <Inventor/SbVec3f.h>
#include <Inventor/SbLine.h>

class COIN_DLL_API SbLineProjector : public SbProjector {
  typedef SbProjector inherited;

public:
  SbLineProjector(void);
  virtual SbProjector * copy(void) const;

  virtual SbVec3f project(const SbVec2f & point);
  virtual SbBool tryProject(const SbVec2f & point, const float epsilon, SbVec3f & result);

  void setLine(const SbLine & line);
  const SbLine & getLine(void) const;
  virtual SbVec3f getVector(const SbVec2f & viewpos1,
                            const SbVec2f & viewpos2);
  virtual SbVec3f getVector(const SbVec2f & viewpos);
  void setStartPosition(const SbVec2f & viewpos);
  void setStartPosition(const SbVec3f & point);

protected:
  SbLine line;
  SbVec3f lastPoint;
};

#endif // !COIN_SBLINEPROJECTOR_H
