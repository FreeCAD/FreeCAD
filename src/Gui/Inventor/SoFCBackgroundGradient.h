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

#ifndef GUI_SOFCBACKGROUNDGRADIENT_H
#define GUI_SOFCBACKGROUNDGRADIENT_H

#include <Inventor/SbColor.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoSubNode.h>
#include <FCGlobal.h>
#include <QImage>
#include <QPointer>


class QTimer;
class QOpenGLTexture;
class SoGLRenderAction;

namespace Gui {

class View3DInventorViewer;

class GuiExport SoFCBackgroundGradient : public SoNode {
    using inherited = SoNode;

    SO_NODE_HEADER(Gui::SoFCBackgroundGradient);

public:
    enum Gradient {
        LINEAR = 0,
        RADIAL = 1
    };
    static void initClass();
    static void finish();
    SoFCBackgroundGradient();

    void GLRender (SoGLRenderAction *action) override;
    void setGradient(Gradient grad);
    Gradient getGradient() const;
    void setColorGradient(const SbColor& fromColor,
                          const SbColor& toColor);
    void setColorGradient(const SbColor& fromColor,
                          const SbColor& toColor,
                          const SbColor& midColor);
    void setupUpdater(void);
    void setViewer(View3DInventorViewer *new_viewer);

private:
    Gradient gradient;

protected:
    ~SoFCBackgroundGradient() override;

    QColor fCol, tCol, mCol;

    Gradient old_gradient{};
    QColor old_fCol{}, old_tCol{}, old_mCol{};

    QScopedPointer <QOpenGLTexture> bgTex{};    // background texture
    QScopedPointer <QOpenGLTexture> noiseTex{}; // interleaved white noise texture with which the background texture is mixed

    QImage bgImage{};
    inline static QImage noiseImage{};

    inline static constexpr QSize bgTex_size{1280, 720}; // Coin3D may report incorrect values of the size of the viewport
    inline static constexpr QSize noise_size{1280, 720};
    inline static constexpr double bgTex_opacity = 0.95;

    inline static constexpr int pre_update_delay = 300;
    QPointer <QTimer> updater;
    QPointer <View3DInventorViewer> viewer;
};

/*!
 * \brief A function to generate 2D white noise
 * \param size
 * \return QImage containing 2D white noise
 * Generates 2D white noise of QImage::Format_RGB32.
 */
QImage whiteNoise(const QSize &size);
QImage whiteNoise(const int width, const int height);

/*!
 * \brief A function to generate grey checkerboard pattern
 * \param img
 * The creators of Call of Duty: Advanced Warfare recommend using interleaved gradient noise, possessing properties from both dithering and random approaches. It has rich range of values like random noise, but at the same time it produces temporally coherent results.
This function creates interleaved white noise invisible to the naked eye, but making big areas of the same color pleasant to the eyes with spatial coherency.
 */
void checkerboardPattern(QImage &img);

} // namespace Gui


#endif // GUI_SOFCBACKGROUNDGRADIENT_H

