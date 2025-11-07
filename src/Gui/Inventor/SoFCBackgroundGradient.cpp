/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include <span>
#include <QTimer>
#include <QPainter>
#include <QOpenGLTexture>
#include <QFutureWatcher>
#include <QRandomGenerator>
#include <QGraphicsPixmapItem>
#include <QGraphicsBlurEffect>
#include <QtConcurrent/QtConcurrentRun>
#include <Gui/View3DInventorViewer.h>

#include "SoFCBackgroundGradient.h"


using namespace Gui;

SO_NODE_SOURCE(SoFCBackgroundGradient)

void SoFCBackgroundGradient::finish()
{
    atexit_cleanup();
}

/*!
  Constructor.
*/
SoFCBackgroundGradient::SoFCBackgroundGradient()
{
    SO_NODE_CONSTRUCTOR(SoFCBackgroundGradient);
    fCol.setRgbF(0.5, 0.5, 0.8);
    tCol.setRgbF(0.7, 0.7, 0.9);
    mCol.setRgbF(1.0, 1.0, 1.0);
    gradient = Gradient::LINEAR;

    setupUpdater();

    if (bgImage.isNull()){
        QPixmap pix{1, 1};
        pix.fill(Qt::GlobalColor::white);
        bgImage = pix.toImage();
    }

    if (noiseImage.isNull()){
        noiseImage = whiteNoise(noise_size);
        checkerboardPattern(noiseImage);
    }
}

/*!
  Destructor.
*/
SoFCBackgroundGradient::~SoFCBackgroundGradient() = default;

// doc from parent
void SoFCBackgroundGradient::initClass()
{
    SO_NODE_INIT_CLASS(SoFCBackgroundGradient,SoNode,"Node");
}

void SoFCBackgroundGradient::GLRender (SoGLRenderAction * /*action*/)
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-1,1,-1,1,-1,1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glPushAttrib(GL_ENABLE_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);


    glEnable(GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if(noiseTex == nullptr){
        noiseTex.reset(new QOpenGLTexture(noiseImage));
        noiseTex->setMinMagFilters(QOpenGLTexture::Filter::Nearest, QOpenGLTexture::Filter::Nearest);
        noiseTex->setWrapMode(QOpenGLTexture::WrapMode::Repeat);
        noiseTex->generateMipMaps();
    }
    noiseTex->bind();

    glBegin(GL_QUADS);
    glColor4d(1.0, 1.0, 1.0, 1.0);
    glTexCoord2d(0.0, 0.0); glVertex2d(-1.0, -1.0);
    glTexCoord2d(0.0, 1.0); glVertex2d(-1.0, +1.0);
    glTexCoord2d(1.0, 1.0); glVertex2d(+1.0, +1.0);
    glTexCoord2d(1.0, 0.0); glVertex2d(+1.0, -1.0);
    glEnd();


    if(bgTex == nullptr){
        bgTex.reset(new QOpenGLTexture(bgImage));
        bgTex->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
        bgTex->setWrapMode(QOpenGLTexture::ClampToEdge);
        bgTex->generateMipMaps();
    }
    bgTex->bind();

    glBegin(GL_QUADS);
    glColor4d(1.0, 1.0, 1.0, bgTex_opacity);
    glTexCoord2d(0.0, 0.0); glVertex2d(-1.0, -1.0);
    glTexCoord2d(0.0, 1.0); glVertex2d(-1.0, +1.0);
    glTexCoord2d(1.0, 1.0); glVertex2d(+1.0, +1.0);
    glTexCoord2d(1.0, 0.0); glVertex2d(+1.0, -1.0);
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glPopAttrib();
    glPopMatrix(); // restore modelview
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void SoFCBackgroundGradient::setGradient(SoFCBackgroundGradient::Gradient grad)
{
    gradient = grad;

    updater->start();
}

SoFCBackgroundGradient::Gradient SoFCBackgroundGradient::getGradient() const
{
    return gradient;
}

void SoFCBackgroundGradient::setColorGradient(const SbColor& fromColor,
                                              const SbColor& toColor)
{
    fCol = QColor::fromRgbF(fromColor[0], fromColor[1], fromColor[2]);
    tCol = QColor::fromRgbF(toColor[0], toColor[1], toColor[2]);
    mCol = QColor{};

    updater->start();
}

void SoFCBackgroundGradient::setColorGradient(const SbColor& fromColor,
                                              const SbColor& toColor,
                                              const SbColor& midColor)
{
    fCol = QColor::fromRgbF(fromColor[0], fromColor[1], fromColor[2]);
    tCol = QColor::fromRgbF(toColor[0], toColor[1], toColor[2]);
    mCol = QColor::fromRgbF(midColor[0], midColor[1], midColor[2]);

    updater->start();
}

static
QImage createLinearGradient(const QSize &&size, const QList <QColor> &colors)
{
    const int w = size.width(), h = size.height();
    const QColor &fromColor = colors.first(),
                 &toColor = colors.at(1),
                 &midColor = colors.last();
    const QPointF start(0, h/2), finalStop(w, h/2);

    QLinearGradient linearGrad(start, finalStop);
    if (midColor.isValid()){
        linearGrad.setStops({{0.0, fromColor}, {0.5, midColor}, {1.0, toColor}});
    }else {
        linearGrad.setStops({{0.0, fromColor}, {1.0, toColor}});
    }

   QPixmap pix{w, h};
   QPainter painter{&pix};
   painter.fillRect(0, 0, w, h, linearGrad);

   return pix.toImage();
}

static
QImage createRadialGradient(const QSize &&size, const QList <QColor> &colors)
{
    const int w = size.width(), h = size.height();
    const QColor &fromColor = colors.first(),
                 &toColor = colors.at(1),
                 &midColor = colors.last();
    const qreal cx = static_cast <qreal> (w) / 2.0,
                cy = static_cast <qreal> (h) / 2.0,
                r  = static_cast <qreal> (w) / 2.0;

    QRadialGradient radialGrad(cx, cy, r);
    if (midColor.isValid()){
        radialGrad.setStops({{0.0, fromColor}, {0.5, midColor}, {1.0, toColor}});
    }else {
        radialGrad.setStops({{0.0, fromColor}, {1.0, toColor}});
    }

    QPixmap pix{w, h};
    QPainter painter{&pix};
    painter.fillRect(0, 0, w, h, radialGrad);

    return pix.toImage();
}

static
void horizontalShuffle(QImage &img, const qreal percent)
{
    const int w = img.width(), h = img.height();
    const int shuffle_distance = static_cast <int> (static_cast <qreal> (w) * percent / 100.0);
    QRandomGenerator rng{QRandomGenerator::system()->generate()};

    for (int y = 1; y + 2 < h; y += 3){
        std::span <QRgb> f_span(reinterpret_cast <QRgb *> (img.scanLine(y)), w);
        std::span <QRgb> b_span(reinterpret_cast <QRgb *> (img.scanLine(y + 1)), w);
        for(int f = 1, b = w - shuffle_distance - 1; f + shuffle_distance < w - 1; f++, b--){
            auto f_subspan = f_span.subspan(f, shuffle_distance);
            std::shuffle(f_subspan.begin(), f_subspan.end(), rng);
            auto b_subspan = b_span.subspan(b, shuffle_distance);
            std::shuffle(b_subspan.begin(), b_subspan.end(), rng);
        }
    }
}

static
QImage createBgImage(const QSize &size,
                     SoFCBackgroundGradient::Gradient type,
                     const QList <QColor> &colors)
{
    Q_ASSERT_X(colors.count() == 3, "bgImage generation", "Three colors must be provided to create an image."
                                                          "If you are unsure, the third color can be invalid.");

    const int w = size.width(), h = size.height();
    constexpr qreal shuffle_percent = 1;
    QImage bgImage;

    switch(type){
        case SoFCBackgroundGradient::Gradient::LINEAR:{
            QImage hor_grad = createLinearGradient({h, w}, colors);
            horizontalShuffle(hor_grad, shuffle_percent);
            QImage rotated = hor_grad.transformed(QTransform().rotate(270.0));
            bgImage.swap(rotated);
        }break;
        case SoFCBackgroundGradient::Gradient::RADIAL:{
            QImage rad_grad = createRadialGradient({h, w}, colors);
            horizontalShuffle(rad_grad, shuffle_percent);
            QImage rotated = rad_grad.transformed(QTransform().rotate(90.0));
            horizontalShuffle(rotated, shuffle_percent*9.0/16.0);
            bgImage.swap(rotated);
        }break;
    }

    return bgImage;
}

QImage Gui::whiteNoise(const int width, const int height)
{
    QImage image(width, height, QImage::Format_RGB32);
    QRandomGenerator::global()->fillRange(reinterpret_cast <quint32 *> (image.scanLine(0)), width * height);
    return image;
}

QImage Gui::whiteNoise(const QSize &size)
{
    return whiteNoise(size.width(), size.height());
}

void Gui::checkerboardPattern(QImage &img)
{
    const int w = img.width(), h = img.height();
    auto pixelData = reinterpret_cast <quint32 *> (img.scanLine(0));

    for (int y = 0, i = 0; y < h; y++)
        for (int x = 0; x < w; x++, i++)
            if ( (x + y) % 2 == 0 )
                pixelData[i] = qRgb(128, 128, 128);
}

/* FreeCAD sets gradient and colors many times per several milliseconds.
 * QTimer updater "waits" till FreeCAD stops setting them and only then
 * generates the background image if the values have changed. After the generation
 * it sets the background image and updates the viewer. */
void SoFCBackgroundGradient::setupUpdater(void)
{
    updater = new QTimer();
    updater->setSingleShot(true);
    updater->setInterval(pre_update_delay);

    /* If there are N 3D viewers, when user changes colors or gradient,
     * there will be N + 2 regenerations of the background image.
     * Background image generation is supposed to prevent the UI from freezing. */
    auto *watcher = new QFutureWatcher <QImage> (updater);
    auto createFun = [this, watcher](){
        if (old_fCol == fCol && old_tCol == tCol && old_mCol == mCol &&
            old_gradient == gradient){
            return;
        }

        old_fCol = fCol; old_tCol = tCol; old_mCol = mCol;
        old_gradient = gradient;

        const QList <QColor> colors_lst{fCol, tCol, mCol};
        QFuture <QImage> future = QtConcurrent::run(createBgImage, bgTex_size, gradient, colors_lst);
        watcher->setFuture(future);
    };

    auto resetFun = [this, watcher](){
        bgImage = watcher->result();
        bgTex.reset();
        if(viewer){
            QMetaObject::invokeMethod(viewer, "redraw", Qt::QueuedConnection);
        }
    };
    QObject::connect(watcher, &QFutureWatcher<QImage>::finished, resetFun);
    updater->callOnTimeout(createFun);
}

void SoFCBackgroundGradient::setViewer(View3DInventorViewer *new_viewer)
{
    this->viewer = new_viewer;
    updater->setParent(new_viewer);
}


