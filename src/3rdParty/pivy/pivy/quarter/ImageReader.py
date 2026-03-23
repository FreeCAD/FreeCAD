###
# Copyright (c) 2002-2008 Kongsberg SIM
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

from pivy.qt.QtGui import QImage
from pivy.coin import SbImage


def readImageCB(filename, image, closure):
    return closure.readImage(filename, image)


class ImageReader:
    def __init__(self):
        pass
        # FIXME: enable once ImageReader has been translated. 20080508
        # tamer.
        #SbImage.addReadImageCB(readImageCB, self)

    def __del__(self):
        SbImage.removeReadImageCB(readImageCB, self)

    def readImage(self, filename, sbimage):
        image = QImage()
        if (image.load(filename.getString())):
            # Keep in 8-bits mode if that was what we read
            if (image.depth() == 8 and image.isGrayscale()):
                c = 1
            else:
                # FIXME: consider if we should detect allGrayscale() and alpha (c = 2)
                c = 3
                if image.hasAlphaChannel():
                    c = 4
                    image.convertToFormat(QImage.Format_ARGB32)
                else:
                    image.convertToFormat(QImage.Format_RGB32)

                # FIXME 20080508 jkg: implement when pivy is ready
                #sbimage.setValue(SbVec2s(image.width(), image.height()), c, None)

                return True
        return False
