// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2013 Werner Mayer <wmayer[at]users.sourceforge.net>
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

#include <algorithm>

#include <QOpenGLWidget>

#include <Inventor/SbViewportRegion.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/nodes/SoDepthBuffer.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoVertexProperty.h>

#include "GLPainter.h"
#include "Quarter/QuarterWidget.h"
#include "View3DInventorViewer.h"


using namespace Gui;

TYPESYSTEM_SOURCE_ABSTRACT(Gui::GLGraphicsItem, Base::BaseClass)

namespace
{
float clamp01(float value)
{
    return std::clamp(value, 0.0f, 1.0f);
}

SbVec3f toOverlayPoint(int x, int y, int width, int height)
{
    return {
        static_cast<float>(x) - 0.5f * static_cast<float>(width),
        0.5f * static_cast<float>(height) - static_cast<float>(y),
        0.0f,
    };
}

SoSeparator* createOverlayRoot(int width, int height)
{
    auto* root = new SoSeparator;
    root->ref();

    auto* camera = new SoOrthographicCamera;
    camera->aspectRatio.setValue(static_cast<float>(width) / static_cast<float>(height));
    camera->height.setValue(static_cast<float>(height));
    root->addChild(camera);

    auto* depth = new SoDepthBuffer;
    depth->test.setValue(false);
    depth->write.setValue(false);
    depth->function.setValue(SoDepthBuffer::ALWAYS);
    root->addChild(depth);

    auto* lightModel = new SoLightModel;
    lightModel->model.setValue(SoLightModel::BASE_COLOR);
    root->addChild(lightModel);

    return root;
}

void setOverlayCacheContext(
    SoGLRenderAction& action,
    const QOpenGLWidget* widget,
    const View3DInventorViewer* viewer
)
{
    if (viewer && viewer->getSoRenderManager()) {
        if (auto* glra = viewer->getSoRenderManager()->getGLRenderAction()) {
            action.setCacheContext(glra->getCacheContext());
            return;
        }
    }

    for (QObject* current = const_cast<QOpenGLWidget*>(widget); current; current = current->parent()) {
        if (auto* quarter = qobject_cast<SIM::Coin3D::Quarter::QuarterWidget*>(current)) {
            action.setCacheContext(quarter->getCacheContextId());
            return;
        }
    }
}

void applyOverlay(
    SoNode* root,
    int width,
    int height,
    const QOpenGLWidget* widget,
    const View3DInventorViewer* viewer
)
{
    SoGLRenderAction action(SbViewportRegion(width, height));
    setOverlayCacheContext(action, widget, viewer);
    action.setTransparencyType(SoGLRenderAction::BLEND);
    action.apply(root);
}

}  // namespace

GLPainter::GLPainter()
{}

GLPainter::~GLPainter()
{
    end();
}

bool GLPainter::begin(QPaintDevice* device)
{
    if (glWidget) {
        return false;
    }

    glWidget = dynamic_cast<QOpenGLWidget*>(device);
    if (!glWidget) {
        return false;
    }

    viewer = qobject_cast<View3DInventorViewer*>(glWidget);

    if (viewer && viewer->getSoRenderManager()) {
        const SbViewportRegion vp = viewer->getSoRenderManager()->getViewportRegion();
        const SbVec2s size = vp.getViewportSizePixels();
        width = size[0];
        height = size[1];
    }
    else {
        const QSize view = glWidget->size();
        const qreal dpr = glWidget->devicePixelRatioF();
        width = static_cast<int>(view.width() * dpr);
        height = static_cast<int>(view.height() * dpr);
    }

    if (width <= 0 || height <= 0) {
        glWidget = nullptr;
        viewer = nullptr;
        return false;
    }

    glWidget->makeCurrent();

    lineWidth = 1.0f;
    pointSize = 1.0f;
    colorR = 1.0f;
    colorG = 1.0f;
    colorB = 1.0f;
    transparency = 0.0f;
    logicOp = false;
    lineStipple = false;
    lineStippleFactor = 1;
    lineStipplePattern = 0xFFFF;

    return true;
}

bool GLPainter::end()
{
    if (!glWidget) {
        return false;
    }

    glWidget = nullptr;
    viewer = nullptr;
    return true;
}

bool GLPainter::isActive() const
{
    return glWidget != nullptr;
}

void GLPainter::setLineWidth(float w)
{
    lineWidth = w;
}

void GLPainter::setPointSize(float s)
{
    pointSize = s;
}

void GLPainter::setColor(float r, float g, float b, float a)
{
    colorR = clamp01(r);
    colorG = clamp01(g);
    colorB = clamp01(b);
    transparency = 1.0f - clamp01(a);
}

void GLPainter::setLogicOp(GLenum mode)
{
    (void)mode;
    this->logicOp = true;
}

void GLPainter::resetLogicOp()
{
    this->logicOp = false;
}

void GLPainter::setDrawBuffer(GLenum mode)
{
    (void)mode;
}

void GLPainter::setLineStipple(GLint factor, GLushort pattern)
{
    lineStippleFactor = factor;
    lineStipplePattern = pattern;
    this->lineStipple = true;
}

void GLPainter::resetLineStipple()
{
    this->lineStipple = false;
}

// Draw routines
void GLPainter::drawRect(int x1, int y1, int x2, int y2)
{
    if (!glWidget) {
        return;
    }

    SoSeparator* root = createOverlayRoot(width, height);

    auto* material = new SoMaterial;
    material->diffuseColor.setValue(colorR, colorG, colorB);
    material->transparency.setValue(transparency);
    root->addChild(material);

    auto* drawStyle = new SoDrawStyle;
    drawStyle->lineWidth.setValue(lineWidth);
    if (lineStipple) {
        drawStyle->linePatternScaleFactor.setValue(lineStippleFactor);
        drawStyle->linePattern.setValue(lineStipplePattern);
    }
    root->addChild(drawStyle);

    auto* vp = new SoVertexProperty;
    vp->vertex.set1Value(0, toOverlayPoint(x1, y1, width, height));
    vp->vertex.set1Value(1, toOverlayPoint(x2, y1, width, height));
    vp->vertex.set1Value(2, toOverlayPoint(x2, y2, width, height));
    vp->vertex.set1Value(3, toOverlayPoint(x1, y2, width, height));
    vp->vertex.set1Value(4, toOverlayPoint(x1, y1, width, height));

    auto* lineSet = new SoLineSet;
    lineSet->vertexProperty.setValue(vp);
    lineSet->numVertices.setValue(5);
    root->addChild(lineSet);

    applyOverlay(root, width, height, glWidget, viewer);
    root->unref();
}

void GLPainter::drawLine(int x1, int y1, int x2, int y2)
{
    if (!glWidget) {
        return;
    }

    SoSeparator* root = createOverlayRoot(width, height);

    auto* material = new SoMaterial;
    material->diffuseColor.setValue(colorR, colorG, colorB);
    material->transparency.setValue(transparency);
    root->addChild(material);

    auto* drawStyle = new SoDrawStyle;
    drawStyle->lineWidth.setValue(lineWidth);
    if (lineStipple) {
        drawStyle->linePatternScaleFactor.setValue(lineStippleFactor);
        drawStyle->linePattern.setValue(lineStipplePattern);
    }
    root->addChild(drawStyle);

    auto* vp = new SoVertexProperty;
    vp->vertex.set1Value(0, toOverlayPoint(x1, y1, width, height));
    vp->vertex.set1Value(1, toOverlayPoint(x2, y2, width, height));

    auto* lineSet = new SoLineSet;
    lineSet->vertexProperty.setValue(vp);
    lineSet->numVertices.setValue(2);
    root->addChild(lineSet);

    applyOverlay(root, width, height, glWidget, viewer);
    root->unref();
}

void GLPainter::drawPoint(int x, int y)
{
    if (!glWidget) {
        return;
    }

    SoSeparator* root = createOverlayRoot(width, height);

    auto* material = new SoMaterial;
    material->diffuseColor.setValue(colorR, colorG, colorB);
    material->transparency.setValue(transparency);
    root->addChild(material);

    auto* drawStyle = new SoDrawStyle;
    drawStyle->pointSize.setValue(pointSize);
    root->addChild(drawStyle);

    auto* vp = new SoVertexProperty;
    vp->vertex.set1Value(0, toOverlayPoint(x, y, width, height));

    auto* pointSet = new SoPointSet;
    pointSet->vertexProperty.setValue(vp);
    pointSet->numPoints.setValue(1);
    root->addChild(pointSet);

    applyOverlay(root, width, height, glWidget, viewer);
    root->unref();
}

//-----------------------------------------------

Rubberband::Rubberband(View3DInventorViewer* v)
    : viewer(v)
{
    x_old = y_old = x_new = y_new = 0;
    working = false;
    stipple = true;

    rgb_r = 1.0f;
    rgb_g = 1.0f;
    rgb_b = 1.0f;
    rgb_a = 1.0f;
}

Rubberband::Rubberband()
    : viewer(nullptr)
{
    x_old = y_old = x_new = y_new = 0;
    working = false;
    stipple = true;

    rgb_r = 0.27f;
    rgb_g = 0.4f;
    rgb_b = 1.0f;
    rgb_a = 1.0f;
}

Rubberband::~Rubberband() = default;

void Rubberband::setWorking(bool on)
{
    working = on;
}

void Rubberband::setViewer(View3DInventorViewer* v)
{
    viewer = v;
}

void Rubberband::setCoords(int x1, int y1, int x2, int y2)
{
    x_old = x1;
    y_old = y1;
    x_new = x2;
    y_new = y2;
}

void Rubberband::setLineStipple(bool on)
{
    stipple = on;
}

void Rubberband::setColor(float r, float g, float b, float a)
{
    rgb_a = a;
    rgb_b = b;
    rgb_g = g;
    rgb_r = r;
}

void Rubberband::paintGL()
{
    if (!working) {
        return;
    }

    if (!viewer) {
        return;
    }

    const SbViewportRegion vp = viewer->getSoRenderManager()->getViewportRegion();
    SbVec2s size = vp.getViewportSizePixels();
    const int viewportWidth = size[0];
    const int viewportHeight = size[1];
    if (viewportWidth <= 0 || viewportHeight <= 0) {
        return;
    }

    SoSeparator* root = createOverlayRoot(viewportWidth, viewportHeight);

    const int xMin = std::min(x_old, x_new);
    const int xMax = std::max(x_old, x_new);
    const int yMin = std::min(y_old, y_new);
    const int yMax = std::max(y_old, y_new);

    {
        auto* sep = new SoSeparator;

        auto* material = new SoMaterial;
        material->diffuseColor.setValue(1.0f, 1.0f, 1.0f);
        material->transparency.setValue(1.0f - 0.5f);
        sep->addChild(material);

        auto* vp = new SoVertexProperty;
        vp->vertex.set1Value(0, toOverlayPoint(xMin, yMin, viewportWidth, viewportHeight));
        vp->vertex.set1Value(1, toOverlayPoint(xMin, yMax, viewportWidth, viewportHeight));
        vp->vertex.set1Value(2, toOverlayPoint(xMax, yMax, viewportWidth, viewportHeight));
        vp->vertex.set1Value(3, toOverlayPoint(xMax, yMin, viewportWidth, viewportHeight));

        auto* faceSet = new SoFaceSet;
        faceSet->vertexProperty.setValue(vp);
        faceSet->numVertices.setValue(4);
        sep->addChild(faceSet);

        root->addChild(sep);
    }

    {
        auto* sep = new SoSeparator;

        auto* material = new SoMaterial;
        material->diffuseColor.setValue(clamp01(rgb_r), clamp01(rgb_g), clamp01(rgb_b));
        material->transparency.setValue(1.0f - clamp01(rgb_a));
        sep->addChild(material);

        auto* drawStyle = new SoDrawStyle;
        drawStyle->lineWidth.setValue(4.0f);
        if (stipple) {
            drawStyle->linePatternScaleFactor.setValue(3);
            drawStyle->linePattern.setValue(0xAAAA);
        }
        sep->addChild(drawStyle);

        auto* vp = new SoVertexProperty;
        vp->vertex.set1Value(0, toOverlayPoint(x_old, y_old, viewportWidth, viewportHeight));
        vp->vertex.set1Value(1, toOverlayPoint(x_old, y_new, viewportWidth, viewportHeight));
        vp->vertex.set1Value(2, toOverlayPoint(x_new, y_new, viewportWidth, viewportHeight));
        vp->vertex.set1Value(3, toOverlayPoint(x_new, y_old, viewportWidth, viewportHeight));
        vp->vertex.set1Value(4, toOverlayPoint(x_old, y_old, viewportWidth, viewportHeight));

        auto* lineSet = new SoLineSet;
        lineSet->vertexProperty.setValue(vp);
        lineSet->numVertices.setValue(5);
        sep->addChild(lineSet);

        root->addChild(sep);
    }

    applyOverlay(root, viewportWidth, viewportHeight, nullptr, viewer);
    root->unref();
}

// -----------------------------------------------------------------------------------

Polyline::Polyline(View3DInventorViewer* v)
    : viewer(v)
{
    x_new = y_new = 0;
    working = false;
    closed = true;
    stippled = false;
    line = 2.0;

    rgb_r = 1.0f;
    rgb_g = 1.0f;
    rgb_b = 1.0f;
    rgb_a = 1.0f;
}

Polyline::Polyline()
    : viewer(nullptr)
{
    x_new = y_new = 0;
    working = false;
    closed = true;
    stippled = false;
    line = 2.0;

    rgb_r = 1.0f;
    rgb_g = 1.0f;
    rgb_b = 1.0f;
    rgb_a = 1.0f;
}

Polyline::~Polyline() = default;

void Polyline::setWorking(bool on)
{
    working = on;
}

bool Polyline::isWorking() const
{
    return working;
}

void Polyline::setViewer(View3DInventorViewer* v)
{
    viewer = v;
}

void Polyline::setCoords(int x, int y)
{
    x_new = x;
    y_new = y;
}

void Polyline::setColor(int r, int g, int b, int a)
{
    rgb_r = r;
    rgb_g = g;
    rgb_b = b;
    rgb_a = a;
}

void Polyline::setClosed(bool c)
{
    closed = c;
}

void Polyline::setCloseStippled(bool c)
{
    stippled = c;
}

void Polyline::setLineWidth(float l)
{
    line = l;
}

void Polyline::addNode(const QPoint& p)
{
    _cNodeVector.push_back(p);
}

void Polyline::popNode()
{
    if (!_cNodeVector.empty()) {
        _cNodeVector.pop_back();
    }
}

void Polyline::clear()
{
    _cNodeVector.clear();
}

void Polyline::paintGL()
{
    if (!working) {
        return;
    }

    if (!viewer) {
        return;
    }

    if (_cNodeVector.empty()) {
        return;
    }

    const SbViewportRegion vp = viewer->getSoRenderManager()->getViewportRegion();
    SbVec2s size = vp.getViewportSizePixels();
    const int viewportWidth = size[0];
    const int viewportHeight = size[1];
    if (viewportWidth <= 0 || viewportHeight <= 0) {
        return;
    }

    SoSeparator* root = createOverlayRoot(viewportWidth, viewportHeight);

    auto* material = new SoMaterial;
    material->diffuseColor.setValue(clamp01(rgb_r), clamp01(rgb_g), clamp01(rgb_b));
    material->transparency.setValue(1.0f - clamp01(rgb_a));
    root->addChild(material);

    auto* drawStyle = new SoDrawStyle;
    drawStyle->lineWidth.setValue(line);
    root->addChild(drawStyle);

    const int count = static_cast<int>(_cNodeVector.size());
    if (count <= 1) {
        root->unref();
        return;
    }

    if (closed && !stippled) {
        auto* vp = new SoVertexProperty;
        for (int i = 0; i < count; ++i) {
            const QPoint& pnt = _cNodeVector[static_cast<size_t>(i)];
            vp->vertex.set1Value(i, toOverlayPoint(pnt.x(), pnt.y(), viewportWidth, viewportHeight));
        }
        const QPoint& first = _cNodeVector.front();
        vp->vertex.set1Value(count, toOverlayPoint(first.x(), first.y(), viewportWidth, viewportHeight));

        auto* lineSet = new SoLineSet;
        lineSet->vertexProperty.setValue(vp);
        lineSet->numVertices.setValue(count + 1);
        root->addChild(lineSet);
    }
    else {
        auto* vp = new SoVertexProperty;
        for (int i = 0; i < count; ++i) {
            const QPoint& pnt = _cNodeVector[static_cast<size_t>(i)];
            vp->vertex.set1Value(i, toOverlayPoint(pnt.x(), pnt.y(), viewportWidth, viewportHeight));
        }

        auto* lineSet = new SoLineSet;
        lineSet->vertexProperty.setValue(vp);
        lineSet->numVertices.setValue(count);
        root->addChild(lineSet);

        if (closed && stippled) {
            auto* sep = new SoSeparator;

            auto* stippleStyle = new SoDrawStyle;
            stippleStyle->lineWidth.setValue(line);
            stippleStyle->linePatternScaleFactor.setValue(2);
            stippleStyle->linePattern.setValue(0x3F3F);
            sep->addChild(stippleStyle);

            auto* closeVp = new SoVertexProperty;
            const QPoint& last = _cNodeVector.back();
            const QPoint& first = _cNodeVector.front();
            closeVp->vertex.set1Value(
                0,
                toOverlayPoint(last.x(), last.y(), viewportWidth, viewportHeight)
            );
            closeVp->vertex.set1Value(
                1,
                toOverlayPoint(first.x(), first.y(), viewportWidth, viewportHeight)
            );

            auto* closeLine = new SoLineSet;
            closeLine->vertexProperty.setValue(closeVp);
            closeLine->numVertices.setValue(2);
            sep->addChild(closeLine);

            root->addChild(sep);
        }
    }

    applyOverlay(root, viewportWidth, viewportHeight, nullptr, viewer);
    root->unref();
}
