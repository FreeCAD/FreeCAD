// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2017 Kustaa Nyholm  <kustaa.nyholm@sparetimelabs.com>
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the     *
 *   License, or (at your option) any later version.                          *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful, but           *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of               *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Lesser General Public License for more details.                      *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD.  If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                         *
 *                                                                            *
 ******************************************************************************/

#include <FCConfig.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <numbers>
#include <set>
#include <vector>

#include <boost/math/constants/constants.hpp>

#include <Inventor/SbVec3f.h>
#include <Inventor/SbVec4f.h>
#include <Inventor/actions/SoAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoIRRenderAction.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/events/SoEvent.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/nodes/SoCallback.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoSeparator.h>

#include <QApplication>
#include <QCursor>
#include <QElapsedTimer>
#include <QImage>
#include <QMenu>
#include <QPainterPath>

#include <Base/Color.h>
#include <Base/Tools.h>
#include "NaviCube.h"
#include "Application.h"
#include "Camera.h"
#include "Command.h"
#include "Action.h"
#include "MainWindow.h"
#include "Navigation/NavigationAnimation.h"
#include "Navigation/NavigationStyle.h"
#include "Inventor/SoNaviCube.h"
#include "View3DInventorViewer.h"
#include "View3DInventor.h"
#include "ViewParams.h"


using namespace std;
using namespace Gui;

using PickId = Gui::SoNaviCube::PickId;

enum class FaceType
{
    None,
    Main,
    Edge,
    Corner,
    Button
};

class NaviCubeImplementation
{
public:
    explicit NaviCubeImplementation(Gui::View3DInventorViewer*);
    ~NaviCubeImplementation();
    void createContextMenu(const std::vector<std::string>& cmd);
    void createCubeFaceTextures();

    void moveToCorner(NaviCube::Corner c);
    void setLabels(const std::vector<std::string>& labels);

    bool processSoEvent(const SoEvent* ev);
    void setSize(int size);
    SoNode* getCoinNode() const;
    void requestRedraw();

private:
    void resetClickState();
    void setHiliteWithHysteresis(PickId);
    struct LabelTexture
    {
        qreal fontSize = 0.0;
        string label;
    };
    bool mousePressed(short x, short y);
    bool mouseReleased(short x, short y);
    bool mouseMoved(short x, short y);
    bool hasDraggedPastThreshold(short x, short y) const;
    void startCameraRotationDrag();
    void updateCameraRotationDrag(short x, short y);
    PickId pickFace(short x, short y);

    void prepare();
    void handleResize(const SbVec2s& viewSize);
    void handleMenu();

    void setHilite(PickId);

    FaceType getFaceType(PickId) const;
    SbRotation getFaceRotation(PickId) const;

    QString str(const char* str);
    QMenu* createNaviCubeMenu();
    bool readyToRender();
    bool populateRenderParams(
        float opacity,
        int viewportX,
        int viewportY,
        int viewportWidth,
        int viewportHeight
    );

    SbRotation getNearestOrientation(PickId pickId);
    qreal getPhysicalCubeWidgetSize();

public:
    static int cubeWidgetSize;
    QColor baseColor;
    QColor emphaseColor;
    QColor hiliteColor;
    bool showCS = true;
    PickId hiliteId = PickId::None;
    double borderWidth = 1.1;
    bool rotateToNearest = true;
    int naviStepByTurn = 8;
    float fontZoom = 0.3F;
    float chamfer = 0.12F;
    std::string textFont;
    int fontWeight = 0;
    int fontStretch = 0;
    float inactiveOpacity = 0.5;
    SbVec2s posOffset = SbVec2s(0, 0);

    Base::Color xColor {1.0F, 0.0F, 0.0F};
    Base::Color yColor {0.0F, 1.0F, 0.0F};
    Base::Color zColor {0.0F, 0.0F, 1.0F};

    bool prepared = false;
    static vector<string> commands;
    bool draggable = false;
    SbVec2s viewSize = SbVec2s(0, 0);

private:
    enum class MouseDragMode
    {
        None,
        MoveNaviCube,
        RotateCamera
    };

    bool mouseDown = false;
    bool dragStarted = false;
    bool hovering = false;
    QElapsedTimer clickTimer;
    PickId lastClickPickId = PickId::None;
    PickId pendingHiliteId = PickId::None;
    int pendingHiliteCount = 0;
    MouseDragMode dragMode = MouseDragMode::None;
    SbVec2s pressPos = SbVec2s(0, 0);
    SbVec2s lastDragPos = SbVec2s(0, 0);

    SbVec2f relPos = SbVec2f(1.0f, 1.0f);
    SbVec2s posAreaBase = SbVec2s(0, 0);
    SbVec2s posAreaSize = SbVec2s(0, 0);
    qreal devicePixelRatio = 1.0;

    Gui::View3DInventorViewer* viewer;

    Gui::SoNaviCube* soNaviCube = nullptr;

    map<PickId, LabelTexture> labelTextures;

    QMenu* menu;

    std::shared_ptr<NavigationAnimation> flatButtonAnimation;
    SbRotation flatButtonTargetOrientation;

    void syncNodeState(SoAction* action);
    static void traversalCallback(void* userdata, SoAction* action);

    SoSeparator* coinRoot = nullptr;
    SoCallback* actionSync = nullptr;
};

int NaviCubeImplementation::cubeWidgetSize = 132;

int NaviCube::getNaviCubeSize()
{
    return NaviCubeImplementation::cubeWidgetSize;
}

SoNode* NaviCube::getCoinNode() const
{
    return naviCubeImplementation->getCoinNode();
}

NaviCube::NaviCube(Gui::View3DInventorViewer* viewer)
{
    naviCubeImplementation = new NaviCubeImplementation(viewer);
    updateColors();
}

NaviCube::~NaviCube()
{
    delete naviCubeImplementation;
}

void NaviCube::createContextMenu(const std::vector<std::string>& cmd)
{
    naviCubeImplementation->createContextMenu(cmd);
}

bool NaviCube::processSoEvent(const SoEvent* ev)
{
    return naviCubeImplementation->processSoEvent(ev);
}

vector<string> NaviCubeImplementation::commands;

void NaviCube::setCorner(Corner c)
{
    naviCubeImplementation->moveToCorner(c);
}

void NaviCube::setOffset(int x, int y)
{
    naviCubeImplementation->posOffset = SbVec2s(x, y);
    naviCubeImplementation->viewSize = SbVec2s(0, 0);
    naviCubeImplementation->requestRedraw();
}

bool NaviCube::isDraggable()
{
    return naviCubeImplementation->draggable;
}

void NaviCube::setDraggable(bool draggable)
{
    naviCubeImplementation->draggable = draggable;
}

void NaviCube::setSize(int size)
{
    naviCubeImplementation->setSize(size);
}

void NaviCube::setChamfer(float chamfer)
{
    naviCubeImplementation->chamfer = min(max(0.05f, chamfer), 0.18f);
    naviCubeImplementation->prepared = false;
    naviCubeImplementation->requestRedraw();
}

void NaviCube::setNaviRotateToNearest(bool toNearest)
{
    naviCubeImplementation->rotateToNearest = toNearest;
}

void NaviCube::setNaviStepByTurn(int steps)
{
    naviCubeImplementation->naviStepByTurn = steps;
}

void NaviCube::setFont(std::string font)
{
    naviCubeImplementation->textFont = font;
    naviCubeImplementation->prepared = false;
    naviCubeImplementation->requestRedraw();
}

void NaviCube::setFontWeight(int weight)
{
    naviCubeImplementation->fontWeight = weight;
    naviCubeImplementation->prepared = false;
    naviCubeImplementation->requestRedraw();
}

void NaviCube::setFontStretch(int stretch)
{
    naviCubeImplementation->fontStretch = stretch;
    naviCubeImplementation->prepared = false;
    naviCubeImplementation->requestRedraw();
}

void NaviCube::setFontZoom(float zoom)
{
    naviCubeImplementation->fontZoom = zoom;
    naviCubeImplementation->prepared = false;
    naviCubeImplementation->requestRedraw();
}

void NaviCube::setBaseColor(QColor bColor)
{
    naviCubeImplementation->baseColor = bColor;
    naviCubeImplementation->requestRedraw();
}

void NaviCube::setEmphaseColor(QColor eColor)
{
    naviCubeImplementation->emphaseColor = eColor;
    naviCubeImplementation->prepared = false;
    naviCubeImplementation->requestRedraw();
}

void NaviCube::setHiliteColor(QColor HiliteColor)
{
    naviCubeImplementation->hiliteColor = HiliteColor;
    naviCubeImplementation->requestRedraw();
}

void NaviCube::setBorderWidth(double BorderWidth)
{
    naviCubeImplementation->borderWidth = BorderWidth;
    naviCubeImplementation->requestRedraw();
}

void NaviCube::setShowCS(bool showCS)
{
    naviCubeImplementation->showCS = showCS;
    naviCubeImplementation->requestRedraw();
}

void NaviCube::setNaviCubeLabels(const std::vector<std::string>& labels)
{
    naviCubeImplementation->setLabels(labels);
}

void NaviCube::setInactiveOpacity(float opacity)
{
    naviCubeImplementation->inactiveOpacity = opacity;
    naviCubeImplementation->requestRedraw();
}

qreal NaviCubeImplementation::getPhysicalCubeWidgetSize()
{
    return cubeWidgetSize * devicePixelRatio;
}

void NaviCubeImplementation::setLabels(const std::vector<std::string>& labels)
{
    labelTextures[PickId::Front].label = labels[0];
    labelTextures[PickId::Top].label = labels[1];
    labelTextures[PickId::Right].label = labels[2];
    labelTextures[PickId::Rear].label = labels[3];
    labelTextures[PickId::Bottom].label = labels[4];
    labelTextures[PickId::Left].label = labels[5];
    prepared = false;
    requestRedraw();
}

NaviCubeImplementation::NaviCubeImplementation(Gui::View3DInventorViewer* viewer)
    : baseColor {226, 232, 239}
    , emphaseColor {0, 0, 0}
    , hiliteColor {170, 226, 255}
{
    soNaviCube = new Gui::SoNaviCube();
    soNaviCube->ref();

    coinRoot = new SoSeparator();
    coinRoot->ref();
    coinRoot->setName("naviCubeRoot");

    actionSync = new SoCallback();
    actionSync->setCallback(NaviCubeImplementation::traversalCallback, this);
    coinRoot->addChild(actionSync);
    coinRoot->addChild(soNaviCube);

    this->viewer = viewer;
    menu = createNaviCubeMenu();
}

NaviCubeImplementation::~NaviCubeImplementation()
{
    delete menu;
    if (coinRoot) {
        coinRoot->unref();
        coinRoot = nullptr;
    }
    actionSync = nullptr;
    if (soNaviCube) {
        soNaviCube->clearLabelTextures();
        soNaviCube->unref();
        soNaviCube = nullptr;
    }
}

SoNode* NaviCubeImplementation::getCoinNode() const
{
    return coinRoot;
}

void NaviCubeImplementation::traversalCallback(void* userdata, SoAction* action)
{
    if (!userdata) {
        return;
    }
    static_cast<NaviCubeImplementation*>(userdata)->syncNodeState(action);
}

void NaviCubeImplementation::syncNodeState(SoAction* action)
{
    if (!action) {
        return;
    }

    const SoType type = action->getTypeId();
    const bool isGLRender = type.isDerivedFrom(SoGLRenderAction::getClassTypeId());
    const bool isIRRender = type.isDerivedFrom(SoIRRenderAction::getClassTypeId());

    if (isGLRender || isIRRender) {
        if (!readyToRender()) {
            return;
        }
    }
    else if (!prepared) {
        return;
    }

    SoState* state = action->getState();
    if (!state) {
        return;
    }

    const SbViewportRegion& region = SoViewportRegionElement::get(state);
    const SbVec2s viewSize = region.getViewportSizePixels();
    if (viewSize[0] <= 0 || viewSize[1] <= 0) {
        return;
    }

    handleResize(viewSize);

    const qreal physicalCubeWidgetSize = getPhysicalCubeWidgetSize();
    if (physicalCubeWidgetSize <= 0) {
        return;
    }

    const int viewportWidth = static_cast<int>(physicalCubeWidgetSize);
    const int viewportHeight = static_cast<int>(physicalCubeWidgetSize);
    const int posX = static_cast<int>(relPos[0] * posAreaSize[0]) + posAreaBase[0]
        - viewportWidth / 2;
    const int posY = static_cast<int>(relPos[1] * posAreaSize[1]) + posAreaBase[1]
        - viewportHeight / 2;

    if (
        !populateRenderParams(hovering ? 1.0F : inactiveOpacity, posX, posY, viewportWidth, viewportHeight)
    ) {
        return;
    }
}

void NaviCubeImplementation::requestRedraw()
{
    if (viewer) {
        if (auto* rm = viewer->getSoRenderManager()) {
            rm->invalidateForeground();
        }
    }
}

FaceType NaviCubeImplementation::getFaceType(PickId id) const
{
    switch (id) {
        default:
            return FaceType::None;
        case PickId::Front:
        case PickId::Top:
        case PickId::Right:
        case PickId::Rear:
        case PickId::Bottom:
        case PickId::Left:
            return FaceType::Main;
        case PickId::FrontTop:
        case PickId::FrontBottom:
        case PickId::FrontRight:
        case PickId::FrontLeft:
        case PickId::RearTop:
        case PickId::RearBottom:
        case PickId::RearRight:
        case PickId::RearLeft:
        case PickId::TopRight:
        case PickId::TopLeft:
        case PickId::BottomRight:
        case PickId::BottomLeft:
            return FaceType::Edge;
        case PickId::FrontTopRight:
        case PickId::FrontTopLeft:
        case PickId::FrontBottomRight:
        case PickId::FrontBottomLeft:
        case PickId::RearTopRight:
        case PickId::RearTopLeft:
        case PickId::RearBottomRight:
        case PickId::RearBottomLeft:
            return FaceType::Corner;
        case PickId::ArrowNorth:
        case PickId::ArrowSouth:
        case PickId::ArrowEast:
        case PickId::ArrowWest:
        case PickId::ArrowRight:
        case PickId::ArrowLeft:
        case PickId::Backside:
        case PickId::Home:
        case PickId::ViewMenu:
            return FaceType::Button;
    }
}

SbRotation NaviCubeImplementation::getFaceRotation(PickId id) const
{
    const auto makeFaceRotation = [](SbVec3f x, SbVec3f z, float rotZ) {
        SbVec3f y = x.cross(-z);
        x.normalize();
        y.normalize();
        z.normalize();

        SbMatrix R(x[0], y[0], z[0], 0, x[1], y[1], z[1], 0, x[2], y[2], z[2], 0, 0, 0, 0, 1);
        return (SbRotation(R) * SbRotation(SbVec3f(0, 0, 1), rotZ)).inverse();
    };

    const SbVec3f x(1.0F, 0.0F, 0.0F);
    const SbVec3f y(0.0F, 1.0F, 0.0F);
    const SbVec3f z(0.0F, 0.0F, 1.0F);
    constexpr float pi = std::numbers::pi_v<float>;

    switch (id) {
        default:
            return SbRotation();

        // Main faces
        case PickId::Top:
            return makeFaceRotation(x, z, 0.0F);
        case PickId::Front:
            return makeFaceRotation(x, -y, 0.0F);
        case PickId::Left:
            return makeFaceRotation(-y, -x, 0.0F);
        case PickId::Rear:
            return makeFaceRotation(-x, y, 0.0F);
        case PickId::Right:
            return makeFaceRotation(y, x, 0.0F);
        case PickId::Bottom:
            return makeFaceRotation(x, -z, 0.0F);

        // Corner faces
        case PickId::FrontTopRight:
            return makeFaceRotation(-x - y, x - y + z, pi);
        case PickId::FrontTopLeft:
            return makeFaceRotation(-x + y, -x - y + z, pi);
        case PickId::FrontBottomRight:
            return makeFaceRotation(x + y, x - y - z, 0.0F);
        case PickId::FrontBottomLeft:
            return makeFaceRotation(x - y, -x - y - z, 0.0F);
        case PickId::RearTopRight:
            return makeFaceRotation(x - y, x + y + z, pi);
        case PickId::RearTopLeft:
            return makeFaceRotation(x + y, -x + y + z, pi);
        case PickId::RearBottomRight:
            return makeFaceRotation(-x + y, x + y - z, 0.0F);
        case PickId::RearBottomLeft:
            return makeFaceRotation(-x - y, -x + y - z, 0.0F);

        // Edge faces
        case PickId::FrontTop:
            return makeFaceRotation(x, z - y, 0.0F);
        case PickId::FrontBottom:
            return makeFaceRotation(x, -z - y, 0.0F);
        case PickId::RearBottom:
            return makeFaceRotation(x, y - z, pi);
        case PickId::RearTop:
            return makeFaceRotation(x, y + z, pi);
        case PickId::RearRight:
            return makeFaceRotation(z, x + y, pi / 2.0F);
        case PickId::FrontRight:
            return makeFaceRotation(z, x - y, pi / 2.0F);
        case PickId::FrontLeft:
            return makeFaceRotation(z, -x - y, pi / 2.0F);
        case PickId::RearLeft:
            return makeFaceRotation(z, y - x, pi / 2.0F);
        case PickId::TopLeft:
            return makeFaceRotation(y, z - x, pi);
        case PickId::TopRight:
            return makeFaceRotation(y, x + z, 0.0F);
        case PickId::BottomRight:
            return makeFaceRotation(y, x - z, 0.0F);
        case PickId::BottomLeft:
            return makeFaceRotation(y, -z - x, pi);

        // Buttons (axis rotations; angle is scaled by caller)
        case PickId::ArrowNorth:
            return SbRotation(SbVec3f(-1, 0, 0), 1).inverse();
        case PickId::ArrowSouth:
            return SbRotation(SbVec3f(1, 0, 0), 1).inverse();
        case PickId::ArrowEast:
            return SbRotation(SbVec3f(0, 1, 0), 1).inverse();
        case PickId::ArrowWest:
            return SbRotation(SbVec3f(0, -1, 0), 1).inverse();
        case PickId::ArrowLeft:
            return SbRotation(SbVec3f(0, 0, 1), 1).inverse();
        case PickId::ArrowRight:
            return SbRotation(SbVec3f(0, 0, -1), 1).inverse();
        case PickId::Backside:
            return SbRotation(SbVec3f(0, 1, 0), 1).inverse();
        case PickId::Home:
        case PickId::ViewMenu:
            return SbRotation();
    }
}

void NaviCubeImplementation::moveToCorner(NaviCube::Corner c)
{
    if (c == NaviCube::TopLeftCorner) {
        relPos = SbVec2f(0.0f, 1.0f);
    }
    else if (c == NaviCube::TopRightCorner) {
        relPos = SbVec2f(1.0f, 1.0f);
    }
    else if (c == NaviCube::BottomLeftCorner) {
        relPos = SbVec2f(0.0f, 0.0f);
    }
    else if (c == NaviCube::BottomRightCorner) {
        relPos = SbVec2f(1.0f, 0.0f);
    }
    requestRedraw();
}

auto convertWeights = [](int weight) -> QFont::Weight {
    if (weight >= 87) {
        return QFont::Black;
    }
    if (weight >= 81) {
        return QFont::ExtraBold;
    }
    if (weight >= 75) {
        return QFont::Bold;
    }
    if (weight >= 63) {
        return QFont::DemiBold;
    }
    if (weight >= 57) {
        return QFont::Medium;
    }
    if (weight >= 50) {
        return QFont::Normal;
    }
    if (weight >= 25) {
        return QFont::Light;
    }
    if (weight >= 12) {
        return QFont::ExtraLight;
    }
    return QFont::Thin;
};

int imageVerticalBalance(QImage p, int sizeHint)
{
    if (sizeHint < 0) {
        return 0;
    }

    int h = p.height();
    int startRow = (h - sizeHint) / 2;
    bool done = false;
    int x, bottom, top;
    for (top = startRow; top < h; top++) {
        for (x = 0; x < p.width(); x++) {
            if (qAlpha(p.pixel(x, top))) {
                done = true;
                break;
            }
        }
        if (done) {
            break;
        }
    }
    for (bottom = startRow; bottom < h; bottom++) {
        for (x = 0; x < p.width(); x++) {
            if (qAlpha(p.pixel(x, h - 1 - bottom))) {
                return (bottom - top) / 2;
            }
        }
    }
    return 0;
}

void NaviCubeImplementation::createCubeFaceTextures()
{
    int texSize = 192;  // Works well for the max cube size 1024
    QFont font;
    if (textFont.empty()) {
        font.fromString(QStringLiteral("Arial"));
    }
    else {
        font.fromString(QString::fromStdString(textFont));
    }
    font.setStyleHint(QFont::SansSerif);
    if (fontWeight > 0) {
        font.setWeight(convertWeights(fontWeight));
    }
    if (fontStretch > 0) {
        font.setStretch(fontStretch);
    }
    font.setPointSizeF(texSize);
    QFontMetrics fm(font);
    qreal minFontSize = texSize;
    qreal maxFontSize = 0.;
    vector<PickId> mains
        = {PickId::Front, PickId::Top, PickId::Right, PickId::Rear, PickId::Bottom, PickId::Left};
    for (PickId pickId : mains) {
        auto t = QString::fromUtf8(labelTextures[pickId].label.c_str());
        QRect br = fm.boundingRect(t);
        float scale = (float)texSize / max(br.width(), br.height());
        labelTextures[pickId].fontSize = texSize * scale;
        minFontSize = std::min(minFontSize, labelTextures[pickId].fontSize);
        maxFontSize = std::max(maxFontSize, labelTextures[pickId].fontSize);
    }
    if (fontZoom > 0.0) {
        maxFontSize = minFontSize + (maxFontSize - minFontSize) * fontZoom;
    }
    else {
        maxFontSize = minFontSize * std::pow(2.0, fontZoom);
    }
    for (PickId pickId : mains) {
        QImage image(texSize, texSize, QImage::Format_ARGB32);
        image.fill(qRgba(255, 255, 255, 0));
        if (labelTextures[pickId].fontSize > 0.5) {
            // 5% margin looks nice and prevents some artifacts
            font.setPointSizeF(std::min(labelTextures[pickId].fontSize, maxFontSize) * 0.9);
            QPainter paint;
            paint.begin(&image);
            paint.setRenderHints(
                QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform
            );
            paint.setPen(Qt::white);
            QString text = QString::fromUtf8(labelTextures[pickId].label.c_str());
            paint.setFont(font);
            paint.drawText(QRect(0, 0, texSize, texSize), Qt::AlignCenter, text);
            int offset = imageVerticalBalance(image, font.pointSize());
            image.fill(qRgba(255, 255, 255, 0));
            paint.drawText(QRect(0, offset, texSize, texSize), Qt::AlignCenter, text);
            paint.end();
        }

        // Coin backend: store as a Coin-managed texture (SoTexture2).
        // Mirror to match the OpenGL texture coordinate origin (bottom-left).
#if QT_VERSION < QT_VERSION_CHECK(6, 9, 0)
        const QImage rgba = image.mirrored().convertToFormat(QImage::Format_RGBA8888);
#else
        const QImage rgba = image.flipped(Qt::Vertical).convertToFormat(QImage::Format_RGBA8888);
#endif
        const int w = rgba.width();
        const int h = rgba.height();
        std::vector<unsigned char> pixels(static_cast<size_t>(w) * static_cast<size_t>(h) * 4U);
        for (int y = 0; y < h; ++y) {
            const unsigned char* src = rgba.constScanLine(y);
            unsigned char* dst = pixels.data() + static_cast<size_t>(y) * static_cast<size_t>(w) * 4U;
            std::memcpy(dst, src, static_cast<size_t>(w) * 4U);
        }

        soNaviCube->setLabelImage(pickId, SbVec2s(w, h), 4, pixels.data());
    }
}

void NaviCubeImplementation::setSize(int size)
{
    cubeWidgetSize = size;
    viewSize = SbVec2s(0, 0);
    prepared = false;
    requestRedraw();
}

void NaviCubeImplementation::prepare()
{
    if (prepared || !viewer->viewport()) {
        return;
    }

    soNaviCube->setChamfer(chamfer);

    createCubeFaceTextures();
    prepared = true;
    requestRedraw();
}

bool NaviCubeImplementation::readyToRender()
{
    prepare();
    return prepared;
}

bool NaviCubeImplementation::populateRenderParams(
    float opacity,
    int viewportX,
    int viewportY,
    int viewportWidth,
    int viewportHeight
)
{
    SoCamera* cam = viewer->getSoRenderManager()->getCamera();
    if (!cam) {
        return false;
    }

    soNaviCube->size = static_cast<float>(cubeWidgetSize);
    soNaviCube->opacity = opacity;
    soNaviCube->borderWidth = static_cast<float>(borderWidth);
    soNaviCube->showCoordinateSystem = showCS;
    soNaviCube->hiliteId = static_cast<int>(hiliteId);
    soNaviCube->baseColor.setValue(baseColor.redF(), baseColor.greenF(), baseColor.blueF());
    soNaviCube->baseAlpha = static_cast<float>(baseColor.alphaF());
    soNaviCube->emphaseColor.setValue(emphaseColor.redF(), emphaseColor.greenF(), emphaseColor.blueF());
    soNaviCube->emphaseAlpha = static_cast<float>(emphaseColor.alphaF());
    soNaviCube->hiliteColor.setValue(hiliteColor.redF(), hiliteColor.greenF(), hiliteColor.blueF());
    soNaviCube->hiliteAlpha = static_cast<float>(hiliteColor.alphaF());
    soNaviCube->axisXColor.setValue(
        static_cast<float>(xColor.r),
        static_cast<float>(xColor.g),
        static_cast<float>(xColor.b)
    );
    soNaviCube->axisYColor.setValue(
        static_cast<float>(yColor.r),
        static_cast<float>(yColor.g),
        static_cast<float>(yColor.b)
    );
    soNaviCube->axisZColor.setValue(
        static_cast<float>(zColor.r),
        static_cast<float>(zColor.g),
        static_cast<float>(zColor.b)
    );

    SbVec4f rect(
        static_cast<float>(viewportX),
        static_cast<float>(viewportY),
        static_cast<float>(viewportWidth),
        static_cast<float>(viewportHeight)
    );
    soNaviCube->viewportRect = rect;
    soNaviCube->cameraOrientation = cam->orientation.getValue();
    soNaviCube->cameraIsOrthographic = cam->getTypeId().isDerivedFrom(
        SoOrthographicCamera::getClassTypeId()
    );

    return true;
}

void NaviCubeImplementation::createContextMenu(const std::vector<std::string>& cmd)
{
    CommandManager& rcCmdMgr = Application::Instance->commandManager();
    menu->clear();

    for (const auto& i : cmd) {
        Command* cmd = rcCmdMgr.getCommandByName(i.c_str());
        if (cmd) {
            cmd->addTo(menu);
        }
    }
}

void NaviCubeImplementation::handleResize(const SbVec2s& viewSize)
{
    qreal currentDevicePixelRatio = viewer->devicePixelRatio();
    if (viewSize != this->viewSize || currentDevicePixelRatio != devicePixelRatio) {
        devicePixelRatio = currentDevicePixelRatio;
        qreal physicalCubeWidgetSize = getPhysicalCubeWidgetSize();
        posAreaBase[0] = std::min((int)(posOffset[0] + physicalCubeWidgetSize * 0.55), viewSize[0] / 2);
        posAreaBase[1] = std::min((int)(posOffset[1] + physicalCubeWidgetSize * 0.55), viewSize[1] / 2);
        posAreaSize[0] = viewSize[0] - 2 * posAreaBase[0];
        posAreaSize[1] = viewSize[1] - 2 * posAreaBase[1];
        this->viewSize = viewSize;
    }
}

PickId NaviCubeImplementation::pickFace(short x, short y)
{
    if (!readyToRender()) {
        return PickId::None;
    }

    qreal physicalCubeWidgetSize = getPhysicalCubeWidgetSize();
    if (std::abs(x) > physicalCubeWidgetSize / 2 || std::abs(y) > physicalCubeWidgetSize / 2) {
        return PickId::None;
    }

    const int viewportSize = static_cast<int>(physicalCubeWidgetSize * 2.0);
    if (!populateRenderParams(1.0F, 0, 0, viewportSize, viewportSize)) {
        return PickId::None;
    }

    const int center = viewportSize / 2;
    const SbVec2s point(static_cast<short>(2 * x + center), static_cast<short>(2 * y + center));
    const PickId picked = soNaviCube->pickAt(point);

    return picked;
}

bool NaviCubeImplementation::mousePressed(short x, short y)
{
    PickId pick = pickFace(x, y);
    setHilite(pick);
    if (pick == PickId::None) {
        mouseDown = false;
        dragStarted = false;
        dragMode = MouseDragMode::None;
        return false;
    }

    mouseDown = true;
    dragStarted = false;
    pressPos = SbVec2s(x, y);
    lastDragPos = pressPos;

    const FaceType faceType = getFaceType(pick);
    const bool dragAllowed = faceType == FaceType::Main || faceType == FaceType::Edge
        || faceType == FaceType::Corner;
    dragMode = dragAllowed ? (draggable ? MouseDragMode::MoveNaviCube : MouseDragMode::RotateCamera)
                           : MouseDragMode::None;
    return true;
}

void NaviCubeImplementation::handleMenu()
{
    menu->exec(QCursor::pos());
}

void NaviCubeImplementation::resetClickState()
{
    lastClickPickId = PickId::None;
    clickTimer.invalidate();
}

SbRotation NaviCubeImplementation::getNearestOrientation(PickId pickId)
{
    SbRotation cameraOrientation = viewer->getCameraOrientation();
    SbRotation standardOrientation = getFaceRotation(pickId);

    SbVec3f cameraZ;
    cameraOrientation.multVec(SbVec3f(0, 0, 1), cameraZ);

    SbVec3f standardZ;
    standardOrientation.multVec(SbVec3f(0, 0, 1), standardZ);

    // Cleanup near zero values
    for (int i = 0; i < 3; i++) {
        if (abs(standardZ[i]) < 1e-6) {
            standardZ[i] = 0.0F;
        }
    }
    standardZ.normalize();

    // Rotate the camera to the selected face by the smallest angle to align the z-axis
    SbRotation intermediateOrientation = cameraOrientation * SbRotation(cameraZ, standardZ);

    // Find an axis and angle to go from the intermediateOrientation to the standardOrientation
    SbVec3f axis;
    float angle;
    SbRotation rotation = intermediateOrientation.inverse() * standardOrientation;
    rotation.getValue(axis, angle);

    // Make sure the found axis aligns with the standardZ axis
    if (standardZ.dot(axis) < 0) {
        axis.negate();
        angle *= -1;
    }

    constexpr float pi = std::numbers::pi_v<float>;

    // Make angle positive
    if (angle < 0) {
        angle += 2 * pi;
    }

    // f is a small value used to control orientation priority when the camera is almost exactly
    // between two orientations (e.g. +45 and -45 degrees). The standard orientation is preferred
    // compared to +90 and -90 degree orientations and the +90 and -90 degree orientations are
    // preferred compared to an upside down standard orientation
    float f = 0.00001F;
    FaceType faceType = getFaceType(pickId);

    // Find the angle to rotate to the nearest orientation
    if (faceType == FaceType::Corner) {
        // 6 possible orientations for the corners
        if (angle <= (pi / 6 + f)) {
            angle = 0;
        }
        else if (angle <= (pi / 2 + f)) {
            angle = pi / 3;
        }
        else if (angle < (5 * pi / 6 - f)) {
            angle = 2 * pi / 3;
        }
        else if (angle <= (pi + pi / 6 + f)) {
            angle = pi;
        }
        else if (angle < (pi + pi / 2 - f)) {
            angle = pi + pi / 3;
        }
        else if (angle < (pi + 5 * pi / 6 - f)) {
            angle = pi + 2 * pi / 3;
        }
        else {
            angle = 0;
        }
    }
    else {
        // 4 possible orientations for the main and edge faces
        if (angle <= (pi / 4 + f)) {
            angle = 0;
        }
        else if (angle <= (3 * pi / 4 + f)) {
            angle = pi / 2;
        }
        else if (angle < (pi + pi / 4 - f)) {
            angle = pi;
        }
        else if (angle < (pi + 3 * pi / 4 - f)) {
            angle = pi + pi / 2;
        }
        else {
            angle = 0;
        }
    }

    // Set the rotation to go from the standard orientation to the nearest orientation
    rotation.setValue(standardZ, angle);

    return standardOrientation * rotation.inverse();
}

bool NaviCubeImplementation::mouseReleased(short x, short y)
{
    static const float pi = boost::math::constants::pi<float>();

    setHilite(PickId::None);
    if (!mouseDown) {
        return false;
    }

    const bool wasDragging = dragStarted;
    const MouseDragMode releasedDragMode = dragMode;
    mouseDown = false;
    dragStarted = false;
    dragMode = MouseDragMode::None;

    if (wasDragging) {
        if (releasedDragMode == MouseDragMode::RotateCamera) {
            if (auto* navigation = viewer->navigationStyle()) {
                navigation->endOrbitDrag();
            }
        }
        resetClickState();
    }
    else {
        PickId pickId = pickFace(x, y);
        long step = Base::clamp(long(naviStepByTurn), 4L, 36L);
        float rotStepAngle = (2 * std::numbers::pi) / step;

        FaceType faceType = getFaceType(pickId);
        if (faceType == FaceType::Main || faceType == FaceType::Edge || faceType == FaceType::Corner) {
            // FIXME: Quarter currently flattens Qt double-clicks into ordinary Coin button
            // presses, so NaviCube has to detect the recenter gesture locally.
            bool moveToCenter = clickTimer.isValid() && lastClickPickId == pickId
                && clickTimer.elapsed() < QApplication::doubleClickInterval();

            // Handle the cube faces
            SbRotation orientation;
            if (rotateToNearest) {
                orientation = getNearestOrientation(pickId);
            }
            else {
                orientation = getFaceRotation(pickId);
            }
            viewer->setCameraOrientation(orientation, moveToCenter);

            if (moveToCenter) {
                resetClickState();
            }
            else {
                lastClickPickId = pickId;
                clickTimer.start();
            }
            setHilite(PickId::None);
        }
        else if (faceType == FaceType::Button) {
            // Handle the menu
            if (pickId == PickId::ViewMenu) {
                resetClickState();
                setHilite(PickId::None);
                handleMenu();
                return true;
            }
            else if (pickId == PickId::Home) {
                resetClickState();
                viewer->viewHome();
                setHilite(pickId);
                return true;
            }

            // Handle the flat buttons
            resetClickState();
            SbRotation rotation = getFaceRotation(pickId);
            if (pickId == PickId::Backside) {
                rotation.scaleAngle(pi);
            }
            else {
                rotation.scaleAngle(rotStepAngle);
            }

            // If the previous flat button animation is still active then apply the rotation to the
            // previous target orientation, otherwise apply the rotation to the current camera
            // orientation
            if (flatButtonAnimation != nullptr
                && flatButtonAnimation->state() == QAbstractAnimation::Running) {
                flatButtonTargetOrientation = rotation * flatButtonTargetOrientation;
            }
            else {
                flatButtonTargetOrientation = rotation * viewer->getCameraOrientation();
            }

            flatButtonAnimation = viewer->setCameraOrientation(flatButtonTargetOrientation);
            setHilite(pickId);
        }
        else {
            setHilite(PickId::None);
            resetClickState();
            return true;
        }
    }
    return true;
}

void NaviCubeImplementation::setHilite(PickId hilite)
{
    if (hilite != hiliteId) {
        hiliteId = hilite;
        if (soNaviCube) {
            soNaviCube->hiliteId = static_cast<int>(hiliteId);
        }
        requestRedraw();
    }
}

bool NaviCubeImplementation::hasDraggedPastThreshold(short x, short y) const
{
    const long dx = x - pressPos[0];
    const long dy = y - pressPos[1];
    constexpr long minimumDragThreshold = 3;
    const long threshold
        = std::max(minimumDragThreshold, static_cast<long>(std::lround(3.0 * devicePixelRatio)));
    return dx * dx + dy * dy >= threshold * threshold;
}

void NaviCubeImplementation::startCameraRotationDrag()
{
    constexpr float minCameraDistanceFactor = 1.05F;
    constexpr float clippingRadiusFactor = 1.0F;
    constexpr float dragSensitivity = 0.45F;

    dragStarted = true;
    setHilite(PickId::None);

    if (auto* navigation = viewer->navigationStyle()) {
        NavigationStyle::OrbitDragOptions options;
        options.centerMode = NavigationStyle::OrbitDragOptions::CenterMode::SceneBoundingSphere;
        options.minDistanceFactor = minCameraDistanceFactor;
        options.clippingRadiusFactor = clippingRadiusFactor;
        options.sensitivity = dragSensitivity;
        navigation->beginOrbitDrag(options);
    }
}

void NaviCubeImplementation::updateCameraRotationDrag(short x, short y)
{
    const int dx = x - lastDragPos[0];
    const int dy = y - lastDragPos[1];
    if (dx == 0 && dy == 0) {
        return;
    }

    auto* navigation = viewer->navigationStyle();
    const qreal physicalCubeWidgetSize = getPhysicalCubeWidgetSize();
    if (!navigation || physicalCubeWidgetSize <= 0) {
        lastDragPos = SbVec2s(x, y);
        return;
    }

    const auto toProjectorPos = [physicalCubeWidgetSize](short value) {
        return 0.5F + static_cast<float>(value) / static_cast<float>(physicalCubeWidgetSize);
    };
    const SbVec2f curpos(toProjectorPos(x), toProjectorPos(y));
    const SbVec2f prevpos(toProjectorPos(lastDragPos[0]), toProjectorPos(lastDragPos[1]));
    navigation->updateOrbitDrag(curpos, prevpos);

    lastDragPos = SbVec2s(x, y);
    requestRedraw();
}

bool NaviCubeImplementation::mouseMoved(short x, short y)
{
    qreal physicalCubeWidgetSize = getPhysicalCubeWidgetSize();
    bool hovering = mouseDown
        || (std::abs(x) <= physicalCubeWidgetSize / 2 && std::abs(y) <= physicalCubeWidgetSize / 2);

    if (hovering != this->hovering) {
        this->hovering = hovering;
        requestRedraw();
    }

    if (!dragStarted) {
        PickId pick = pickFace(x, y);
        if (!mouseDown) {
            setHiliteWithHysteresis(pick);
        }
        else {
            setHilite(pick);
        }
    }

    if (mouseDown && dragMode == MouseDragMode::MoveNaviCube) {
        const int dx = x - pressPos[0];
        const int dy = y - pressPos[1];
        if (!dragStarted && hasDraggedPastThreshold(x, y)) {
            dragStarted = true;
            setHilite(PickId::None);
        }
        if (dragStarted && (dx != 0 || dy != 0)) {
            float newX = relPos[0] + static_cast<float>(dx) / posAreaSize[0];
            float newY = relPos[1] + static_cast<float>(dy) / posAreaSize[1];
            relPos[0] = std::min(std::max(newX, 0.0f), 1.0f);
            relPos[1] = std::min(std::max(newY, 0.0f), 1.0f);

            requestRedraw();
        }
        return true;
    }

    if (mouseDown && dragMode == MouseDragMode::RotateCamera) {
        if (!dragStarted && hasDraggedPastThreshold(x, y)) {
            startCameraRotationDrag();
        }
        if (dragStarted) {
            updateCameraRotationDrag(x, y);
        }
        return true;
    }

    // Button controls do not start drags, but NaviCube still owns the mouse sequence until release.
    return mouseDown;
}

void NaviCubeImplementation::setHiliteWithHysteresis(PickId hilite)
{
    static const int stableFrames = []() -> int {
        const char* env = std::getenv("FREECAD_NAVICUBE_PICK_STABLE_FRAMES");
        if (!env || !*env) {
            return 0;
        }
        char* end = nullptr;
        long v = std::strtol(env, &end, 10);
        if (end == env) {
            return 0;
        }
        if (v < 0) {
            v = 0;
        }
        if (v > 16) {
            v = 16;
        }
        return static_cast<int>(v);
    }();

    if (stableFrames <= 1) {
        setHilite(hilite);
        return;
    }

    if (hilite == PickId::None) {
        pendingHiliteId = PickId::None;
        pendingHiliteCount = 0;
        setHilite(PickId::None);
        return;
    }

    if (hilite == hiliteId) {
        pendingHiliteId = PickId::None;
        pendingHiliteCount = 0;
        return;
    }

    if (hilite != pendingHiliteId) {
        pendingHiliteId = hilite;
        pendingHiliteCount = 1;
        return;
    }

    ++pendingHiliteCount;
    if (pendingHiliteCount >= stableFrames) {
        pendingHiliteId = PickId::None;
        pendingHiliteCount = 0;
        setHilite(hilite);
    }
}

bool NaviCubeImplementation::processSoEvent(const SoEvent* ev)
{
    short x, y;
    ev->getPosition().getValue(x, y);
    // translate to internal cube center based coordinates
    short rx = x - (short)(posAreaSize[0] * relPos[0]) - posAreaBase[0];
    short ry = y - (short)(posAreaSize[1] * relPos[1]) - posAreaBase[1];
    if (ev->getTypeId().isDerivedFrom(SoMouseButtonEvent::getClassTypeId())) {
        const auto mbev = static_cast<const SoMouseButtonEvent*>(ev);
        if (mbev->isButtonPressEvent(mbev, SoMouseButtonEvent::BUTTON1)) {
            return mousePressed(rx, ry);
        }
        if (mbev->isButtonReleaseEvent(mbev, SoMouseButtonEvent::BUTTON1)) {
            return mouseReleased(rx, ry);
        }
    }
    if (ev->getTypeId().isDerivedFrom(SoLocation2Event::getClassTypeId())) {
        return mouseMoved(rx, ry);
    }
    return false;
}

QString NaviCubeImplementation::str(const char* str)
{
    return QString::fromLatin1(str);
}

void NaviCube::updateColors()
{
    unsigned long colorLong;

    colorLong = Gui::ViewParams::instance()->getAxisXColor();
    naviCubeImplementation->xColor = Base::Color(static_cast<uint32_t>(colorLong));
    colorLong = Gui::ViewParams::instance()->getAxisYColor();
    naviCubeImplementation->yColor = Base::Color(static_cast<uint32_t>(colorLong));
    colorLong = Gui::ViewParams::instance()->getAxisZColor();
    naviCubeImplementation->zColor = Base::Color(static_cast<uint32_t>(colorLong));
    naviCubeImplementation->requestRedraw();
}

void NaviCube::setNaviCubeCommands(const std::vector<std::string>& cmd)
{
    NaviCubeImplementation::commands = cmd;
}

DEF_STD_CMD_AC(NaviCubeDraggableCmd)

NaviCubeDraggableCmd::NaviCubeDraggableCmd()
    : Command("NaviCubeDraggableCmd")
{
    sGroup = "";
    sMenuText = QT_TR_NOOP("Movable Navigation Cube");
    sToolTipText = QT_TR_NOOP("Drags and places the NaviCube");
    sWhatsThis = "";
    sStatusTip = sToolTipText;
    eType = Alter3DView;
}
void NaviCubeDraggableCmd::activated(int iMsg)
{
    auto view = qobject_cast<View3DInventor*>(getMainWindow()->activeWindow());
    view->getViewer()->getNaviCube()->setDraggable(iMsg == 1 ? true : false);
}
bool NaviCubeDraggableCmd::isActive()
{
    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom<Gui::View3DInventor>()) {
        bool check = _pcAction->isChecked();
        auto view = qobject_cast<View3DInventor*>(getMainWindow()->activeWindow());
        bool mode = view->getViewer()->getNaviCube()->isDraggable();
        if (mode != check) {
            _pcAction->setChecked(mode);
        }
        return true;
    }
    return false;
}
Gui::Action* NaviCubeDraggableCmd::createAction()
{
    Gui::Action* pcAction = Command::createAction();
    pcAction->setCheckable(true);
    return pcAction;
}


QMenu* NaviCubeImplementation::createNaviCubeMenu()
{
    auto menu = new QMenu(getMainWindow());
    menu->setObjectName(str("NaviCube_Menu"));

    CommandManager& rcCmdMgr = Application::Instance->commandManager();
    static bool init = true;
    if (init) {
        init = false;
        rcCmdMgr.addCommand(new NaviCubeDraggableCmd);
    }

    vector<string> menuCommands = NaviCubeImplementation::commands;
    if (menuCommands.empty()) {
        menuCommands.emplace_back("Std_OrthographicCamera");
        menuCommands.emplace_back("Std_PerspectiveCamera");
        menuCommands.emplace_back("Std_ViewIsometric");
        menuCommands.emplace_back("Separator");
        menuCommands.emplace_back("Std_ViewFitAll");
        menuCommands.emplace_back("Std_ViewFitSelection");
        menuCommands.emplace_back("Std_AlignToSelection");
        menuCommands.emplace_back("Separator");
        menuCommands.emplace_back("NaviCubeDraggableCmd");
    }

    for (const auto& command : menuCommands) {
        if (command == "Separator") {
            menu->addSeparator();
        }
        else {
            Command* cmd = rcCmdMgr.getCommandByName(command.c_str());
            if (cmd) {
                cmd->addTo(menu);
            }
        }
    }
    return menu;
}
