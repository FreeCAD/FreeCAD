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

#include <QImage>
#include <Inventor/SbImage.h>
#include <Inventor/errors/SoDebugError.h>

#include "QtCoinCompatibility.h"


using namespace SIM::Coin3D::Quarter;

void
QtCoinCompatibility::QImageToSbImage(const QImage & image, SbImage & sbimage)
{
    int w = image.width();
    int h = image.height();
    int c;

    // Keep in 8-bits mode if that was what we read
    if (image.depth() == 8 && image.isGrayscale()) {
      c = 1;
    }
    else {
      // FIXME: consider if we should detect allGrayscale() and alpha (c = 2)
      c = image.hasAlphaChannel() ? 4 : 3;
    }

    SbVec2s size((short) w, (short) h);
    sbimage.setValue(size, c, nullptr);
    unsigned char * buffer = sbimage.getValue(size, c);

    if (c == 1) {
      for (int i = 0; i < h; i++) {
        memcpy(buffer + i*w, image.scanLine(h-(i+1)), w);
      }
    }
    else { // (c == 3 || c == 4)
      QRgb * bits = (QRgb*) image.bits();
      for (int y = 0; y < h; y++) {
        unsigned char * line = &buffer[c*w*(h-(y+1))];
        for (int x = 0; x < w; x++) {
          *line++ = qRed(*bits);
          *line++ = qGreen(*bits);
          *line++ = qBlue(*bits);
          if (c == 4) {
            *line++ = qAlpha(*bits);
          }
          bits++;
        }
      }
    }
}

void
QtCoinCompatibility::SbImageToQImage(const SbImage & sbimage, QImage & img)
{
  SbVec2s ivsize;
  int nc;
  const unsigned char* src = sbimage.getValue (ivsize, nc);
  QSize size(ivsize[0],ivsize[1]);
  assert(src && "Empty image");
  if (nc!=3&&nc!=1&&nc!=4) {
    SoDebugError::postWarning("QtCoinCompatibility::SbImageToQImage",
                        "Implementation not tested for 3 colors or more"
                           );
  }
  QImage::Format format=QImage::Format_Invalid;
  if (nc==3||nc==4) {
    format=QImage::Format_RGB32;
  }
  else if (nc==1) {
    QVector<QRgb> clut;
    for (int i=0;i<256;++i) {
      clut.append(qRgb(i,i,i));
    }
    format=QImage::Format_Indexed8;
  }
  img = QImage(size,format);
  assert(img.size()==size);
  if (nc==1) {
    QVector<QRgb> clut;
    for (int i=0;i<256;++i) {
      clut.append(qRgb(i,i,i));
    }
    img.setColorTable(clut);
  }

  for (int y = 0; y < size.height(); ++y) {
    QRgb * bits = reinterpret_cast<QRgb *>(img.scanLine(size.height() - (y+1)));
    for (int x = 0; x < size.width(); ++x) {
      switch (nc) {
      default:
      case 1:
       {
         img.setPixel(x,size.height()-(y+1),*src++);
       }
       break;
      case 2:
       {
         unsigned char red=*src++;
         unsigned char alpha=*src++;
         *bits=qRgba(red,red,red,alpha);
       }
       break;
      case 3:
       {
         unsigned char red=*src++;
         unsigned char green=*src++;
         unsigned char blue=*src++;
         *bits=qRgb(red,green,blue);
       }
       break;
      case 4:
       {
         unsigned char red=*src++;
         unsigned char green=*src++;
         unsigned char blue=*src++;
         unsigned char alpha=*src++;
         *bits=qRgba(red,green,blue,alpha);
       }
       break;
      }
      ++bits;
    }
  }
}
