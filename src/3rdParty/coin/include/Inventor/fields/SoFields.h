#ifndef COIN_SOFIELDS_H
#define COIN_SOFIELDS_H

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

#include <Inventor/fields/SoField.h>
#include <Inventor/fields/SoSField.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoSFBox2s.h>
#include <Inventor/fields/SoSFBox2i32.h>
#include <Inventor/fields/SoSFBox2f.h>
#include <Inventor/fields/SoSFBox2d.h>
#include <Inventor/fields/SoSFBox3s.h>
#include <Inventor/fields/SoSFBox3i32.h>
#include <Inventor/fields/SoSFBox3f.h>
#include <Inventor/fields/SoSFBox3d.h>
#include <Inventor/fields/SoSFColor.h>
#include <Inventor/fields/SoSFColorRGBA.h>
#include <Inventor/fields/SoSFDouble.h>
#include <Inventor/fields/SoSFEnum.h>
#include <Inventor/fields/SoSFBitMask.h>
#include <Inventor/fields/SoSFEngine.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFImage.h>
#include <Inventor/fields/SoSFImage3.h>
#include <Inventor/fields/SoSFInt32.h>
#include <Inventor/fields/SoSFMatrix.h>
#include <Inventor/fields/SoSFName.h>
#include <Inventor/fields/SoSFNode.h>
#include <Inventor/fields/SoSFPath.h>
#include <Inventor/fields/SoSFPlane.h>
#include <Inventor/fields/SoSFRotation.h>
#include <Inventor/fields/SoSFShort.h>
#include <Inventor/fields/SoSFString.h>
#include <Inventor/fields/SoSFTime.h>
#include <Inventor/fields/SoSFTrigger.h>
#include <Inventor/fields/SoSFUInt32.h>
#include <Inventor/fields/SoSFUShort.h>
#include <Inventor/fields/SoSFVec2b.h>
#include <Inventor/fields/SoSFVec2s.h>
#include <Inventor/fields/SoSFVec2i32.h>
#include <Inventor/fields/SoSFVec2f.h>
#include <Inventor/fields/SoSFVec2d.h>
#include <Inventor/fields/SoSFVec3b.h>
#include <Inventor/fields/SoSFVec3s.h>
#include <Inventor/fields/SoSFVec3i32.h>
#include <Inventor/fields/SoSFVec3f.h>
#include <Inventor/fields/SoSFVec3d.h>
#include <Inventor/fields/SoSFVec4b.h>
#include <Inventor/fields/SoSFVec4ub.h>
#include <Inventor/fields/SoSFVec4s.h>
#include <Inventor/fields/SoSFVec4us.h>
#include <Inventor/fields/SoSFVec4i32.h>
#include <Inventor/fields/SoSFVec4ui32.h>
#include <Inventor/fields/SoSFVec4f.h>
#include <Inventor/fields/SoSFVec4d.h>

#include <Inventor/fields/SoMField.h>
#include <Inventor/fields/SoMFBool.h>
#include <Inventor/fields/SoMFColor.h>
#include <Inventor/fields/SoMFColorRGBA.h>
#include <Inventor/fields/SoMFDouble.h>
#include <Inventor/fields/SoMFEngine.h>
#include <Inventor/fields/SoMFEnum.h>
#include <Inventor/fields/SoMFBitMask.h>
#include <Inventor/fields/SoMFFloat.h>
#include <Inventor/fields/SoMFInt32.h>
#include <Inventor/fields/SoMFMatrix.h>
#include <Inventor/fields/SoMFName.h>
#include <Inventor/fields/SoMFNode.h>
#include <Inventor/fields/SoMFPath.h>
#include <Inventor/fields/SoMFPlane.h>
#include <Inventor/fields/SoMFRotation.h>
#include <Inventor/fields/SoMFShort.h>
#include <Inventor/fields/SoMFString.h>
#include <Inventor/fields/SoMFTime.h>
#include <Inventor/fields/SoMFUInt32.h>
#include <Inventor/fields/SoMFUShort.h>
#include <Inventor/fields/SoMFVec2b.h>
#include <Inventor/fields/SoMFVec2s.h>
#include <Inventor/fields/SoMFVec2i32.h>
#include <Inventor/fields/SoMFVec2f.h>
#include <Inventor/fields/SoMFVec2d.h>
#include <Inventor/fields/SoMFVec3b.h>
#include <Inventor/fields/SoMFVec3s.h>
#include <Inventor/fields/SoMFVec3i32.h>
#include <Inventor/fields/SoMFVec3f.h>
#include <Inventor/fields/SoMFVec3d.h>
#include <Inventor/fields/SoMFVec4b.h>
#include <Inventor/fields/SoMFVec4ub.h>
#include <Inventor/fields/SoMFVec4s.h>
#include <Inventor/fields/SoMFVec4us.h>
#include <Inventor/fields/SoMFVec4i32.h>
#include <Inventor/fields/SoMFVec4ui32.h>
#include <Inventor/fields/SoMFVec4f.h>
#include <Inventor/fields/SoMFVec4d.h>

#endif // !COIN_SOFIELDS_H
