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



struct Face {
    int type;
    vector<Vector3f> vertexArray;
};
struct LabelTexture {
    vector<Vector3f> vertexArray;
    qreal fontSize;
    QOpenGLTexture *texture = nullptr;
    string label;
};


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
    bool mousePressed(short x, short y);
    bool mouseReleased(short x, short y);
    bool mouseMoved(short x, short y);
    int pickFace(short x, short y);
    bool inDragZone(short x, short y);

    void prepare();
    void handleResize();
    void handleMenu();

    void setHilite(int);

    void addCubeFace(const Vector3f&, const Vector3f&, int, int);

    void addButtonFaceTex(QtGLWidget*, int);
    void addButtonFace(int);

    SbRotation setView(float, float) const;
    SbRotation rotateView(SbRotation, int axis, float rotAngle, SbVec3f customAxis = SbVec3f(0, 0, 0)) const;
    void rotateView(const SbRotation&);

    QString str(const char* str);
    QMenu* createNaviCubeMenu();

public:
    enum {
        PID_NONE,
        PID_FRONT,
        PID_TOP,
        PID_RIGHT,
        PID_REAR,
        PID_BOTTOM,
        PID_LEFT,
        PID_FRONT_TOP,
        PID_FRONT_BOTTOM,
        PID_FRONT_RIGHT,
        PID_FRONT_LEFT,
        PID_REAR_TOP,
        PID_REAR_BOTTOM,
        PID_REAR_RIGHT,
        PID_REAR_LEFT,
        PID_TOP_RIGHT,
        PID_TOP_LEFT,
        PID_BOTTOM_RIGHT,
        PID_BOTTOM_LEFT,
        PID_FRONT_TOP_RIGHT,
        PID_FRONT_TOP_LEFT,
        PID_FRONT_BOTTOM_RIGHT,
        PID_FRONT_BOTTOM_LEFT,
        PID_REAR_TOP_RIGHT,
        PID_REAR_TOP_LEFT,
        PID_REAR_BOTTOM_RIGHT,
        PID_REAR_BOTTOM_LEFT,
        PID_ARROW_NORTH,
        PID_ARROW_SOUTH,
        PID_ARROW_EAST,
        PID_ARROW_WEST,
        PID_ARROW_RIGHT,
        PID_ARROW_LEFT,
        PID_DOT_BACKSIDE,
        PID_VIEW_MENU
    };
    enum {
        DIR_UP, DIR_RIGHT, DIR_OUT
    };
    enum {
        SHAPE_NONE, SHAPE_MAIN, SHAPE_EDGE, SHAPE_CORNER, SHAPE_BUTTON
    };
    Gui::View3DInventorViewer* m_View3DInventorViewer;
    void drawNaviCube(bool picking);

    int m_CubeWidgetSize = 132;
    SbVec2f m_RelPos = SbVec2f(1.0f,1.0f);
    SbVec2s m_ViewSize = SbVec2s(0,0);
    SbVec2s m_PosAreaBase = SbVec2s(0,0);
    SbVec2s m_PosAreaSize = SbVec2s(0,0);
    SbVec2s m_PosOffset = SbVec2s(0,0);
    QColor m_BaseColor;
    QColor m_EmphaseColor;
    QColor m_HiliteColor;
    bool m_ShowCS = true;
    int m_HiliteId = 0;
    bool m_MouseDown = false;
    bool m_Dragging = false;
    bool m_Draggable = false;
    bool m_MightDrag = false;
    double m_BorderWidth = 1.1;
    bool m_RotateToNearest = true;
    int m_NaviStepByTurn = 8;
    float m_FontZoom = 0.3;
    float m_Chamfer = 0.12;
    std::string m_TextFont;
    int m_FontWeight = 0;
    int m_FontStretch = 0;

    QtGLFramebufferObject* m_PickingFramebuffer;

    bool m_Prepared = false;

    map<int, Face> m_Faces;
    map<int, LabelTexture> m_LabelTextures;
    static vector<string> m_commands;

    QMenu* m_Menu;
};

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
void NaviCubeImplementation::setLabels(const std::vector<std::string>& labels)
{
    m_LabelTextures[PID_FRONT].label  = labels[0];
    m_LabelTextures[PID_TOP].label    = labels[1];
    m_LabelTextures[PID_RIGHT].label  = labels[2];
    m_LabelTextures[PID_REAR].label   = labels[3];
    m_LabelTextures[PID_BOTTOM].label = labels[4];
    m_LabelTextures[PID_LEFT].label   = labels[5];
    m_Prepared = false;
}


NaviCubeImplementation::NaviCubeImplementation(Gui::View3DInventorViewer* viewer)
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


void NaviCubeImplementation::createCubeFaceTextures() {
    int texSize = 128; // Works well for the max cube size 1024
    // find font sizes
    QFont font;
    QString fontString = QString::fromStdString(m_TextFont);
    font.fromString(fontString);
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
    vector<int> mains = {PID_FRONT, PID_TOP, PID_RIGHT, PID_REAR, PID_BOTTOM, PID_LEFT};
    for (int pickId : mains) {
        auto t = QString::fromUtf8(m_LabelTextures[pickId].label.c_str()); 
        float textlen = fm.horizontalAdvance(t);
        m_LabelTextures[pickId].fontSize = texSize * texSize / textlen;
        minFontSize = std::min(minFontSize, m_LabelTextures[pickId].fontSize);
        maxFontSize = std::max(maxFontSize, m_LabelTextures[pickId].fontSize);
    }
    if (m_FontZoom > 0.0)
        maxFontSize = minFontSize + (maxFontSize - minFontSize) * m_FontZoom;
    else {
        maxFontSize = minFontSize * std::pow(2.0, m_FontZoom);
    }
    for (int pickId : mains) {
        m_LabelTextures[pickId].fontSize = std::min(m_LabelTextures[pickId].fontSize, maxFontSize);
        QImage image(texSize, texSize, QImage::Format_ARGB32);
        image.fill(qRgba(255, 255, 255, 0));
        QPainter paint;
        paint.begin(&image);
        paint.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
        paint.setPen(Qt::white);
        if (m_LabelTextures[pickId].fontSize > 0.5) {
            QString text = QString::fromUtf8(m_LabelTextures[pickId].label.c_str());
            // 5% margin looks nice and prevents some artifacts
            font.setPointSizeF(m_LabelTextures[pickId].fontSize * 0.9); 
            paint.setFont(font);
            paint.drawText(QRect(0, 0, texSize, texSize), Qt::AlignCenter, text);
        }
        paint.end();
        if (m_LabelTextures[pickId].texture) {
            delete m_LabelTextures[pickId].texture;
        }
        m_LabelTextures[pickId].texture = new QOpenGLTexture(image.mirrored());
        m_LabelTextures[pickId].texture->generateMipMaps();
        m_LabelTextures[pickId].texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        m_LabelTextures[pickId].texture->setMagnificationFilter(QOpenGLTexture::Linear);
    }
}

void NaviCubeImplementation::addButtonFace(int pickId)
{
    if (m_Faces[pickId].vertexArray.size())
        m_Faces[pickId].vertexArray.clear();
    float scale = 0.005;
    float offx = 0.5;
    float offy = 0.5;
    vector<float> pointData;

    switch (pickId) {
        default:
            break;
        case PID_ARROW_RIGHT:
        case PID_ARROW_LEFT: {
            pointData = {
                66.6, -66.6,//outer curve
                58.3, -74.0,
                49.2 ,-80.3,
                39.4 ,-85.5,
                29. , -89.5,
                25.3, -78.1,//inner curve
                34.3, -74.3,
                42.8, -69.9,
                50.8, -64.4,
                58.1, -58.1,
                53.8, -53.8,//arrowhead
                74.7, -46.8,
                70.7, -70.4
            };
            break;
        }
        case PID_ARROW_WEST:
        case PID_ARROW_NORTH:
        case PID_ARROW_SOUTH:
        case PID_ARROW_EAST: {
            pointData = {
                100.,  0.,
                 80.,-18.,
                 80., 18.
             };
            break;
        }
        case PID_VIEW_MENU: {
            offx = 0.84;
            offy = 0.84;
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
        case PID_DOT_BACKSIDE: {
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
        if (pickId == PID_ARROW_NORTH || pickId == PID_ARROW_WEST || pickId == PID_ARROW_LEFT)
            x = 1.0 - x;
        if (pickId == PID_ARROW_SOUTH || pickId == PID_ARROW_NORTH)
            m_Faces[pickId].vertexArray.emplace_back(Vector3f(y, x, 0.0));
        else
            m_Faces[pickId].vertexArray.emplace_back(Vector3f(x, y, 0.0));
    }
    m_Faces[pickId].type = SHAPE_BUTTON;
}


void NaviCubeImplementation::addCubeFace( const Vector3f& x, const Vector3f& z, int shapeType, int pickId) {
    m_Faces[pickId].vertexArray.clear();
    m_Faces[pickId].type = shapeType;

    Vector3f y = x.cross(-z);

    if (shapeType == SHAPE_CORNER) {
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
    else if (shapeType == SHAPE_EDGE) {
        auto x4 = x * (1 - m_Chamfer * 4);
        auto yE = y * m_Chamfer;
        auto zE = z * (1 - m_Chamfer);
        m_Faces[pickId].vertexArray.reserve(4);
        m_Faces[pickId].vertexArray.emplace_back(zE - x4 - yE);
        m_Faces[pickId].vertexArray.emplace_back(zE + x4 - yE);
        m_Faces[pickId].vertexArray.emplace_back(zE + x4 + yE);
        m_Faces[pickId].vertexArray.emplace_back(zE - x4 + yE);
    }
    else if (shapeType == SHAPE_MAIN) {
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

void NaviCubeImplementation::prepare() {
    createCubeFaceTextures();

    Vector3f x(1, 0, 0);
    Vector3f y(0, 1, 0);
    Vector3f z(0, 0, 1);

    // create the main faces
    addCubeFace( x, z, SHAPE_MAIN, PID_TOP);
    addCubeFace( x,-y, SHAPE_MAIN, PID_FRONT);
    addCubeFace(-y,-x, SHAPE_MAIN, PID_LEFT);
    addCubeFace(-x, y, SHAPE_MAIN, PID_REAR);
    addCubeFace( y, x, SHAPE_MAIN, PID_RIGHT);
    addCubeFace( x,-z, SHAPE_MAIN, PID_BOTTOM);

    // add edge faces
    addCubeFace(x+y, x-y+z, SHAPE_CORNER, PID_FRONT_TOP_RIGHT);
    addCubeFace(x-y,-x-y+z, SHAPE_CORNER, PID_FRONT_TOP_LEFT);
    addCubeFace(x+y, x-y-z, SHAPE_CORNER, PID_FRONT_BOTTOM_RIGHT);
    addCubeFace(x-y,-x-y-z, SHAPE_CORNER, PID_FRONT_BOTTOM_LEFT);
    addCubeFace(x-y, x+y+z, SHAPE_CORNER, PID_REAR_TOP_RIGHT);
    addCubeFace(x+y,-x+y+z, SHAPE_CORNER, PID_REAR_TOP_LEFT);
    addCubeFace(x-y, x+y-z, SHAPE_CORNER, PID_REAR_BOTTOM_RIGHT);
    addCubeFace(x+y,-x+y-z, SHAPE_CORNER, PID_REAR_BOTTOM_LEFT);

    // add corner faces
    addCubeFace(x, z-y, SHAPE_EDGE, PID_FRONT_TOP);
    addCubeFace(x,-z-y, SHAPE_EDGE, PID_FRONT_BOTTOM);
    addCubeFace(x, y-z, SHAPE_EDGE, PID_REAR_BOTTOM);
    addCubeFace(x, y+z, SHAPE_EDGE, PID_REAR_TOP);
    addCubeFace(z, x+y, SHAPE_EDGE, PID_REAR_RIGHT);
    addCubeFace(z, x-y, SHAPE_EDGE, PID_FRONT_RIGHT);
    addCubeFace(z,-x-y, SHAPE_EDGE, PID_FRONT_LEFT);
    addCubeFace(z, y-x, SHAPE_EDGE, PID_REAR_LEFT);
    addCubeFace(y, z-x, SHAPE_EDGE, PID_TOP_LEFT);
    addCubeFace(y, x+z, SHAPE_EDGE, PID_TOP_RIGHT);
    addCubeFace(y, x-z, SHAPE_EDGE, PID_BOTTOM_RIGHT);
    addCubeFace(y,-z-x, SHAPE_EDGE, PID_BOTTOM_LEFT);

    // create the flat buttons
    addButtonFace(PID_ARROW_NORTH);
    addButtonFace(PID_ARROW_SOUTH);
    addButtonFace(PID_ARROW_EAST);
    addButtonFace(PID_ARROW_WEST);
    addButtonFace(PID_ARROW_LEFT);
    addButtonFace(PID_ARROW_RIGHT);
    addButtonFace(PID_DOT_BACKSIDE);
    addButtonFace(PID_VIEW_MENU);


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
    drawNaviCube(false);
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

void NaviCubeImplementation::drawNaviCube(bool pickMode)
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
    GLfloat depthrange[2];
    glGetFloatv(GL_DEPTH_RANGE, depthrange);
    GLdouble projectionmatrix[16];
    glGetDoublev(GL_PROJECTION_MATRIX, projectionmatrix);

    glDepthMask(GL_TRUE);
    glDepthRange(0.1f, 0.9f);
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glDisable(GL_LIGHTING);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glShadeModel(GL_SMOOTH);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glMatrixMode(GL_PROJECTION);
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
    mx[3][2] = -5.1;

    glLoadMatrixf((float*)mx);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    if (pickMode) {
        glDisable(GL_BLEND);
        glShadeModel(GL_FLAT);
        glDisable(GL_DITHER);
        glDisable(GL_POLYGON_SMOOTH);
    }
    else {
        glEnable(GL_BLEND);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    glClear(GL_DEPTH_BUFFER_BIT);

    glEnableClientState(GL_VERTEX_ARRAY);
    QColor& cb = m_EmphaseColor;

    // Draw coordinate system
    if (!pickMode && m_ShowCS) {
        glLineWidth(m_BorderWidth*2.f);
        glPointSize(m_BorderWidth*2.f);
        float a = -1.1f;
        float b = -1.0f;
        float c =  0.5f;

        float pointData[] = {
            b, a, a, // X1
            c, a, a, // X2
            a, b, a, // Y1
            a, c, a, // Y2
            a, a, b, // Z1
            a, a, c, // Z2
            a, a, a  //0
        };
        glVertexPointer(3, GL_FLOAT, 0, pointData);
        glColor3f(1, 0, 0);
        glDrawArrays(GL_LINES, 0, 2);
        glDrawArrays(GL_POINTS, 0, 2);
        glColor3f(0, 1, 0);
        glDrawArrays(GL_LINES, 2, 2);
        glDrawArrays(GL_POINTS, 2, 2);
        glColor3f(0, 0, 1);
        glDrawArrays(GL_LINES, 4, 2);
        glDrawArrays(GL_POINTS, 4, 2);
    }

    // cube faces
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    for (const auto& pair : m_Faces) {
        auto f = pair.second;
        if (f.type == SHAPE_BUTTON)
            continue;
        auto pickId = pair.first;
        if (pickMode) {
            glColor3ub(pickId, 0, 0);
        }
        else {
            QColor& c = m_HiliteId == pickId ? m_HiliteColor : m_BaseColor;
            glColor4f(c.redF(), c.greenF(), c.blueF(), c.alphaF());
        }
        glVertexPointer(3, GL_FLOAT, 0, f.vertexArray.data());
        glDrawArrays(GL_TRIANGLE_FAN, 0, f.vertexArray.size());
    }
    if (!pickMode) {
        // cube borders
        glDepthRange(0.09f, 0.9f); // make sure borders and labels are on top
        glLineWidth(m_BorderWidth);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        // QColor& cb = m_EmphaseColor;
        for (const auto& pair : m_Faces) {
            auto f = pair.second;
            if (f.type == SHAPE_BUTTON)
                continue;
            glColor4f(cb.redF(), cb.greenF(), cb.blueF(), cb.alphaF());
            glVertexPointer(3, GL_FLOAT, 0, f.vertexArray.data());
            glDrawArrays(GL_POLYGON, 0, f.vertexArray.size());
        }
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        // Label textures
        glEnable(GL_TEXTURE_2D);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        float texCoords[] = {0.f,0.f,1.f,0.f,1.f,1.f,0.f,1.f};
        glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
        QColor& c = m_EmphaseColor;
        glColor4f(c.redF(), c.greenF(), c.blueF(), c.alphaF());
        for (const auto& pair : m_LabelTextures) {
            auto f = pair.second;
            int pickId = pair.first;
            glVertexPointer(3, GL_FLOAT, 0, m_LabelTextures[pickId].vertexArray.data());
            glBindTexture(GL_TEXTURE_2D, f.texture->textureId());
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        }
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisable(GL_TEXTURE_2D);

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
        if (f.type != SHAPE_BUTTON)
            continue;
        auto pickId = pair.first;
        if (pickMode) {
            glColor3ub(pickId, 0, 0);
        }
        else {
            QColor& c = m_HiliteId == pickId ? m_HiliteColor : m_BaseColor;
            glColor4f(c.redF(), c.greenF(), c.blueF(), c.alphaF());
        }
        glVertexPointer(3, GL_FLOAT, 0, f.vertexArray.data());
        glDrawArrays(GL_TRIANGLE_FAN, 0, f.vertexArray.size());
        if (!pickMode) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glColor4f(cb.redF(), cb.greenF(), cb.blueF(), cb.alphaF());
            glDrawArrays(GL_POLYGON, 0, f.vertexArray.size());
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
    }

    // Restore original state.
    glPopMatrix();
    glDepthRange(depthrange[0], depthrange[1]);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixd(projectionmatrix);
    glPopAttrib();
}

int NaviCubeImplementation::pickFace(short x, short y) {
    GLubyte pixels[4] = {0};
    if (m_PickingFramebuffer && std::abs(x) <= m_CubeWidgetSize / 2 &&
        std::abs(y) <= m_CubeWidgetSize / 2) {
        m_PickingFramebuffer->bind();

        glViewport(0, 0, m_CubeWidgetSize * 2, m_CubeWidgetSize * 2);
        glLoadIdentity();

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        drawNaviCube(true);

        glFinish();
        glReadPixels(2 * x + m_CubeWidgetSize, 2 * y + m_CubeWidgetSize, 1, 1,
                     GL_RGBA, GL_UNSIGNED_BYTE, &pixels);
        m_PickingFramebuffer->release();
    }
    return pixels[3] == 255 ? pixels[0] : 0;
}

bool NaviCubeImplementation::mousePressed(short x, short y) {
    m_MouseDown = true;
    m_MightDrag = inDragZone(x, y);
    int pick = pickFace(x, y);
    setHilite(pick);
    return pick != 0;
}

SbRotation NaviCubeImplementation::setView(float rotZ, float rotX) const {
    SbRotation rz, rx, t;
    rz.setValue(SbVec3f(0, 0, 1), rotZ * M_PI / 180);
    rx.setValue(SbVec3f(1, 0, 0), rotX * M_PI / 180);
    return rx * rz;
}

SbRotation NaviCubeImplementation::rotateView(SbRotation viewRot, int axis, float rotAngle, SbVec3f customAxis) const {
    SbVec3f up;
    viewRot.multVec(SbVec3f(0, 1, 0), up);

    SbVec3f out;
    viewRot.multVec(SbVec3f(0, 0, 1), out);

    SbVec3f right;
    viewRot.multVec(SbVec3f(1, 0, 0), right);

    SbVec3f direction;
    switch (axis) {
    default:
        return viewRot;
    case DIR_UP:
        direction = up;
        break;
    case DIR_OUT:
        direction = out;
        break;
    case DIR_RIGHT:
        direction = right;
        break;
    }

    if (customAxis != SbVec3f(0, 0, 0))
        direction = customAxis;

    SbRotation rot(direction, -rotAngle * M_PI / 180.0);
    SbRotation newViewRot = viewRot * rot;
    return newViewRot;
}

void NaviCubeImplementation::rotateView(const SbRotation& rot) {
    m_View3DInventorViewer->setCameraOrientation(rot);
}

void NaviCubeImplementation::handleMenu() {
    m_Menu->exec(QCursor::pos());
}

bool NaviCubeImplementation::mouseReleased(short x, short y) {
    setHilite(0);
    m_MouseDown = false;

    // get the current view
    SbMatrix ViewRotMatrix;
    SbRotation CurrentViewRot = m_View3DInventorViewer->getCameraOrientation();
    CurrentViewRot.getValue(ViewRotMatrix);

    if (m_Dragging) {
        m_Dragging = false;
    } else {
        float rot = 45;
        float tilt = 90 - Base::toDegrees(atan(sqrt(2.0)));
        int pick = pickFace(x, y);
        long step = Base::clamp(long(m_NaviStepByTurn), 4L, 36L);
        float rotStepAngle = 360.0f / step;
        bool applyRotation = true;

        SbRotation viewRot = CurrentViewRot;

        switch (pick) {
        default:
            return false;
            break;
        case PID_FRONT:
            viewRot = setView(0, 90);
            // we don't want to dumb rotate to the same view since depending on from where the user clicked on FRONT
            // we have one of four suitable end positions.
            // we use here the same rotation logic used by other programs using OCC like "CAD Assistant"
            // when current matrix's 0,0 entry is larger than its |1,0| entry, we already have the final result
            // otherwise rotate around y
            if (m_RotateToNearest) {
                if (ViewRotMatrix[0][0] < 0 && abs(ViewRotMatrix[0][0]) >= abs(ViewRotMatrix[1][0]))
                    viewRot = rotateView(viewRot, 2, 180);
                else if (ViewRotMatrix[1][0] > 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
                    viewRot = rotateView(viewRot, 2, 90);
                else if (ViewRotMatrix[1][0] < 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
                    viewRot = rotateView(viewRot, 2, -90);
            }
            break;
        case PID_REAR:
            viewRot = setView(180, 90);
            if (m_RotateToNearest) {
                if (ViewRotMatrix[0][0] > 0 && abs(ViewRotMatrix[0][0]) >= abs(ViewRotMatrix[1][0]))
                    viewRot = rotateView(viewRot, 2, 180);
                else if (ViewRotMatrix[1][0] > 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
                    viewRot = rotateView(viewRot, 2, -90);
                else if (ViewRotMatrix[1][0] < 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
                    viewRot = rotateView(viewRot, 2, 90);
            }
            break;
        case PID_LEFT:
            viewRot = setView(270, 90);
            if (m_RotateToNearest) {
                if (ViewRotMatrix[0][1] > 0 && abs(ViewRotMatrix[0][1]) >= abs(ViewRotMatrix[1][1]))
                    viewRot = rotateView(viewRot, 2, 180);
                else if (ViewRotMatrix[1][1] > 0 && abs(ViewRotMatrix[1][1]) > abs(ViewRotMatrix[0][1]))
                    viewRot = rotateView(viewRot, 2, -90);
                else if (ViewRotMatrix[1][1] < 0 && abs(ViewRotMatrix[1][1]) > abs(ViewRotMatrix[0][1]))
                    viewRot = rotateView(viewRot, 2, 90);
            }
            break;
        case PID_RIGHT:
            viewRot = setView(90, 90);
            if (m_RotateToNearest) {
                if (ViewRotMatrix[0][1] < 0 && abs(ViewRotMatrix[0][1]) >= abs(ViewRotMatrix[1][1]))
                    viewRot = rotateView(viewRot, 2, 180);
                else if (ViewRotMatrix[1][1] > 0 && abs(ViewRotMatrix[1][1]) > abs(ViewRotMatrix[0][1]))
                    viewRot = rotateView(viewRot, 2, 90);
                else if (ViewRotMatrix[1][1] < 0 && abs(ViewRotMatrix[1][1]) > abs(ViewRotMatrix[0][1]))
                    viewRot = rotateView(viewRot, 2, -90);
            }
            break;
        case PID_TOP:
            viewRot = setView(0, 0);
            if (m_RotateToNearest) {
                if (ViewRotMatrix[0][0] < 0 && abs(ViewRotMatrix[0][0]) >= abs(ViewRotMatrix[1][0]))
                    viewRot = rotateView(viewRot, 2, 180);
                else if (ViewRotMatrix[1][0] > 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
                    viewRot = rotateView(viewRot, 2, 90);
                else if (ViewRotMatrix[1][0] < 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
                    viewRot = rotateView(viewRot, 2, -90);
            }
            break;
        case PID_BOTTOM:
            viewRot = setView(0, 180);
            if (m_RotateToNearest) {
                if (ViewRotMatrix[0][0] < 0 && abs(ViewRotMatrix[0][0]) >= abs(ViewRotMatrix[1][0]))
                    viewRot = rotateView(viewRot, 2, 180);
                else if (ViewRotMatrix[1][0] > 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
                    viewRot = rotateView(viewRot, 2, 90);
                else if (ViewRotMatrix[1][0] < 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
                    viewRot = rotateView(viewRot, 2, -90);
            }
            break;
        case PID_FRONT_TOP:
            // set to FRONT then rotate
            viewRot = setView(0, 90);
            viewRot = rotateView(viewRot, 1, 45);
            if (m_RotateToNearest) {
                if (ViewRotMatrix[0][0] < 0 && abs(ViewRotMatrix[0][0]) >= abs(ViewRotMatrix[1][0]))
                    viewRot = rotateView(viewRot, 2, 180);
                else if (ViewRotMatrix[1][0] > 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
                    viewRot = rotateView(viewRot, 2, 90);
                else if (ViewRotMatrix[1][0] < 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
                    viewRot = rotateView(viewRot, 2, -90);
            }
            break;
        case PID_FRONT_BOTTOM:
            // set to FRONT then rotate
            viewRot = setView(0, 90);
            viewRot = rotateView(viewRot, 1, -45);
            if (m_RotateToNearest) {
                if (ViewRotMatrix[0][0] < 0 && abs(ViewRotMatrix[0][0]) >= abs(ViewRotMatrix[1][0]))
                    viewRot = rotateView(viewRot, 2, 180);
                else if (ViewRotMatrix[1][0] > 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
                    viewRot = rotateView(viewRot, 2, 90);
                else if (ViewRotMatrix[1][0] < 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
                    viewRot = rotateView(viewRot, 2, -90);
            }
            break;
        case PID_REAR_BOTTOM:
            // set to REAR then rotate
            viewRot = setView(180, 90);
            viewRot = rotateView(viewRot, 1, -45);
            if (m_RotateToNearest) {
                if (ViewRotMatrix[0][0] > 0 && abs(ViewRotMatrix[0][0]) >= abs(ViewRotMatrix[1][0]))
                    viewRot = rotateView(viewRot, 2, 180);
                else if (ViewRotMatrix[1][0] > 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
                    viewRot = rotateView(viewRot, 2, -90);
                else if (ViewRotMatrix[1][0] < 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
                    viewRot = rotateView(viewRot, 2, 90);
            }
            break;
        case PID_REAR_TOP:
            // set to REAR then rotate
            viewRot = setView(180, 90);
            viewRot = rotateView(viewRot, 1, 45);
            if (m_RotateToNearest) {
                if (ViewRotMatrix[0][0] > 0 && abs(ViewRotMatrix[0][0]) >= abs(ViewRotMatrix[1][0]))
                    viewRot = rotateView(viewRot, 2, 180);
                else if (ViewRotMatrix[1][0] > 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
                    viewRot = rotateView(viewRot, 2, -90);
                else if (ViewRotMatrix[1][0] < 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
                    viewRot = rotateView(viewRot, 2, 90);
            }
            break;
        case PID_FRONT_LEFT:
            // set to FRONT then rotate
            viewRot = setView(0, 90);
            viewRot = rotateView(viewRot, 0, 45);
            if (m_RotateToNearest) {
                if (ViewRotMatrix[1][2] < 0 && abs(ViewRotMatrix[1][2]) >= abs(ViewRotMatrix[0][2]))
                    viewRot = rotateView(viewRot, 2, 180);
                else if (ViewRotMatrix[0][2] > 0 && abs(ViewRotMatrix[0][2]) > abs(ViewRotMatrix[1][2]))
                    viewRot = rotateView(viewRot, 2, -90);
                else if (ViewRotMatrix[0][2] < 0 && abs(ViewRotMatrix[0][2]) > abs(ViewRotMatrix[1][2]))
                    viewRot = rotateView(viewRot, 2, 90);
            }
            break;
        case PID_FRONT_RIGHT:
            // set to FRONT then rotate
            viewRot = setView(0, 90);
            viewRot = rotateView(viewRot, 0, -45);
            if (m_RotateToNearest) {
                if (ViewRotMatrix[1][2] < 0 && abs(ViewRotMatrix[1][2]) >= abs(ViewRotMatrix[0][2]))
                    viewRot = rotateView(viewRot, 2, 180);
                else if (ViewRotMatrix[0][2] > 0 && abs(ViewRotMatrix[0][2]) > abs(ViewRotMatrix[1][2]))
                    viewRot = rotateView(viewRot, 2, -90);
                else if (ViewRotMatrix[0][2] < 0 && abs(ViewRotMatrix[0][2]) > abs(ViewRotMatrix[1][2]))
                    viewRot = rotateView(viewRot, 2, 90);
            }
            break;
        case PID_REAR_RIGHT:
            // set to REAR then rotate
            viewRot = setView(180, 90);
            viewRot = rotateView(viewRot, 0, 45);
            if (m_RotateToNearest) {
                if (ViewRotMatrix[1][2] < 0 && abs(ViewRotMatrix[1][2]) >= abs(ViewRotMatrix[0][2]))
                    viewRot = rotateView(viewRot, 2, 180);
                else if (ViewRotMatrix[0][2] > 0 && abs(ViewRotMatrix[0][2]) > abs(ViewRotMatrix[1][2]))
                    viewRot = rotateView(viewRot, 2, -90);
                else if (ViewRotMatrix[0][2] < 0 && abs(ViewRotMatrix[0][2]) > abs(ViewRotMatrix[1][2]))
                    viewRot = rotateView(viewRot, 2, 90);
            }
            break;
        case PID_REAR_LEFT:
            // set to REAR then rotate
            viewRot = setView(180, 90);
            viewRot = rotateView(viewRot, 0, -45);
            if (ViewRotMatrix[1][2] < 0 && abs(ViewRotMatrix[1][2]) >= abs(ViewRotMatrix[0][2]))
                viewRot = rotateView(viewRot, 2, 180);
            else if (ViewRotMatrix[0][2] > 0 && abs(ViewRotMatrix[0][2]) > abs(ViewRotMatrix[1][2]))
                viewRot = rotateView(viewRot, 2, -90);
            else if (ViewRotMatrix[0][2] < 0 && abs(ViewRotMatrix[0][2]) > abs(ViewRotMatrix[1][2]))
                viewRot = rotateView(viewRot, 2, 90);
            break;
        case PID_TOP_LEFT:
            // set to LEFT then rotate
            viewRot = setView(270, 90);
            viewRot = rotateView(viewRot, 1, 45);
            if (m_RotateToNearest) {
                if (ViewRotMatrix[0][1] > 0 && abs(ViewRotMatrix[0][1]) >= abs(ViewRotMatrix[1][1]))
                    viewRot = rotateView(viewRot, 2, 180);
                else if (ViewRotMatrix[1][1] > 0 && abs(ViewRotMatrix[1][1]) > abs(ViewRotMatrix[0][1]))
                    viewRot = rotateView(viewRot, 2, -90);
                else if (ViewRotMatrix[1][1] < 0 && abs(ViewRotMatrix[1][1]) > abs(ViewRotMatrix[0][1]))
                    viewRot = rotateView(viewRot, 2, 90);
            }
            break;
        case PID_TOP_RIGHT:
            // set to RIGHT then rotate
            viewRot = setView(90, 90);
            viewRot = rotateView(viewRot, 1, 45);
            if (m_RotateToNearest) {
                if (ViewRotMatrix[0][1] < 0 && abs(ViewRotMatrix[0][1]) >= abs(ViewRotMatrix[1][1]))
                    viewRot = rotateView(viewRot, 2, 180);
                else if (ViewRotMatrix[1][1] > 0 && abs(ViewRotMatrix[1][1]) > abs(ViewRotMatrix[0][1]))
                    viewRot = rotateView(viewRot, 2, 90);
                else if (ViewRotMatrix[1][1] < 0 && abs(ViewRotMatrix[1][1]) > abs(ViewRotMatrix[0][1]))
                    viewRot = rotateView(viewRot, 2, -90);
            }
            break;
        case PID_BOTTOM_RIGHT:
            // set to RIGHT then rotate
            viewRot = setView(90, 90);
            viewRot = rotateView(viewRot, 1, -45);
            if (m_RotateToNearest) {
                if (ViewRotMatrix[0][1] < 0 && abs(ViewRotMatrix[0][1]) >= abs(ViewRotMatrix[1][1]))
                    viewRot = rotateView(viewRot, 2, 180);
                else if (ViewRotMatrix[1][1] > 0 && abs(ViewRotMatrix[1][1]) > abs(ViewRotMatrix[0][1]))
                    viewRot = rotateView(viewRot, 2, 90);
                else if (ViewRotMatrix[1][1] < 0 && abs(ViewRotMatrix[1][1]) > abs(ViewRotMatrix[0][1]))
                    viewRot = rotateView(viewRot, 2, -90);
            }
            break;
        case PID_BOTTOM_LEFT:
            // set to LEFT then rotate
            viewRot = setView(270, 90);
            viewRot = rotateView(viewRot, 1, -45);
            if (m_RotateToNearest) {
                if (ViewRotMatrix[0][1] > 0 && abs(ViewRotMatrix[0][1]) >= abs(ViewRotMatrix[1][1]))
                    viewRot = rotateView(viewRot, 2, 180);
                else if (ViewRotMatrix[1][1] > 0 && abs(ViewRotMatrix[1][1]) > abs(ViewRotMatrix[0][1]))
                    viewRot = rotateView(viewRot, 2, -90);
                else if (ViewRotMatrix[1][1] < 0 && abs(ViewRotMatrix[1][1]) > abs(ViewRotMatrix[0][1]))
                    viewRot = rotateView(viewRot, 2, 90);
            }
            break;
        case PID_FRONT_BOTTOM_LEFT:
            viewRot = setView(rot - 90, 90 + tilt);
            // we have 3 possible end states:
            // - z-axis is not rotated larger than 120 deg from (0, 1, 0) -> we are already there
            // - y-axis is not rotated larger than 120 deg from (0, 1, 0)
            // - x-axis is not rotated larger than 120 deg from (0, 1, 0)
            if (m_RotateToNearest) {
                if (ViewRotMatrix[1][0] > 0.4823)
                    viewRot = rotateView(viewRot, 0, -120, SbVec3f(1, 1, 1));
                else if (ViewRotMatrix[1][1] > 0.4823)
                    viewRot = rotateView(viewRot, 0, 120, SbVec3f(1, 1, 1));
            }
            break;
        case PID_FRONT_BOTTOM_RIGHT:
            viewRot = setView(90 + rot - 90, 90 + tilt);
            if (m_RotateToNearest) {
                if (ViewRotMatrix[1][0] < -0.4823)
                    viewRot = rotateView(viewRot, 0, 120, SbVec3f(-1, 1, 1));
                else if (ViewRotMatrix[1][1] > 0.4823)
                    viewRot = rotateView(viewRot, 0, -120, SbVec3f(-1, 1, 1));
            }
            break;
        case PID_REAR_BOTTOM_RIGHT:
            viewRot = setView(180 + rot - 90, 90 + tilt);
            if (m_RotateToNearest) {
                if (ViewRotMatrix[1][0] < -0.4823)
                    viewRot = rotateView(viewRot, 0, -120, SbVec3f(-1, -1, 1));
                else if (ViewRotMatrix[1][1] < -0.4823)
                    viewRot = rotateView(viewRot, 0, 120, SbVec3f(-1, -1, 1));
            }
            break;
        case PID_REAR_BOTTOM_LEFT:
            viewRot = setView(270 + rot - 90, 90 + tilt);
            if (m_RotateToNearest) {
                if (ViewRotMatrix[1][0] > 0.4823)
                    viewRot = rotateView(viewRot, 0, 120, SbVec3f(1, -1, 1));
                else if (ViewRotMatrix[1][1] < -0.4823)
                    viewRot = rotateView(viewRot, 0, -120, SbVec3f(1, -1, 1));
            }
            break;
        case PID_FRONT_TOP_RIGHT:
            viewRot = setView(rot, 90 - tilt);
            if (m_RotateToNearest) {
                if (ViewRotMatrix[1][0] > 0.4823)
                    viewRot = rotateView(viewRot, 0, -120, SbVec3f(-1, 1, -1));
                else if (ViewRotMatrix[1][1] < -0.4823)
                    viewRot = rotateView(viewRot, 0, 120, SbVec3f(-1, 1, -1));
            }
            break;
        case PID_FRONT_TOP_LEFT:
            viewRot = setView(rot - 90, 90 - tilt);
            if (m_RotateToNearest) {
                if (ViewRotMatrix[1][0] < -0.4823)
                    viewRot = rotateView(viewRot, 0, 120, SbVec3f(1, 1, -1));
                else if (ViewRotMatrix[1][1] < -0.4823)
                    viewRot = rotateView(viewRot, 0, -120, SbVec3f(1, 1, -1));
            }
            break;
        case PID_REAR_TOP_LEFT:
            viewRot = setView(rot - 180, 90 - tilt);
            if (m_RotateToNearest) {
                if (ViewRotMatrix[1][0] < -0.4823)
                    viewRot = rotateView(viewRot, 0, -120, SbVec3f(1, -1, -1));
                else if (ViewRotMatrix[1][1] > 0.4823)
                    viewRot = rotateView(viewRot, 0, 120, SbVec3f(1, -1, -1));
            }
            break;
        case PID_REAR_TOP_RIGHT:
            viewRot = setView(rot - 270, 90 - tilt);
            if (m_RotateToNearest) {
                if (ViewRotMatrix[1][0] > 0.4823)
                    viewRot = rotateView(viewRot, 0, 120, SbVec3f(-1, -1, -1));
                else if (ViewRotMatrix[1][1] > 0.4823)
                    viewRot = rotateView(viewRot, 0, -120, SbVec3f(-1, -1, -1));
            }
            break;
        case PID_ARROW_LEFT:
            viewRot = rotateView(viewRot, DIR_OUT, rotStepAngle);
            break;
        case PID_ARROW_RIGHT:
            viewRot = rotateView(viewRot, DIR_OUT, -rotStepAngle);
            break;
        case PID_ARROW_WEST:
            viewRot = rotateView(viewRot, DIR_UP, -rotStepAngle);
            break;
        case PID_ARROW_EAST:
            viewRot = rotateView(viewRot, DIR_UP, rotStepAngle);
            break;
        case PID_ARROW_NORTH:
            viewRot = rotateView(viewRot, DIR_RIGHT, -rotStepAngle);
            break;
        case PID_ARROW_SOUTH:
            viewRot = rotateView(viewRot, DIR_RIGHT, rotStepAngle);
            break;
        case PID_DOT_BACKSIDE:
            viewRot = rotateView(viewRot, DIR_UP, 180);
            break;
        case PID_VIEW_MENU:
            handleMenu();
            applyRotation = false;
            break;
        }

        if (applyRotation)
            rotateView(viewRot);
    }
    return true;
}

void NaviCubeImplementation::setHilite(int hilite) {
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
    if (!m_Dragging)
        setHilite(pickFace(x, y));

    if (m_MouseDown && m_Draggable) {
        if (m_MightDrag && !m_Dragging) {
            m_Dragging = true;
            setHilite(0);
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
bool NaviCubeDraggableCmd::isActive(void)
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
