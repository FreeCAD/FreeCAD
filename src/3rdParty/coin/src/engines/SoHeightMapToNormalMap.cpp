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

#include <Inventor/engines/SoHeightMapToNormalMap.h>

#include <memory>

#include <Inventor/SbVec3f.h>
#include <Inventor/SbImage.h>
#include "engines/SoSubEngineP.h"

/*!
  \class SoHeightMapToNormalMap SoHeightMapToNormalMap.h Inventor/engines/SoHeightMapToNormalMap.h
  \brief Engine for computing a normal map from a height map.

  This engine will create a normal map texture from a height map texture.
  You can use it in an Inventor file like this:

  \code
  Texture2 {
    image = HeightMapToNormalMap {
      sourceImage = Texture2 { filename "HeightMap.jpg" } . image
    } . image
  }
  \endcode

  Be aware that the field connections will remain active, so both
  Texture2 nodes and the HeightMapToNormalMap engine will be kept resident
  in memory (unless you intervene manually and detach the engine) even
  though only the "outer" Texture2 node is needed. This can give quite
  a big memory use overhead.

  \ingroup coin_engines
  \COIN_CLASS_EXTENSION
  \since Coin 3.0
*/

/*!
  \enum SoHeightMapToNormalMap::NormalMapFormat
  Enumeration of available normal map formats.
*/

/*!
  \var SoHeightMapToNormalMap::NormalMapFormat SoHeightMapToNormalMap::INT8
  Encode the normals as a 3 component byte texture.
  This is the only option for now, as long as float textures are not conveniently
  supported in Coin.
*/

/*!
  \var SoMFEnum SoHeightMapToNormalMap::format
  This setting decides what kind of normal map is generated.  For now, only the
  INT8 format is available, and it is the default value.
*/

SO_ENGINE_SOURCE(SoHeightMapToNormalMap);

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoHeightMapToNormalMap::initClass(void)
{
  SO_ENGINE_INTERNAL_INIT_CLASS(SoHeightMapToNormalMap);
}

/*!
  Constructor.
*/
SoHeightMapToNormalMap::SoHeightMapToNormalMap(void)
{
  SO_ENGINE_INTERNAL_CONSTRUCTOR(SoHeightMapToNormalMap);

  SO_ENGINE_ADD_INPUT(format, (INT8));

  SO_ENGINE_DEFINE_ENUM_VALUE(NormalMapFormat, INT8);
  SO_ENGINE_SET_SF_ENUM_TYPE(format, NormalMapFormat);
}

/*!
  Static function for computing a normal map from a height map.
  This function can be used directly without any engine instantiation.
*/
void
SoHeightMapToNormalMap::convert(const unsigned char * srcptr, SbVec2s size, int nc, SbImage & dst_out)
{
  float dx, dy;
  int width = size[0];
  int height = size[1];
  std::unique_ptr<unsigned char[]> dstarray(new unsigned char[width*height*3]);
  unsigned char * dstptr = dstarray.get();
  unsigned char red;
  SbVec3f n;

#define GET_PIXEL_RED(x_, y_) \
  srcptr[(y_)*width*nc + (x_)*nc]

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      // do Y Sobel filter
      red = GET_PIXEL_RED((x-1+width) % width, (y+1) % height);
      dy  = static_cast<float>(red) / 255.0f * -1.0f;

      red = GET_PIXEL_RED(x % width, (y+1) % height);
      dy += static_cast<float>(red) / 255.0f * -2.0f;

      red = GET_PIXEL_RED((x+1) % width, (y+1) % height);
      dy += static_cast<float>(red) / 255.0f * -1.0f;

      red = GET_PIXEL_RED((x-1+width) % width, (y-1+height) % height);
      dy += static_cast<float>(red) / 255.0f *  1.0f;

      red = GET_PIXEL_RED(x % width, (y-1+height) % height);
      dy += static_cast<float>(red) / 255.0f *  2.0f;

      red = GET_PIXEL_RED((x+1) % width, (y-1+height) % height);
      dy += static_cast<float>(red) / 255.0f *  1.0f;

      // Do X Sobel filter
      red = GET_PIXEL_RED((x-1+width) % width, (y-1+height) % height);
      dx  = static_cast<float>(red) / 255.0f * -1.0f;

      red = GET_PIXEL_RED((x-1+width) % width, y % height);
      dx += static_cast<float>(red) / 255.0f * -2.0f;

      red = GET_PIXEL_RED((x-1+width) % width, (y+1) % height);
      dx += static_cast<float>(red) / 255.0f * -1.0f;

      red = GET_PIXEL_RED((x+1) % width, (y-1+height) % height);
      dx += static_cast<float>(red) / 255.0f *  1.0f;

      red = GET_PIXEL_RED((x+1) % width, y % height);
      dx += static_cast<float>(red) / 255.0f *  2.0f;

      red = GET_PIXEL_RED((x+1) % width, (y+1) % height);
      dx += static_cast<float>(red) / 255.0f *  1.0f;

      n[0] = -dx;
      n[1] = -dy;
      n[2] = 1.0f;
      (void) n.normalize();

      *dstptr++ = static_cast<unsigned char>(SbMin((n[0]+1.0f) * 128.0f, 255.0f));
      *dstptr++ = static_cast<unsigned char>(SbMin((n[1]+1.0f) * 128.0f, 255.0f));
      *dstptr++ = static_cast<unsigned char>(SbMin((n[2]+1.0f) * 128.0f, 255.0f));
    }
  }
#undef GET_PIXEL_RED
  dst_out.setValue(size, 3, dstarray.get());
}

void
SoHeightMapToNormalMap::inputChanged(SoField * which)
{
  // in case we need to override later
  inherited::inputChanged(which);
}

void
SoHeightMapToNormalMap::evaluate(void)
{
  SbVec2s size;
  int nc;
  const unsigned char * ptr =
    static_cast<const unsigned char *>(sourceImage.getValue(size, nc));

  SbImage targetimg;
  SoHeightMapToNormalMap::convert(ptr, size, nc, targetimg);

  ptr = static_cast<const unsigned char *>(targetimg.getValue(size, nc));
  SO_ENGINE_OUTPUT(image, SoSFImage, setValue(size, nc, ptr));
}
