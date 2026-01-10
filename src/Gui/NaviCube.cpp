/***************************************************************************
 *   Copyright (c) 2017 Kustaa Nyholm  <kustaa.nyholm@sparetimelabs.com>   *
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

#include <FCConfig.h>

#include <algorithm>
#include <numbers>
#include <mutex>
#include <memory>
#ifdef FC_OS_WIN32
# include <windows.h>
#endif
#ifdef FC_OS_MACOSX
# include <OpenGL/gl.h>
#else
# include <GL/gl.h>
#endif
#include <boost/math/constants/constants.hpp>
#include <Inventor/actions/SoAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/nodes/SoCallback.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbVec4f.h>
#include <Inventor/events/SoEvent.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <QApplication>
#include <QCursor>
#include <QImage>
#include <QMenu>
#include <QOpenGLFramebufferObject>
#include <QOpenGLTexture>
#include <QOpenGLWidget>
#include <QPainterPath>

#include <Base/Color.h>
#include <Base/Tools.h>
#include "NaviCube.h"
#include "Application.h"
#include "Command.h"
#include "Action.h"
#include "MainWindow.h"
#include "Navigation/NavigationAnimation.h"
#include "Inventor/SoNaviCube.h"
#include "View3DInventorViewer.h"
#include "View3DInventor.h"
#include "ViewParams.h"


using namespace std;
using namespace Gui;

using PickId = Gui::SoNaviCube::PickId;
using FaceType = Gui::SoNaviCube::FaceType;
using Face = Gui::SoNaviCube::Face;

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
    void requestRedraw(bool touchNode = true);

private:
    struct LabelTexture
    {
        qreal fontSize = 0.0;
        QOpenGLTexture* texture = nullptr;
        string label;
    };
    bool mousePressed(short x, short y);
    bool mouseReleased(short x, short y);
    bool mouseMoved(short x, short y);
    PickId pickFace(short x, short y);
    bool inDragZone(short x, short y);

    void prepare();
    void handleResize(const SbVec2s& viewSize);
    void handleMenu();

    void setHilite(PickId);

    const Face* findFace(PickId) const;
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
    bool isFramebufferValid() const;
    void ensureFramebufferValid();

    SbRotation getNearestOrientation(PickId pickId);
    qreal getPhysicalCubeWidgetSize();

public:
    static int m_CubeWidgetSize;
    QColor m_BaseColor;
    QColor m_EmphaseColor;
    QColor m_HiliteColor;
    bool m_ShowCS = true;
    PickId m_HiliteId = PickId::None;
    double m_BorderWidth = 1.1;
    bool m_RotateToNearest = true;
    int m_NaviStepByTurn = 8;
    float m_FontZoom = 0.3F;
    float m_Chamfer = 0.12F;
    std::string m_TextFont;
    int m_FontWeight = 0;
    int m_FontStretch = 0;
    float m_InactiveOpacity = 0.5;
    SbVec2s m_PosOffset = SbVec2s(0, 0);

    Base::Color m_xColor;
    Base::Color m_yColor;
    Base::Color m_zColor;

    bool m_Prepared = false;
    static vector<string> m_commands;
    bool m_Draggable = false;
    SbVec2s m_ViewSize = SbVec2s(0, 0);

private:
    bool m_MouseDown = false;
    bool m_Dragging = false;
    bool m_MightDrag = false;
    bool m_Hovering = false;

    SbVec2f m_RelPos = SbVec2f(1.0f, 1.0f);
    SbVec2s m_PosAreaBase = SbVec2s(0, 0);
    SbVec2s m_PosAreaSize = SbVec2s(0, 0);
    qreal m_DevicePixelRatio = 1.0;

    QOpenGLFramebufferObject* m_PickingFramebuffer;
    Gui::View3DInventorViewer* m_View3DInventorViewer;

    Gui::SoNaviCube* m_SoNaviCube = nullptr;

    map<PickId, LabelTexture> m_LabelTextures;

    QMenu* m_Menu;

    std::shared_ptr<NavigationAnimation> m_flatButtonAnimation;
    SbRotation m_flatButtonTargetOrientation;

    void syncNodeState(SoAction* action);
    static void traversalCallback(void* userdata, SoAction* action);

    SoSeparator* m_CoinRoot = nullptr;
    SoCallback* m_ActionSync = nullptr;
};

int NaviCubeImplementation::m_CubeWidgetSize = 132;

int NaviCube::getNaviCubeSize()
{
    return NaviCubeImplementation::m_CubeWidgetSize;
}

SoNode* NaviCube::getCoinNode() const
{
    return m_NaviCubeImplementation->getCoinNode();
}

NaviCube::NaviCube(Gui::View3DInventorViewer* viewer)
{
    m_NaviCubeImplementation = new NaviCubeImplementation(viewer);
    updateColors();
}

NaviCube::~NaviCube()
{
    delete m_NaviCubeImplementation;
}

void NaviCube::createContextMenu(const std::vector<std::string>& cmd)
{
    m_NaviCubeImplementation->createContextMenu(cmd);
}

bool NaviCube::processSoEvent(const SoEvent* ev)
{
    return m_NaviCubeImplementation->processSoEvent(ev);
}

vector<string> NaviCubeImplementation::m_commands;

void NaviCube::setCorner(Corner c)
{
    m_NaviCubeImplementation->moveToCorner(c);
}

void NaviCube::setOffset(int x, int y)
{
    m_NaviCubeImplementation->m_PosOffset = SbVec2s(x, y);
    m_NaviCubeImplementation->m_ViewSize = SbVec2s(0, 0);
    m_NaviCubeImplementation->requestRedraw();
}

bool NaviCube::isDraggable()
{
    return m_NaviCubeImplementation->m_Draggable;
}

void NaviCube::setDraggable(bool draggable)
{
    m_NaviCubeImplementation->m_Draggable = draggable;
}

void NaviCube::setSize(int size)
{
    m_NaviCubeImplementation->setSize(size);
}

void NaviCube::setChamfer(float chamfer)
{
    m_NaviCubeImplementation->m_Chamfer = min(max(0.05f, chamfer), 0.18f);
    m_NaviCubeImplementation->m_Prepared = false;
    m_NaviCubeImplementation->requestRedraw();
}

void NaviCube::setNaviRotateToNearest(bool toNearest)
{
    m_NaviCubeImplementation->m_RotateToNearest = toNearest;
}

void NaviCube::setNaviStepByTurn(int steps)
{
    m_NaviCubeImplementation->m_NaviStepByTurn = steps;
}

void NaviCube::setFont(std::string font)
{
    m_NaviCubeImplementation->m_TextFont = font;
    m_NaviCubeImplementation->m_Prepared = false;
    m_NaviCubeImplementation->requestRedraw();
}

void NaviCube::setFontWeight(int weight)
{
    m_NaviCubeImplementation->m_FontWeight = weight;
    m_NaviCubeImplementation->m_Prepared = false;
    m_NaviCubeImplementation->requestRedraw();
}

void NaviCube::setFontStretch(int stretch)
{
    m_NaviCubeImplementation->m_FontStretch = stretch;
    m_NaviCubeImplementation->m_Prepared = false;
    m_NaviCubeImplementation->requestRedraw();
}

void NaviCube::setFontZoom(float zoom)
{
    m_NaviCubeImplementation->m_FontZoom = zoom;
    m_NaviCubeImplementation->m_Prepared = false;
    m_NaviCubeImplementation->requestRedraw();
}

void NaviCube::setBaseColor(QColor bColor)
{
    m_NaviCubeImplementation->m_BaseColor = bColor;
    m_NaviCubeImplementation->requestRedraw();
}

void NaviCube::setEmphaseColor(QColor eColor)
{
    m_NaviCubeImplementation->m_EmphaseColor = eColor;
    m_NaviCubeImplementation->m_Prepared = false;
    m_NaviCubeImplementation->requestRedraw();
}

void NaviCube::setHiliteColor(QColor HiliteColor)
{
    m_NaviCubeImplementation->m_HiliteColor = HiliteColor;
    m_NaviCubeImplementation->requestRedraw();
}

void NaviCube::setBorderWidth(double BorderWidth)
{
    m_NaviCubeImplementation->m_BorderWidth = BorderWidth;
    m_NaviCubeImplementation->requestRedraw();
}

void NaviCube::setShowCS(bool showCS)
{
    m_NaviCubeImplementation->m_ShowCS = showCS;
    m_NaviCubeImplementation->requestRedraw();
}

void NaviCube::setNaviCubeLabels(const std::vector<std::string>& labels)
{
    m_NaviCubeImplementation->setLabels(labels);
}

void NaviCube::setInactiveOpacity(float opacity)
{
    m_NaviCubeImplementation->m_InactiveOpacity = opacity;
    m_NaviCubeImplementation->requestRedraw();
}

qreal NaviCubeImplementation::getPhysicalCubeWidgetSize()
{
    return m_CubeWidgetSize * m_DevicePixelRatio;
}

void NaviCubeImplementation::setLabels(const std::vector<std::string>& labels)
{
    m_LabelTextures[PickId::Front].label = labels[0];
    m_LabelTextures[PickId::Top].label = labels[1];
    m_LabelTextures[PickId::Right].label = labels[2];
    m_LabelTextures[PickId::Rear].label = labels[3];
    m_LabelTextures[PickId::Bottom].label = labels[4];
    m_LabelTextures[PickId::Left].label = labels[5];
    m_Prepared = false;
    requestRedraw();
}

NaviCubeImplementation::NaviCubeImplementation(Gui::View3DInventorViewer* viewer)
    : m_BaseColor {226, 232, 239}
    , m_HiliteColor {170, 226, 255}
{
    m_SoNaviCube = new Gui::SoNaviCube();
    m_SoNaviCube->ref();

    m_CoinRoot = new SoSeparator();
    m_CoinRoot->ref();
    m_CoinRoot->setName("naviCubeRoot");

    m_ActionSync = new SoCallback();
    m_ActionSync->setCallback(NaviCubeImplementation::traversalCallback, this);
    m_CoinRoot->addChild(m_ActionSync);
    m_CoinRoot->addChild(m_SoNaviCube);

    m_View3DInventorViewer = viewer;
    m_PickingFramebuffer = nullptr;
    m_Menu = createNaviCubeMenu();
}

NaviCubeImplementation::~NaviCubeImplementation()
{
    delete m_Menu;
    if (m_PickingFramebuffer) {
        delete m_PickingFramebuffer;
    }
    for (auto tex : m_LabelTextures) {
        delete tex.second.texture;
    }
    if (m_CoinRoot) {
        m_CoinRoot->unref();
        m_CoinRoot = nullptr;
    }
    m_ActionSync = nullptr;
    if (m_SoNaviCube) {
        m_SoNaviCube->clearLabelTextures();
        m_SoNaviCube->unref();
        m_SoNaviCube = nullptr;
    }
}

SoNode* NaviCubeImplementation::getCoinNode() const
{
    return m_CoinRoot;
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

    if (isGLRender) {
        if (!readyToRender()) {
            return;
        }
    }
    else if (!m_Prepared) {
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
    const int posX = static_cast<int>(m_RelPos[0] * m_PosAreaSize[0]) + m_PosAreaBase[0]
        - viewportWidth / 2;
    const int posY = static_cast<int>(m_RelPos[1] * m_PosAreaSize[1]) + m_PosAreaBase[1]
        - viewportHeight / 2;

    if (!populateRenderParams(
            m_Hovering ? 1.0F : m_InactiveOpacity,
            posX,
            posY,
            viewportWidth,
            viewportHeight
        )) {
        return;
    }

    if (!isGLRender) {
        m_SoNaviCube->touch();
    }
}

void NaviCubeImplementation::requestRedraw(bool touchNode)
{
    if (touchNode && m_SoNaviCube) {
        m_SoNaviCube->touch();
    }
    if (m_View3DInventorViewer) {
        if (auto* rm = m_View3DInventorViewer->getSoRenderManager()) {
            rm->scheduleRedraw();
        }
    }
}

const Face* NaviCubeImplementation::findFace(PickId id) const
{
    if (const Face* face = m_SoNaviCube->faceForId(id)) {
        return face;
    }
    return m_SoNaviCube->buttonFaceForId(id);
}

FaceType NaviCubeImplementation::getFaceType(PickId id) const
{
    if (const Face* face = findFace(id)) {
        return face->type;
    }
    return FaceType::None;
}

SbRotation NaviCubeImplementation::getFaceRotation(PickId id) const
{
    if (const Face* face = findFace(id)) {
        return face->rotation;
    }
    return SbRotation();
}

void NaviCubeImplementation::moveToCorner(NaviCube::Corner c)
{
    if (c == NaviCube::TopLeftCorner) {
        m_RelPos = SbVec2f(0.0f, 1.0f);
    }
    else if (c == NaviCube::TopRightCorner) {
        m_RelPos = SbVec2f(1.0f, 1.0f);
    }
    else if (c == NaviCube::BottomLeftCorner) {
        m_RelPos = SbVec2f(0.0f, 0.0f);
    }
    else if (c == NaviCube::BottomRightCorner) {
        m_RelPos = SbVec2f(1.0f, 0.0f);
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
    if (m_TextFont.empty()) {
        font.fromString(QStringLiteral("Arial"));
    }
    else {
        font.fromString(QString::fromStdString(m_TextFont));
    }
    font.setStyleHint(QFont::SansSerif);
    if (m_FontWeight > 0) {
        font.setWeight(convertWeights(m_FontWeight));
    }
    if (m_FontStretch > 0) {
        font.setStretch(m_FontStretch);
    }
    font.setPointSizeF(texSize);
    QFontMetrics fm(font);
    qreal minFontSize = texSize;
    qreal maxFontSize = 0.;
    vector<PickId> mains
        = {PickId::Front, PickId::Top, PickId::Right, PickId::Rear, PickId::Bottom, PickId::Left};
    for (PickId pickId : mains) {
        auto t = QString::fromUtf8(m_LabelTextures[pickId].label.c_str());
        QRect br = fm.boundingRect(t);
        float scale = (float)texSize / max(br.width(), br.height());
        m_LabelTextures[pickId].fontSize = texSize * scale;
        minFontSize = std::min(minFontSize, m_LabelTextures[pickId].fontSize);
        maxFontSize = std::max(maxFontSize, m_LabelTextures[pickId].fontSize);
    }
    if (m_FontZoom > 0.0) {
        maxFontSize = minFontSize + (maxFontSize - minFontSize) * m_FontZoom;
    }
    else {
        maxFontSize = minFontSize * std::pow(2.0, m_FontZoom);
    }
    for (PickId pickId : mains) {
        QImage image(texSize, texSize, QImage::Format_ARGB32);
        image.fill(qRgba(255, 255, 255, 0));
        if (m_LabelTextures[pickId].fontSize > 0.5) {
            // 5% margin looks nice and prevents some artifacts
            font.setPointSizeF(std::min(m_LabelTextures[pickId].fontSize, maxFontSize) * 0.9);
            QPainter paint;
            paint.begin(&image);
            paint.setRenderHints(
                QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform
            );
            paint.setPen(Qt::white);
            QString text = QString::fromUtf8(m_LabelTextures[pickId].label.c_str());
            paint.setFont(font);
            paint.drawText(QRect(0, 0, texSize, texSize), Qt::AlignCenter, text);
            int offset = imageVerticalBalance(image, font.pointSize());
            image.fill(qRgba(255, 255, 255, 0));
            paint.drawText(QRect(0, offset, texSize, texSize), Qt::AlignCenter, text);
            paint.end();
        }

        if (m_LabelTextures[pickId].texture) {
            m_SoNaviCube->setLabelTexture(pickId, 0);
            delete m_LabelTextures[pickId].texture;
        }
        m_LabelTextures[pickId].texture = new QOpenGLTexture(image.mirrored());
        m_LabelTextures[pickId].texture->setMaximumAnisotropy(4.0);
        m_LabelTextures[pickId].texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        m_LabelTextures[pickId].texture->setMagnificationFilter(QOpenGLTexture::Linear);
        m_LabelTextures[pickId].texture->generateMipMaps();
        m_SoNaviCube->setLabelTexture(pickId, m_LabelTextures[pickId].texture->textureId());
    }
}

void NaviCubeImplementation::setSize(int size)
{
    m_CubeWidgetSize = size;
    m_ViewSize = SbVec2s(0, 0);
    m_Prepared = false;
    requestRedraw();
}

void NaviCubeImplementation::prepare()
{
    if (m_Prepared || !m_View3DInventorViewer->viewport()) {
        return;
    }

    m_SoNaviCube->setChamfer(m_Chamfer);
    (void)m_SoNaviCube->faces();

    createCubeFaceTextures();

    if (m_PickingFramebuffer) {
        delete m_PickingFramebuffer;
        m_PickingFramebuffer = nullptr;
    }

    ensureFramebufferValid();
    m_Prepared = true;
    requestRedraw();
}

bool NaviCubeImplementation::readyToRender()
{
    prepare();
    return m_Prepared;
}

bool NaviCubeImplementation::populateRenderParams(
    float opacity,
    int viewportX,
    int viewportY,
    int viewportWidth,
    int viewportHeight
)
{
    SoCamera* cam = m_View3DInventorViewer->getSoRenderManager()->getCamera();
    if (!cam) {
        return false;
    }

    m_SoNaviCube->size = static_cast<float>(m_CubeWidgetSize);
    m_SoNaviCube->opacity = opacity;
    m_SoNaviCube->borderWidth = static_cast<float>(m_BorderWidth);
    m_SoNaviCube->showCoordinateSystem = m_ShowCS;
    m_SoNaviCube->hiliteId = static_cast<int>(m_HiliteId);
    m_SoNaviCube->baseColor.setValue(m_BaseColor.redF(), m_BaseColor.greenF(), m_BaseColor.blueF());
    m_SoNaviCube->baseAlpha = static_cast<float>(m_BaseColor.alphaF());
    m_SoNaviCube->emphaseColor
        .setValue(m_EmphaseColor.redF(), m_EmphaseColor.greenF(), m_EmphaseColor.blueF());
    m_SoNaviCube->emphaseAlpha = static_cast<float>(m_EmphaseColor.alphaF());
    m_SoNaviCube->hiliteColor
        .setValue(m_HiliteColor.redF(), m_HiliteColor.greenF(), m_HiliteColor.blueF());
    m_SoNaviCube->hiliteAlpha = static_cast<float>(m_HiliteColor.alphaF());
    m_SoNaviCube->axisXColor.setValue(
        static_cast<float>(m_xColor.r),
        static_cast<float>(m_xColor.g),
        static_cast<float>(m_xColor.b)
    );
    m_SoNaviCube->axisYColor.setValue(
        static_cast<float>(m_yColor.r),
        static_cast<float>(m_yColor.g),
        static_cast<float>(m_yColor.b)
    );
    m_SoNaviCube->axisZColor.setValue(
        static_cast<float>(m_zColor.r),
        static_cast<float>(m_zColor.g),
        static_cast<float>(m_zColor.b)
    );

    SbVec4f rect(
        static_cast<float>(viewportX),
        static_cast<float>(viewportY),
        static_cast<float>(viewportWidth),
        static_cast<float>(viewportHeight)
    );
    m_SoNaviCube->viewportRect = rect;
    m_SoNaviCube->cameraOrientation = cam->orientation.getValue();
    m_SoNaviCube->cameraIsOrthographic = cam->getTypeId().isDerivedFrom(
        SoOrthographicCamera::getClassTypeId()
    );

    return true;
}

void NaviCubeImplementation::createContextMenu(const std::vector<std::string>& cmd)
{
    CommandManager& rcCmdMgr = Application::Instance->commandManager();
    m_Menu->clear();

    for (const auto& i : cmd) {
        Command* cmd = rcCmdMgr.getCommandByName(i.c_str());
        if (cmd) {
            cmd->addTo(m_Menu);
        }
    }
}

void NaviCubeImplementation::handleResize(const SbVec2s& viewSize)
{
    qreal devicePixelRatio = m_View3DInventorViewer->devicePixelRatio();
    if (viewSize != m_ViewSize || devicePixelRatio != m_DevicePixelRatio) {
        m_DevicePixelRatio = devicePixelRatio;
        qreal physicalCubeWidgetSize = getPhysicalCubeWidgetSize();
        m_PosAreaBase[0]
            = std::min((int)(m_PosOffset[0] + physicalCubeWidgetSize * 0.55), viewSize[0] / 2);
        m_PosAreaBase[1]
            = std::min((int)(m_PosOffset[1] + physicalCubeWidgetSize * 0.55), viewSize[1] / 2);
        m_PosAreaSize[0] = viewSize[0] - 2 * m_PosAreaBase[0];
        m_PosAreaSize[1] = viewSize[1] - 2 * m_PosAreaBase[1];
        m_ViewSize = viewSize;
    }
}

bool NaviCubeImplementation::isFramebufferValid() const
{
    return m_PickingFramebuffer && m_PickingFramebuffer->isValid();
}


void NaviCubeImplementation::ensureFramebufferValid()
{
    if (!isFramebufferValid()) {
        if (m_PickingFramebuffer) {

            if (!m_PickingFramebuffer->isValid()) {
                Base::Console().developerWarning(
                    "NaviCube",
                    "The frame buffer has become invalid, a new frame buffer will be created\n"
                );
            }

            delete m_PickingFramebuffer;
            m_PickingFramebuffer = nullptr;
        }

        qreal physicalCubeWidgetSize = getPhysicalCubeWidgetSize();
        m_PickingFramebuffer = new QOpenGLFramebufferObject(
            2 * physicalCubeWidgetSize,
            2 * physicalCubeWidgetSize,
            QOpenGLFramebufferObject::CombinedDepthStencil
        );
    }
}

PickId NaviCubeImplementation::pickFace(short x, short y)
{
    if (!readyToRender()) {
        return PickId::None;
    }

    qreal physicalCubeWidgetSize = getPhysicalCubeWidgetSize();
    GLubyte pixels[4] = {0};
    ensureFramebufferValid();
    if (m_PickingFramebuffer && std::abs(x) <= physicalCubeWidgetSize / 2
        && std::abs(y) <= physicalCubeWidgetSize / 2) {
        auto* viewportWidget = static_cast<QOpenGLWidget*>(m_View3DInventorViewer->viewport());
        viewportWidget->makeCurrent();
        m_PickingFramebuffer->bind();

        int viewportSize = static_cast<int>(physicalCubeWidgetSize * 2.0);

        if (populateRenderParams(1.0F, 0, 0, viewportSize, viewportSize)) {
            m_SoNaviCube->renderGL(true);
            glFinish();
            int center = viewportSize / 2;
            glReadPixels(2 * x + center, 2 * y + center, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixels);
        }

        m_PickingFramebuffer->release();
        viewportWidget->doneCurrent();
    }
    return pixels[3] == 255 ? static_cast<PickId>(pixels[0]) : PickId::None;
}

bool NaviCubeImplementation::mousePressed(short x, short y)
{
    m_MouseDown = true;
    m_MightDrag = inDragZone(x, y);
    PickId pick = pickFace(x, y);
    setHilite(pick);
    return pick != PickId::None;
}

void NaviCubeImplementation::handleMenu()
{
    m_Menu->exec(QCursor::pos());
}

SbRotation NaviCubeImplementation::getNearestOrientation(PickId pickId)
{
    SbRotation cameraOrientation = m_View3DInventorViewer->getCameraOrientation();
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
    m_MouseDown = false;

    if (m_Dragging) {
        m_Dragging = false;
    }
    else {
        PickId pickId = pickFace(x, y);
        long step = Base::clamp(long(m_NaviStepByTurn), 4L, 36L);
        float rotStepAngle = (2 * std::numbers::pi) / step;

        FaceType faceType = getFaceType(pickId);
        if (faceType == FaceType::Main || faceType == FaceType::Edge || faceType == FaceType::Corner) {
            // Handle the cube faces
            SbRotation orientation;
            if (m_RotateToNearest) {
                orientation = getNearestOrientation(pickId);
            }
            else {
                orientation = getFaceRotation(pickId);
            }
            m_View3DInventorViewer->setCameraOrientation(orientation);
        }
        else if (faceType == FaceType::Button) {

            // Handle the menu
            if (pickId == PickId::ViewMenu) {
                handleMenu();
                return true;
            }

            // Handle the flat buttons
            SbRotation rotation = getFaceRotation(pickId);
            if (pickId == PickId::DotBackside) {
                rotation.scaleAngle(pi);
            }
            else {
                rotation.scaleAngle(rotStepAngle);
            }

            // If the previous flat button animation is still active then apply the rotation to the
            // previous target orientation, otherwise apply the rotation to the current camera
            // orientation
            if (m_flatButtonAnimation != nullptr
                && m_flatButtonAnimation->state() == QAbstractAnimation::Running) {
                m_flatButtonTargetOrientation = rotation * m_flatButtonTargetOrientation;
            }
            else {
                m_flatButtonTargetOrientation = rotation
                    * m_View3DInventorViewer->getCameraOrientation();
            }

            m_flatButtonAnimation = m_View3DInventorViewer->setCameraOrientation(
                m_flatButtonTargetOrientation
            );
        }
        else {
            return false;
        }
    }
    return true;
}

void NaviCubeImplementation::setHilite(PickId hilite)
{
    if (hilite != m_HiliteId) {
        m_HiliteId = hilite;
        if (m_SoNaviCube) {
            m_SoNaviCube->hiliteId = static_cast<int>(m_HiliteId);
        }
        m_View3DInventorViewer->getSoRenderManager()->scheduleRedraw();
    }
}

bool NaviCubeImplementation::inDragZone(short x, short y)
{
    qreal physicalCubeWidgetSize = getPhysicalCubeWidgetSize();
    int limit = physicalCubeWidgetSize / 4;
    return std::abs(x) < limit && std::abs(y) < limit;
}

bool NaviCubeImplementation::mouseMoved(short x, short y)
{
    qreal physicalCubeWidgetSize = getPhysicalCubeWidgetSize();
    bool hovering = std::abs(x) <= physicalCubeWidgetSize / 2
        && std::abs(y) <= physicalCubeWidgetSize / 2;

    if (hovering != m_Hovering) {
        m_Hovering = hovering;
        m_View3DInventorViewer->getSoRenderManager()->scheduleRedraw();
    }

    if (!m_Dragging) {
        setHilite(pickFace(x, y));
    }

    if (m_MouseDown && m_Draggable) {
        if (m_MightDrag && !m_Dragging) {
            m_Dragging = true;
            setHilite(PickId::None);
        }
        if (m_Dragging && (std::abs(x) || std::abs(y))) {
            float newX = m_RelPos[0] + (float)(x) / m_PosAreaSize[0];
            float newY = m_RelPos[1] + (float)(y) / m_PosAreaSize[1];
            m_RelPos[0] = std::min(std::max(newX, 0.0f), 1.0f);
            m_RelPos[1] = std::min(std::max(newY, 0.0f), 1.0f);

            m_View3DInventorViewer->getSoRenderManager()->scheduleRedraw();
            return true;
        }
    }
    return false;
}

bool NaviCubeImplementation::processSoEvent(const SoEvent* ev)
{
    short x, y;
    ev->getPosition().getValue(x, y);
    // translate to internal cube center based coordinates
    short rx = x - (short)(m_PosAreaSize[0] * m_RelPos[0]) - m_PosAreaBase[0];
    short ry = y - (short)(m_PosAreaSize[1] * m_RelPos[1]) - m_PosAreaBase[1];
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
    m_NaviCubeImplementation->m_xColor = Base::Color(static_cast<uint32_t>(colorLong));
    colorLong = Gui::ViewParams::instance()->getAxisYColor();
    m_NaviCubeImplementation->m_yColor = Base::Color(static_cast<uint32_t>(colorLong));
    colorLong = Gui::ViewParams::instance()->getAxisZColor();
    m_NaviCubeImplementation->m_zColor = Base::Color(static_cast<uint32_t>(colorLong));
    m_NaviCubeImplementation->requestRedraw();
}

void NaviCube::setNaviCubeCommands(const std::vector<std::string>& cmd)
{
    NaviCubeImplementation::m_commands = cmd;
}

DEF_STD_CMD_AC(NaviCubeDraggableCmd)

NaviCubeDraggableCmd::NaviCubeDraggableCmd()
    : Command("NaviCubeDraggableCmd")
{
    sGroup = "";
    sMenuText = QT_TR_NOOP("Movable Navigation Cube");
    sToolTipText = QT_TR_NOOP("Drag and place NaviCube");
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

    vector<string> commands = NaviCubeImplementation::m_commands;
    if (commands.empty()) {
        commands.emplace_back("Std_OrthographicCamera");
        commands.emplace_back("Std_PerspectiveCamera");
        commands.emplace_back("Std_ViewIsometric");
        commands.emplace_back("Separator");
        commands.emplace_back("Std_ViewFitAll");
        commands.emplace_back("Std_ViewFitSelection");
        commands.emplace_back("Std_AlignToSelection");
        commands.emplace_back("Separator");
        commands.emplace_back("NaviCubeDraggableCmd");
    }

    for (const auto& command : commands) {
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
