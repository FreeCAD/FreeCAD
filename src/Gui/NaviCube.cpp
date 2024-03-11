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

#include "PreCompiled.h"
#ifndef _PreComp_
# include <algorithm>
# include <cfloat>
# ifdef FC_OS_WIN32
#  include <windows.h>
# endif
# ifdef FC_OS_MACOSX
#  include <OpenGL/gl.h>
# else
#  include <GL/gl.h>
# endif
# include <boost/math/constants/constants.hpp>
# include <Inventor/nodes/SoOrthographicCamera.h>
# include <Inventor/events/SoEvent.h>
# include <Inventor/events/SoLocation2Event.h>
# include <Inventor/events/SoMouseButtonEvent.h>
# include <QApplication>
# include <QCursor>
# include <QImage>
# include <QMenu>
# include <QOpenGLTexture>
# include <QPainterPath>
#endif

#include <App/Color.h>
#include <Base/Tools.h>
#include <Eigen/Dense>

#include "NaviCube.h"
#include "Application.h"
#include "Command.h"
#include "Action.h"
#include "MainWindow.h"
#include "View3DInventorViewer.h"
#include "View3DInventor.h"


using namespace Eigen;
using namespace std;
using namespace Gui;





class NaviCubeImplementation {
public:
    explicit NaviCubeImplementation(Gui::View3DInventorViewer*);
    ~NaviCubeImplementation();
    void drawNaviCube();
    void createContextMenu(const std::vector<std::string>& cmd);
    void createCubeFaceTextures();

    void moveToCorner(NaviCube::Corner c);
    void setLabels(const std::vector<std::string>& labels);

    bool processSoEvent(const SoEvent* ev);
    void setSize(int size);

private:
    enum class PickId {
        None,
        Front,
        Top,
        Right,
        Rear,
        Bottom,
        Left,
        FrontTop,
        FrontBottom,
        FrontRight,
        FrontLeft,
        RearTop,
        RearBottom,
        RearRight,
        RearLeft,
        TopRight,
        TopLeft,
        BottomRight,
        BottomLeft,
        FrontTopRight,
        FrontTopLeft,
        FrontBottomRight,
        FrontBottomLeft,
        RearTopRight,
        RearTopLeft,
        RearBottomRight,
        RearBottomLeft,
        ArrowNorth,
        ArrowSouth,
        ArrowEast,
        ArrowWest,
        ArrowRight,
        ArrowLeft,
        DotBackside,
        ViewMenu
    };
    enum class DirId{
        Custom, Up, Right, Out
    };
    enum class ShapeId{
        None, Main, Edge, Corner, Button
    };
    struct Face {
        ShapeId type;
        vector<Vector3f> vertexArray;
        // The rotation is the standard orientation for the faces of the cube
        // For the flat buttons the rotation contains the direction of the rotation
        // The standard orientation is the desired camera orientation when a face is selected and
        // rotate to nearest is disabled
        SbRotation rotation;
    };
    struct LabelTexture {
        vector<Vector3f> vertexArray;
        qreal fontSize;
        QOpenGLTexture *texture = nullptr;
        string label;
    };
    bool mousePressed(short x, short y);
    bool mouseReleased(short x, short y);
    bool mouseMoved(short x, short y);
    PickId pickFace(short x, short y);
    bool inDragZone(short x, short y);

    void prepare();
    void handleResize();
    void handleMenu();

    void setHilite(PickId);

    void addCubeFace(const Vector3f&, const Vector3f&, ShapeId, PickId, float rotZ = 0.0);
    void addButtonFace(PickId, const SbVec3f& direction = SbVec3f(0, 0, 0));

    QString str(const char* str);
    QMenu* createNaviCubeMenu();
    void drawNaviCube(bool picking, float opacity);

    SbRotation getNearestOrientation(PickId pickId);

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
    SbVec2s m_PosOffset = SbVec2s(0,0);

    bool m_Prepared = false;
    static vector<string> m_commands;
    bool m_Draggable = false;
    SbVec2s m_ViewSize = SbVec2s(0,0);

private:
    bool m_MouseDown = false;
    bool m_Dragging = false;
    bool m_MightDrag = false;
    bool m_Hovering = false;

    SbVec2f m_RelPos = SbVec2f(1.0f,1.0f);
    SbVec2s m_PosAreaBase = SbVec2s(0,0);
    SbVec2s m_PosAreaSize = SbVec2s(0,0);

    QtGLFramebufferObject* m_PickingFramebuffer;
    Gui::View3DInventorViewer* m_View3DInventorViewer;

    map<PickId, Face> m_Faces;
    map<PickId, LabelTexture> m_LabelTextures;

    QMenu* m_Menu;
};

int NaviCubeImplementation::m_CubeWidgetSize = 132;

int NaviCube::getNaviCubeSize()
{
    return NaviCubeImplementation::m_CubeWidgetSize;
}

NaviCube::NaviCube(Gui::View3DInventorViewer* viewer) {
    m_NaviCubeImplementation = new NaviCubeImplementation(viewer);
}

NaviCube::~NaviCube() {
    delete m_NaviCubeImplementation;
}

void NaviCube::drawNaviCube() {
    m_NaviCubeImplementation->drawNaviCube();
}

void NaviCube::createContextMenu(const std::vector<std::string>& cmd) {
    m_NaviCubeImplementation->createContextMenu(cmd);
}

bool NaviCube::processSoEvent(const SoEvent* ev) {
    return m_NaviCubeImplementation->processSoEvent(ev);
}

vector<string> NaviCubeImplementation::m_commands;

void NaviCube::setCorner(Corner c) {
    m_NaviCubeImplementation->moveToCorner(c);
}

void NaviCube::setOffset(int x, int y) {
    m_NaviCubeImplementation->m_PosOffset = SbVec2s(x, y);
    m_NaviCubeImplementation->m_ViewSize = SbVec2s(0,0);
}

bool NaviCube::isDraggable() {
    return m_NaviCubeImplementation->m_Draggable;
}

void NaviCube::setDraggable(bool draggable) {
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
}

void NaviCube::setFontWeight(int weight)
{
    m_NaviCubeImplementation->m_FontWeight = weight;
    m_NaviCubeImplementation->m_Prepared = false;
}

void NaviCube::setFontStretch(int stretch)
{
    m_NaviCubeImplementation->m_FontStretch = stretch;
    m_NaviCubeImplementation->m_Prepared = false;
}

void NaviCube::setFontZoom(float zoom)
{
    m_NaviCubeImplementation->m_FontZoom = zoom;
    m_NaviCubeImplementation->m_Prepared = false;
}

void NaviCube::setBaseColor(QColor bColor)
{
    m_NaviCubeImplementation->m_BaseColor = bColor;
}

void NaviCube::setEmphaseColor(QColor eColor)
{
    m_NaviCubeImplementation->m_EmphaseColor = eColor;
    m_NaviCubeImplementation->m_Prepared = false;
}

void NaviCube::setHiliteColor(QColor HiliteColor)
{
    m_NaviCubeImplementation->m_HiliteColor = HiliteColor;
}

void NaviCube::setBorderWidth(double BorderWidth)
{
    m_NaviCubeImplementation->m_BorderWidth = BorderWidth;
}

void NaviCube::setShowCS(bool showCS)
{
    m_NaviCubeImplementation->m_ShowCS = showCS;
}

void NaviCube::setNaviCubeLabels(const std::vector<std::string>& labels)
{
    m_NaviCubeImplementation->setLabels(labels);
}

void NaviCube::setInactiveOpacity(float opacity)
{
    m_NaviCubeImplementation->m_InactiveOpacity = opacity;
}

void NaviCubeImplementation::setLabels(const std::vector<std::string>& labels)
{
    m_LabelTextures[PickId::Front].label  = labels[0];
    m_LabelTextures[PickId::Top].label    = labels[1];
    m_LabelTextures[PickId::Right].label  = labels[2];
    m_LabelTextures[PickId::Rear].label   = labels[3];
    m_LabelTextures[PickId::Bottom].label = labels[4];
    m_LabelTextures[PickId::Left].label   = labels[5];
    m_Prepared = false;
}

NaviCubeImplementation::NaviCubeImplementation(Gui::View3DInventorViewer* viewer)
    : m_BaseColor{226, 232, 239}
    , m_HiliteColor{170, 226, 255}
{
    m_View3DInventorViewer = viewer;
    m_PickingFramebuffer = nullptr;
    m_Menu = createNaviCubeMenu();
}

NaviCubeImplementation::~NaviCubeImplementation()
{
    delete m_Menu;
    if (m_PickingFramebuffer)
        delete m_PickingFramebuffer;
    for (auto tex: m_LabelTextures) {
        delete tex.second.texture;
    }
}

void NaviCubeImplementation::moveToCorner(NaviCube::Corner c) {
    if      (c == NaviCube::TopLeftCorner)     m_RelPos = SbVec2f(0.0f, 1.0f);
    else if (c == NaviCube::TopRightCorner)    m_RelPos = SbVec2f(1.0f, 1.0f);
    else if (c == NaviCube::BottomLeftCorner)  m_RelPos = SbVec2f(0.0f, 0.0f);
    else if (c == NaviCube::BottomRightCorner) m_RelPos = SbVec2f(1.0f, 0.0f);
 }

auto convertWeights = [](int weight) -> QFont::Weight {
    if (weight >= 87)
        return QFont::Black;
    if (weight >= 81)
        return QFont::ExtraBold;
    if (weight >= 75)
        return QFont::Bold;
    if (weight >= 63)
        return QFont::DemiBold;
    if (weight >= 57)
        return QFont::Medium;
    if (weight >= 50)
        return QFont::Normal;
    if (weight >= 25)
        return QFont::Light;
    if (weight >= 12)
        return QFont::ExtraLight;
    return QFont::Thin;
};

int imageVerticalBalance(QImage p, int sizeHint) {
    if (sizeHint < 0) {
        return 0;
    }

    int h = p.height();
    int startRow = (h - sizeHint) / 2;
    bool done = false;
    int x, bottom, top;
    for (top = startRow; top < h; top++){
        for (x = 0; x < p.width(); x++){
            if (qAlpha(p.pixel(x, top))) {
                done = true;
                break;
            }
        }
        if (done) break;
    }
    for (bottom = startRow; bottom < h; bottom++) {
        for (x = 0; x < p.width(); x++){
            if (qAlpha(p.pixel(x, h-1-bottom)))
                return (bottom-top)/2;
        }
    }
    return 0;
}

void NaviCubeImplementation::createCubeFaceTextures() {
    int texSize = 192; // Works well for the max cube size 1024
    QFont font;
    if (m_TextFont.empty()) font.fromString(QStringLiteral("Arial"));
    else font.fromString(QString::fromStdString(m_TextFont));
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
    vector<PickId> mains = {PickId::Front, PickId::Top, PickId::Right, PickId::Rear, PickId::Bottom, PickId::Left};
    for (PickId pickId : mains) {
        auto t = QString::fromUtf8(m_LabelTextures[pickId].label.c_str());
        QRect br = fm.boundingRect(t);
        float scale = (float)texSize / max(br.width(),br.height());
        m_LabelTextures[pickId].fontSize = texSize * scale;
        minFontSize = std::min(minFontSize, m_LabelTextures[pickId].fontSize);
        maxFontSize = std::max(maxFontSize, m_LabelTextures[pickId].fontSize);
    }
    if (m_FontZoom > 0.0)
        maxFontSize = minFontSize + (maxFontSize - minFontSize) * m_FontZoom;
    else {
        maxFontSize = minFontSize * std::pow(2.0, m_FontZoom);
    }
    for (PickId pickId : mains) {
        QImage image(texSize, texSize, QImage::Format_ARGB32);
        image.fill(qRgba(255, 255, 255, 0));
        if (m_LabelTextures[pickId].fontSize > 0.5) {
            // 5% margin looks nice and prevents some artifacts
            font.setPointSizeF(std::min(m_LabelTextures[pickId].fontSize, maxFontSize)*0.9);
            QPainter paint;
            paint.begin(&image);
            paint.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
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
            delete m_LabelTextures[pickId].texture;
        }
        m_LabelTextures[pickId].texture = new QOpenGLTexture(image.mirrored());
        m_LabelTextures[pickId].texture->setMaximumAnisotropy(4.0);
        m_LabelTextures[pickId].texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        m_LabelTextures[pickId].texture->setMagnificationFilter(QOpenGLTexture::Linear);
        m_LabelTextures[pickId].texture->generateMipMaps();
    }
}

void NaviCubeImplementation::addButtonFace(PickId pickId, const SbVec3f& direction)
{
    if (m_Faces[pickId].vertexArray.size())
        m_Faces[pickId].vertexArray.clear();
    float scale = 0.005F;
    float offx = 0.5F;
    float offy = 0.5F;
    vector<float> pointData;

    switch (pickId) {
        default:
            break;
        case PickId::ArrowRight:
        case PickId::ArrowLeft: {
            pointData = {
                66.6F, -66.6F,//outer curve
                58.3F, -74.0F,
                49.2F ,-80.3F,
                39.4F ,-85.5F,
                29.0F, -89.5F,
                25.3F, -78.1F,//inner curve
                34.3F, -74.3F,
                42.8F, -69.9F,
                50.8F, -64.4F,
                58.1F, -58.1F,
                53.8F, -53.8F,//arrowhead
                74.7F, -46.8F,
                70.7F, -70.4F
            };
            break;
        }
        case PickId::ArrowWest:
        case PickId::ArrowNorth:
        case PickId::ArrowSouth:
        case PickId::ArrowEast: {
            pointData = {
                100.,  0.,
                 80.,-18.,
                 80., 18.
             };
            break;
        }
        case PickId::ViewMenu: {
            offx = 0.84F;
            offy = 0.84F;
            pointData = {
                  0.,   0.,//top rhombus
                 15.,  -6.,
                  0., -12.,
                -15.,  -6.,
                  0.,   0.,//left rhombus
                -15.,  -6.,
                -15.,  12.,
                  0.,  18.,
                  0.,   0.,//right rhombus
                  0.,  18.,
                 15.,  12.,
                 15.,  -6.
            };
            break;
        }
        case PickId::DotBackside: {
            int steps = 16;
            for (int i = 0; i < steps; i++) {
                float angle = 2.0f * M_PI * ((float)i+0.5) / (float)steps;
                pointData.emplace_back(10. * cos(angle) + 87.);
                pointData.emplace_back(10. * sin(angle) - 87.);
            }
            break;
        }
    }

    int count = static_cast<int>(pointData.size())/2;
    m_Faces[pickId].vertexArray.reserve(count);
    for (int i = 0; i < count; i++) {
        float x = pointData[i*2]   * scale + offx;
        float y = pointData[i*2+1] * scale + offy;
        if (pickId == PickId::ArrowNorth || pickId == PickId::ArrowWest || pickId == PickId::ArrowLeft)
            x = 1.0 - x;
        if (pickId == PickId::ArrowSouth || pickId == PickId::ArrowNorth)
            m_Faces[pickId].vertexArray.emplace_back(Vector3f(y, x, 0.0));
        else
            m_Faces[pickId].vertexArray.emplace_back(Vector3f(x, y, 0.0));
    }
    m_Faces[pickId].type = ShapeId::Button;
    m_Faces[pickId].rotation = SbRotation(direction, 1).inverse();
}

void NaviCubeImplementation::addCubeFace(const Vector3f& x, const Vector3f& z, ShapeId shapeType, PickId pickId, float rotZ) {
    m_Faces[pickId].vertexArray.clear();
    m_Faces[pickId].type = shapeType;

    Vector3f y = x.cross(-z);

    // Determine the standard orientations based on vector x and vector z
    // Rotate by an additional rotZ if vector x and vector z are not already the standard orientation

    // Create normalized vectors for x, y and z
    SbVec3f xN(x.x(), x.y(), x.z());
    SbVec3f yN(y.x(), y.y(), y.z());
    SbVec3f zN(z.x(), z.y(), z.z());
    xN.normalize();
    yN.normalize();
    zN.normalize();

    // Create a rotation matrix
    SbMatrix R(xN[0], yN[0], zN[0], 0,
                 xN[1], yN[1], zN[1], 0,
                 xN[2], yN[2], zN[2], 0,
                 0,     0,     0,     1);

    // Store the standard orientation
    m_Faces[pickId].rotation = (SbRotation(R) * SbRotation(SbVec3f(0, 0, 1), rotZ)).inverse();

    if (shapeType == ShapeId::Corner) {
        auto xC = x * m_Chamfer;
        auto yC = y * m_Chamfer;
        auto zC = (1 - 2 * m_Chamfer) * z;
        m_Faces[pickId].vertexArray.reserve(6);
        m_Faces[pickId].vertexArray.emplace_back(zC - 2 * xC);
        m_Faces[pickId].vertexArray.emplace_back(zC - xC - yC);
        m_Faces[pickId].vertexArray.emplace_back(zC + xC - yC);
        m_Faces[pickId].vertexArray.emplace_back(zC + 2 * xC);
        m_Faces[pickId].vertexArray.emplace_back(zC + xC + yC);
        m_Faces[pickId].vertexArray.emplace_back(zC - xC + yC);
    }
    else if (shapeType == ShapeId::Edge) {
        auto x4 = x * (1 - m_Chamfer * 4);
        auto yE = y * m_Chamfer;
        auto zE = z * (1 - m_Chamfer);
        m_Faces[pickId].vertexArray.reserve(4);
        m_Faces[pickId].vertexArray.emplace_back(zE - x4 - yE);
        m_Faces[pickId].vertexArray.emplace_back(zE + x4 - yE);
        m_Faces[pickId].vertexArray.emplace_back(zE + x4 + yE);
        m_Faces[pickId].vertexArray.emplace_back(zE - x4 + yE);
    }
    else if (shapeType == ShapeId::Main) {
        auto x2 = x * (1 - m_Chamfer * 2);
        auto y2 = y * (1 - m_Chamfer * 2);
        auto x4 = x * (1 - m_Chamfer * 4);
        auto y4 = y * (1 - m_Chamfer * 4);
        m_Faces[pickId].vertexArray.reserve(8);
        m_Faces[pickId].vertexArray.emplace_back(z - x2 - y4);
        m_Faces[pickId].vertexArray.emplace_back(z - x4 - y2);
        m_Faces[pickId].vertexArray.emplace_back(z + x4 - y2);
        m_Faces[pickId].vertexArray.emplace_back(z + x2 - y4);

        m_Faces[pickId].vertexArray.emplace_back(z + x2 + y4);
        m_Faces[pickId].vertexArray.emplace_back(z + x4 + y2);
        m_Faces[pickId].vertexArray.emplace_back(z - x4 + y2);
        m_Faces[pickId].vertexArray.emplace_back(z - x2 + y4);

        m_LabelTextures[pickId].vertexArray.clear();
        m_LabelTextures[pickId].vertexArray.emplace_back(z - x2 - y2);
        m_LabelTextures[pickId].vertexArray.emplace_back(z + x2 - y2);
        m_LabelTextures[pickId].vertexArray.emplace_back(z + x2 + y2);
        m_LabelTextures[pickId].vertexArray.emplace_back(z - x2 + y2);

    }
}

void NaviCubeImplementation::setSize(int size)
{
    m_CubeWidgetSize = size;
    m_ViewSize = SbVec2s(0,0);
    m_Prepared = false;
}

void NaviCubeImplementation::prepare()
{
    static const float pi = boost::math::constants::pi<float>();
    static const float pi1_2 = boost::math::constants::half_pi<float>();

    createCubeFaceTextures();

    Vector3f x(1, 0, 0);
    Vector3f y(0, 1, 0);
    Vector3f z(0, 0, 1);

    // create the main faces
    addCubeFace( x, z, ShapeId::Main, PickId::Top);
    addCubeFace( x,-y, ShapeId::Main, PickId::Front);
    addCubeFace(-y,-x, ShapeId::Main, PickId::Left);
    addCubeFace(-x, y, ShapeId::Main, PickId::Rear);
    addCubeFace( y, x, ShapeId::Main, PickId::Right);
    addCubeFace( x,-z, ShapeId::Main, PickId::Bottom);

    // create corner faces
    addCubeFace(-x-y, x-y+z, ShapeId::Corner, PickId::FrontTopRight, pi);
    addCubeFace(-x+y,-x-y+z, ShapeId::Corner, PickId::FrontTopLeft, pi);
    addCubeFace(x+y, x-y-z, ShapeId::Corner, PickId::FrontBottomRight);
    addCubeFace(x-y,-x-y-z, ShapeId::Corner, PickId::FrontBottomLeft);
    addCubeFace(x-y, x+y+z, ShapeId::Corner, PickId::RearTopRight, pi);
    addCubeFace(x+y,-x+y+z, ShapeId::Corner, PickId::RearTopLeft, pi);
    addCubeFace(-x+y, x+y-z, ShapeId::Corner, PickId::RearBottomRight);
    addCubeFace(-x-y,-x+y-z, ShapeId::Corner, PickId::RearBottomLeft);

    // create edge faces
    addCubeFace(x, z-y, ShapeId::Edge, PickId::FrontTop);
    addCubeFace(x,-z-y, ShapeId::Edge, PickId::FrontBottom);
    addCubeFace(x, y-z, ShapeId::Edge, PickId::RearBottom, pi);
    addCubeFace(x, y+z, ShapeId::Edge, PickId::RearTop, pi);
    addCubeFace(z, x+y, ShapeId::Edge, PickId::RearRight, pi1_2);
    addCubeFace(z, x-y, ShapeId::Edge, PickId::FrontRight, pi1_2);
    addCubeFace(z,-x-y, ShapeId::Edge, PickId::FrontLeft, pi1_2);
    addCubeFace(z, y-x, ShapeId::Edge, PickId::RearLeft, pi1_2);
    addCubeFace(y, z-x, ShapeId::Edge, PickId::TopLeft, pi);
    addCubeFace(y, x+z, ShapeId::Edge, PickId::TopRight);
    addCubeFace(y, x-z, ShapeId::Edge, PickId::BottomRight);
    addCubeFace(y,-z-x, ShapeId::Edge, PickId::BottomLeft, pi);

    // create the flat buttons
    addButtonFace(PickId::ArrowNorth, SbVec3f(-1, 0, 0));
    addButtonFace(PickId::ArrowSouth, SbVec3f(1, 0, 0));
    addButtonFace(PickId::ArrowEast, SbVec3f(0, 1, 0));
    addButtonFace(PickId::ArrowWest, SbVec3f(0, -1, 0));
    addButtonFace(PickId::ArrowLeft, SbVec3f(0, 0, 1));
    addButtonFace(PickId::ArrowRight, SbVec3f(0, 0, -1));
    addButtonFace(PickId::DotBackside, SbVec3f(0, 1, 0));
    addButtonFace(PickId::ViewMenu);

    if (m_PickingFramebuffer)
        delete m_PickingFramebuffer;
    m_PickingFramebuffer =
        new QtGLFramebufferObject(2 * m_CubeWidgetSize, 2 * m_CubeWidgetSize,
                                  QtGLFramebufferObject::CombinedDepthStencil);
    m_View3DInventorViewer->getSoRenderManager()->scheduleRedraw();
}

void NaviCubeImplementation::drawNaviCube() {
    handleResize();
    int posX = (int)(m_RelPos[0] * m_PosAreaSize[0]) + m_PosAreaBase[0] - m_CubeWidgetSize / 2;
    int posY = (int)(m_RelPos[1] * m_PosAreaSize[1]) + m_PosAreaBase[1] - m_CubeWidgetSize / 2;
    glViewport(posX, posY, m_CubeWidgetSize, m_CubeWidgetSize);
    drawNaviCube(false, m_Hovering ? 1.f : m_InactiveOpacity);
}

void NaviCubeImplementation::createContextMenu(const std::vector<std::string>& cmd) {
    CommandManager& rcCmdMgr = Application::Instance->commandManager();
    m_Menu->clear();

    for (const auto & i : cmd) {
        Command* cmd = rcCmdMgr.getCommandByName(i.c_str());
        if (cmd)
            cmd->addTo(m_Menu);
    }
}

void NaviCubeImplementation::handleResize() {
    SbVec2s viewSize = m_View3DInventorViewer->getSoRenderManager()->getSize();
    if (viewSize != m_ViewSize) {
        m_PosAreaBase[0] = std::min((int)(m_PosOffset[0] + m_CubeWidgetSize * 0.55), viewSize[0] / 2);
        m_PosAreaBase[1] = std::min((int)(m_PosOffset[1] + m_CubeWidgetSize * 0.55), viewSize[1] / 2);
        m_PosAreaSize[0] = viewSize[0] - 2 * m_PosAreaBase[0];
        m_PosAreaSize[1] = viewSize[1] - 2 * m_PosAreaBase[1];
        m_ViewSize = viewSize;
    }
}

void NaviCubeImplementation::drawNaviCube(bool pickMode, float opacity)
{
    if (!m_Prepared) {
        if (!m_View3DInventorViewer->viewport())
            return;
        prepare();
        m_Prepared = true;
        m_View3DInventorViewer->getSoRenderManager()->scheduleRedraw();
        return;
    }

    SoCamera* cam = m_View3DInventorViewer->getSoRenderManager()->getCamera();
    if (!cam)
        return;

    // Store GL state.
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    // configure
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthRange(0.f, 1.f);
    glClearDepth(1.f);
    glClear(GL_DEPTH_BUFFER_BIT);
    glDepthFunc(GL_LEQUAL);

    glDisable(GL_LIGHTING);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    if (pickMode) {
        glDisable(GL_BLEND);
        glShadeModel(GL_FLAT);
        glDisable(GL_DITHER);
        glDisable(GL_POLYGON_SMOOTH);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    else {
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0f, 1.0f);
        glEnable(GL_BLEND);
        glShadeModel(GL_SMOOTH);
    }

    // mimic 3d view projection
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    const float NEARVAL = 0.1f;
    const float FARVAL = 10.1f;
    if (cam->getTypeId().isDerivedFrom(SoOrthographicCamera::getClassTypeId())) {
        glOrtho(-2.1, 2.1, -2.1, 2.1, NEARVAL, FARVAL);
    }
    else {
        const float dim = NEARVAL * float(tan(M_PI / 8.0)) * 1.1;
        glFrustum(-dim, dim, -dim, dim, NEARVAL, FARVAL);
    }
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    SbMatrix mx;
    mx = cam->orientation.getValue();
    mx = mx.inverse();
    mx[3][2] = -5.1F;
    glLoadMatrixf((float*)mx);

    glEnableClientState(GL_VERTEX_ARRAY);
    QColor& cb = m_EmphaseColor;

    // Draw coordinate system
    if (!pickMode && m_ShowCS) {
        glLineWidth(m_BorderWidth*2.f);
        glPointSize(m_BorderWidth*2.f);
        float a = -1.1f;
        float b = -1.05f;
        float c =  0.5f;

        float pointData[] = {
            b, a, a, // X1
            c, a, a, // X2
            a, b, a, // Y1
            a, c, a, // Y2
            a, a, b, // Z1
            a, a, c, // Z2
            a, a, a  // 0
        };
        glVertexPointer(3, GL_FLOAT, 0, pointData);
        glColor4f(1, 0, 0, opacity);
        glDrawArrays(GL_LINES, 0, 2);
        glDrawArrays(GL_POINTS, 0, 2);
        glColor4f(0, 1, 0, opacity);
        glDrawArrays(GL_LINES, 2, 2);
        glDrawArrays(GL_POINTS, 2, 2);
        glColor4f(0, 0, 1, opacity);
        glDrawArrays(GL_LINES, 4, 2);
        glDrawArrays(GL_POINTS, 4, 2);
    }

    // cube faces
    for (const auto& pair : m_Faces) {
        auto f = pair.second;
        if (f.type == ShapeId::Button)
            continue;
        auto pickId = pair.first;
        if (pickMode) {
            glColor3ub(static_cast<GLubyte>(pickId), 0, 0);
        }
        else {
            QColor& c = m_HiliteId == pickId ? m_HiliteColor : m_BaseColor;
            glColor4f(c.redF(), c.greenF(), c.blueF(), c.alphaF() * opacity);
        }
        glVertexPointer(3, GL_FLOAT, 0, f.vertexArray.data());
        glDrawArrays(GL_TRIANGLE_FAN, 0, f.vertexArray.size());
    }
    if (!pickMode) {
        // cube borders
        glLineWidth(m_BorderWidth);
        for (const auto& pair : m_Faces) {
            auto f = pair.second;
            if (f.type == ShapeId::Button)
                continue;
            glColor4f(cb.redF(), cb.greenF(), cb.blueF(), cb.alphaF() * opacity);
            glVertexPointer(3, GL_FLOAT, 0, f.vertexArray.data());
            glDrawArrays(GL_LINES, 0, f.vertexArray.size());
        }

        // Label textures
        glDisable(GL_POLYGON_OFFSET_FILL); // make sure labels are on top
        glEnable(GL_TEXTURE_2D);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        float texCoords[] = {0.f,0.f,1.f,0.f,1.f,1.f,0.f,1.f};
        glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
        QColor& c = m_EmphaseColor;
        glColor4f(c.redF(), c.greenF(), c.blueF(), c.alphaF() * opacity);
        for (const auto& pair : m_LabelTextures) {
            auto f = pair.second;
            PickId pickId = pair.first;
            glVertexPointer(3, GL_FLOAT, 0, m_LabelTextures[pickId].vertexArray.data());
            glBindTexture(GL_TEXTURE_2D, f.texture->textureId());
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        }
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_POLYGON_OFFSET_FILL);
    }

    // Draw the flat buttons
    glDisable(GL_CULL_FACE);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 1.0, 1.0, 0.0, 0.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    for (const auto& pair : m_Faces) {
        auto f = pair.second;
        if (f.type != ShapeId::Button)
            continue;
        PickId pickId = pair.first;
        if (pickMode) {
            glColor3ub(static_cast<GLubyte>(pickId), 0, 0);
        }
        else {
            QColor& c = m_HiliteId == pickId ? m_HiliteColor : m_BaseColor;
            glColor4f(c.redF(), c.greenF(), c.blueF(), c.alphaF() * opacity);
        }
        glVertexPointer(3, GL_FLOAT, 0, f.vertexArray.data());
        glDrawArrays(GL_TRIANGLE_FAN, 0, f.vertexArray.size());
        if (!pickMode) {
            glColor4f(cb.redF(), cb.greenF(), cb.blueF(), cb.alphaF() * opacity);
            glDrawArrays(GL_LINE_LOOP, 0, f.vertexArray.size());
        }
    }

    // Restore original state.
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
}

NaviCubeImplementation::PickId NaviCubeImplementation::pickFace(short x, short y) {
    GLubyte pixels[4] = {0};
    if (m_PickingFramebuffer && std::abs(x) <= m_CubeWidgetSize / 2 &&
        std::abs(y) <= m_CubeWidgetSize / 2) {
        m_PickingFramebuffer->bind();

        glViewport(0, 0, m_CubeWidgetSize * 2, m_CubeWidgetSize * 2);

        drawNaviCube(true, 1.f);

        glFinish();
        glReadPixels(2 * x + m_CubeWidgetSize, 2 * y + m_CubeWidgetSize, 1, 1,
                     GL_RGBA, GL_UNSIGNED_BYTE, &pixels);
        m_PickingFramebuffer->release();
    }
    return pixels[3] == 255 ? static_cast<PickId>(pixels[0]) : PickId::None;
}

bool NaviCubeImplementation::mousePressed(short x, short y) {
    m_MouseDown = true;
    m_MightDrag = inDragZone(x, y);
    PickId pick = pickFace(x, y);
    setHilite(pick);
    return pick != PickId::None;
}

void NaviCubeImplementation::handleMenu() {
    m_Menu->exec(QCursor::pos());
}

SbRotation NaviCubeImplementation::getNearestOrientation(PickId pickId) {
    SbRotation cameraOrientation = m_View3DInventorViewer->getCameraOrientation();
    SbRotation standardOrientation = m_Faces[pickId].rotation;

    SbVec3f cameraZ;
    cameraOrientation.multVec(SbVec3f(0, 0, 1), cameraZ);

    SbVec3f standardZ;
    standardOrientation.multVec(SbVec3f(0, 0, 1), standardZ);

    // Cleanup near zero values
    for (int i = 0; i < 3; i++) {
        if (abs(standardZ[i]) < 1e-6) standardZ[i] = 0.0F;
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

    static const float pi = boost::math::constants::pi<float>();
    static const float pi2 = boost::math::constants::two_pi<float>();
    static const float pi1_2 = boost::math::constants::half_pi<float>();
    static const float pi1_3 = boost::math::constants::third_pi<float>();
    static const float pi2_3 = boost::math::constants::two_thirds_pi<float>();

    // Make angle positive
    if (angle < 0) {
        angle += pi2;
    }

    // f is a small value used to control orientation priority when the camera is almost exactly between two
    // orientations (e.g. +45 and -45 degrees). The standard orientation is preferred compared to
    // +90 and -90 degree orientations and the +90 and -90 degree orientations are preferred compared to an
    // upside down standard orientation
    float f = 0.00001F;

    // Find the angle to rotate to the nearest orientation
    if (m_Faces[pickId].type == ShapeId::Corner) {
        // 6 possible orientations for the corners
        if (angle <= (M_PI / 6 + f)) {
            angle = 0;
        }
        else if (angle <= (M_PI_2 + f)) {
            angle = pi1_3;
        }
        else if (angle < (5 * M_PI / 6 - f)) {
            angle = pi2_3;
        }
        else if (angle <= (M_PI + M_PI / 6 + f)) {
            angle = pi;
        }
        else if (angle < (M_PI + M_PI_2 - f)) {
            angle = pi + pi1_3;
        }
        else if (angle < (M_PI + 5 * M_PI / 6 - f)) {
            angle = pi + pi2_3;
        }
        else {
            angle = 0;
        }
    }
    else {
        // 4 possible orientations for the main and edge faces
        if (angle <= (M_PI_4 + f)) {
            angle = 0;
        }
        else if (angle <= (3 * M_PI_4 + f)) {
            angle = pi1_2;
        }
        else if (angle < (M_PI + M_PI_4 - f)) {
            angle = pi;
        }
        else if (angle < (M_PI + 3 * M_PI_4 - f)) {
            angle = pi + pi1_2;
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
    } else {
        PickId pickId = pickFace(x, y);
        long step = Base::clamp(long(m_NaviStepByTurn), 4L, 36L);
        float rotStepAngle = (2 * M_PI) / step;

        if (m_Faces[pickId].type == ShapeId::Main || m_Faces[pickId].type == ShapeId::Edge || m_Faces[pickId].type == ShapeId::Corner) {
            // Handle the cube faces
            SbRotation orientation;
            if (m_RotateToNearest) {
                orientation = getNearestOrientation(pickId);
            }
            else {
                orientation = m_Faces[pickId].rotation;
            }
            m_View3DInventorViewer->setCameraOrientation(orientation);
        }
        else if (m_Faces[pickId].type == ShapeId::Button) {

            // Handle the menu
            if (pickId == PickId::ViewMenu) {
                handleMenu();
                return true;
            }

            // Handle the flat buttons
            SbRotation rotation = m_Faces[pickId].rotation;
            if (pickId == PickId::DotBackside) {
                rotation.scaleAngle(pi);
            }
            else {
                rotation.scaleAngle(rotStepAngle);
            }
            m_View3DInventorViewer->setCameraOrientation(rotation * m_View3DInventorViewer->getCameraOrientation());
        }
        else {
            return false;
        }
    }
    return true;
}

void NaviCubeImplementation::setHilite(PickId hilite) {
    if (hilite != m_HiliteId) {
        m_HiliteId = hilite;
        m_View3DInventorViewer->getSoRenderManager()->scheduleRedraw();
    }
}

bool NaviCubeImplementation::inDragZone(short x, short y) {
    int limit = m_CubeWidgetSize / 4;
    return std::abs(x) < limit && std::abs(y) < limit;
}

bool NaviCubeImplementation::mouseMoved(short x, short y) {
    bool hovering = std::abs(x) <= m_CubeWidgetSize / 2 &&
            std::abs(y) <= m_CubeWidgetSize / 2;

    if (hovering != m_Hovering) {
        m_Hovering = hovering;
        m_View3DInventorViewer->getSoRenderManager()->scheduleRedraw();
    }

    if (!m_Dragging)
        setHilite(pickFace(x, y));

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

bool NaviCubeImplementation::processSoEvent(const SoEvent* ev) {
    short x, y;
    ev->getPosition().getValue(x, y);
    // translate to internal cube center based coordinates
    short rx = x - (short)(m_PosAreaSize[0]*m_RelPos[0]) - m_PosAreaBase[0];
    short ry = y - (short)(m_PosAreaSize[1]*m_RelPos[1]) - m_PosAreaBase[1];
    if (ev->getTypeId().isDerivedFrom(SoMouseButtonEvent::getClassTypeId())) {
        const auto mbev = static_cast<const SoMouseButtonEvent*>(ev);
        if (mbev->isButtonPressEvent(mbev, SoMouseButtonEvent::BUTTON1))
            return mousePressed(rx, ry);
        if (mbev->isButtonReleaseEvent(mbev, SoMouseButtonEvent::BUTTON1))
            return mouseReleased(rx, ry);
    }
    if (ev->getTypeId().isDerivedFrom(SoLocation2Event::getClassTypeId())) {
        return mouseMoved(rx, ry);
    }
    return false;
}

QString NaviCubeImplementation::str(const char* str) {
    return QString::fromLatin1(str);
}

void NaviCube::setNaviCubeCommands(const std::vector<std::string>& cmd)
{
    NaviCubeImplementation::m_commands = cmd;
}

DEF_STD_CMD_AC(NaviCubeDraggableCmd)

NaviCubeDraggableCmd::NaviCubeDraggableCmd()
    : Command("NaviCubeDraggableCmd")
{
    sGroup        = "";
    sMenuText     = QT_TR_NOOP("Movable navigation cube");
    sToolTipText  = QT_TR_NOOP("Drag and place NaviCube");
    sWhatsThis    = "";
    sStatusTip    = sToolTipText;
    eType         = Alter3DView;
}
void NaviCubeDraggableCmd::activated(int iMsg)
{
    auto view = qobject_cast<View3DInventor*>(getMainWindow()->activeWindow());
    view->getViewer()->getNaviCube()->setDraggable(iMsg == 1 ? true : false);
}
bool NaviCubeDraggableCmd::isActive()
{
    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        bool check = _pcAction->isChecked();
        auto view = qobject_cast<View3DInventor*>(getMainWindow()->activeWindow());
        bool mode = view->getViewer()->getNaviCube()->isDraggable();
        if (mode != check)
            _pcAction->setChecked(mode);
        return true;
    }
    return false;
}
Gui::Action * NaviCubeDraggableCmd::createAction()
{
    Gui::Action *pcAction = Command::createAction();
    pcAction->setCheckable(true);
    return pcAction;
}


QMenu* NaviCubeImplementation::createNaviCubeMenu() {
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
        commands.emplace_back("Separator");
        commands.emplace_back("NaviCubeDraggableCmd");
    }

    for (const auto & command : commands) {
        if (command == "Separator") {
            menu->addSeparator();
        }
        else {
            Command* cmd = rcCmdMgr.getCommandByName(command.c_str());
            if (cmd)
                cmd->addTo(menu);
        }
    }
    return menu;
}
