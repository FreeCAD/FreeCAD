#include <Quarter/QtCoinCompatibility.h>
#include <Inventor/SbImage.h>
#include <QtGui/QImage>
#include <Inventor/errors/SoDebugError.h>

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
    sbimage.setValue(size, c, NULL);
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
