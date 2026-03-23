#ifndef COIN_SOGEO_H
#define COIN_SOGEO_H

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

#include <Inventor/SbDPMatrix.h>
#include <Inventor/SbMatrix.h>
#include <Inventor/SbVec3d.h>

class SbString;
class SbVec3d;

class COIN_DLL_API SoGeo {
public:
  static void init(void);

  static SbVec3d toGD(const SbString * originsystem,
                      const int numoriginsys,
                      const SbVec3d & origincoords);
  static SbVec3d fromGD(const SbVec3d & gd,
                        const SbString * tosystem,
                        const int numtosys);

  static SbDPMatrix calculateDPTransform(const SbString * originsystem,
                                         const int numoriginsys,
                                         const SbVec3d & origincoords,
                                         const SbString * localsystem,
                                         const int numlocalsys,
                                         const SbVec3d & localcoords);


  static SbMatrix calculateTransform(const SbString * originsystem,
                                     const int numoriginsys,
                                     const SbVec3d & origincoords,
                                     const SbString * localsystem,
                                     const int numlocalsys,
                                     const SbVec3d & localcoords);
};

#endif // COIN_SOGEO_H
