#ifndef COIN_SOGENERATE_H
#define COIN_SOGENERATE_H

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

#ifndef COIN_INTERNAL
#error this is a private header file
#endif /* !COIN_INTERNAL */

//
// reusable code for generatePrimitives (OpenInventor, VRML and VRML2)
//

#include <Inventor/SbBasic.h>
#include <Inventor/system/inttypes.h>

class SoShape;
class SoAction;

// flags for cone, cylinder and cube

#define SOGEN_GENERATE_SIDE       0x01
#define SOGEN_GENERATE_TOP        0x02
#define SOGEN_GENERATE_BOTTOM     0x04
#define SOGEN_MATERIAL_PER_PART   0x08

void sogen_generate_cone(const float bottomRadius,
                         const float height,
                         const int numslices,
                         const unsigned int flags,
                         SoShape * const shape,
                         SoAction * const action);


void sogen_generate_cylinder(const float radius,
                             const float height,
                             const int numslices,
                             const unsigned int flags,
                             SoShape * const shape,
                             SoAction * const action);

void sogen_generate_sphere(const float radius,
                           const int numstacks,
                           const int numslices,
                           SoShape * const shape,
                           SoAction * const action);

void sogen_generate_cube(const float width,
                         const float height,
                         const float depth,
                         const unsigned int flags,
                         SoShape * const shape,
                         SoAction * const action);

#endif // !COIN_SOGENERATE_H
