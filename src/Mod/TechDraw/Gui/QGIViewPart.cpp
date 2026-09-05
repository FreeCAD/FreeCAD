/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
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

#include <QPainterPath>
#include <QKeyEvent>
#include <QGraphicsTransform>
#include <QImage>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <optional>
#include <qmath.h>
#include <utility>
#include <vector>

#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <Poly_Triangulation.hxx>
#include <Precision.hxx>
#include <Standard_Version.hxx>
#include <TopAbs_Orientation.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#if OCC_VERSION_HEX < 0x070600
#include <Poly_Array1OfTriangle.hxx>
#include <TColgp_Array1OfPnt.hxx>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Tools.h>
#include <Base/Vector3D.h>
#include <Gui/Selection/Selection.h>
#include <Mod/TechDraw/App/CenterLine.h>
#include <Mod/TechDraw/App/Cosmetic.h>
#include <Mod/TechDraw/App/DrawComplexSection.h>
#include <Mod/TechDraw/App/DrawGeomHatch.h>
#include <Mod/TechDraw/App/DrawHatch.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawViewDetail.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawViewSection.h>
#include <Mod/TechDraw/App/Geometry.h>
#include <Mod/TechDraw/App/DrawBrokenView.h>
#include <Mod/TechDraw/App/DrawProjGroup.h>
#include <Mod/TechDraw/App/DrawProjGroupItem.h>
#include <Mod/Part/App/Tools.h>

#include "DrawGuiUtil.h"
#include "MDIViewPage.h"
#include "PreferencesGui.h"
#include "QGIArrow.h"
#include "QGICMark.h"
#include "QGICenterLine.h"
#include "QGIEdge.h"
#include "QGIFace.h"
#include "QGIHighlight.h"
#include "QGIMatting.h"
#include "QGIPrimPath.h"
#include "QGISectionLine.h"
#include "QGIVertex.h"
#include "QGIViewPart.h"
#include "Rez.h"
#include "ViewProviderGeomHatch.h"
#include "ViewProviderHatch.h"
#include "ViewProviderViewPart.h"
#include "ViewProviderViewSection.h"
#include "ZVALUE.h"
#include "PathBuilder.h"
#include "QGIBreakLine.h"
#include "QGSPage.h"
#include "QGIProjGroup.h"

using namespace TechDraw;
using namespace TechDrawGui;
using namespace std;
using DU = DrawUtil;
using FillMode = QGIFace::FillMode;

const float lineScaleFactor = Rez::guiX(1.);// temp fiddle for devel

namespace {

constexpr double ShadedPixelsPerMillimetre = 12.0;  // approximately 300 dpi
constexpr int ShadedMaxImageDimension = 4096;
constexpr double ShadedAngularDeflection = 0.20;

struct ShadedVertex
{
    QPointF point;
    double depth;
    gp_Vec normal;
};

struct ShadedTriangle
{
    ShadedVertex vertices[3];
};

struct ShadedImage
{
    QImage image;
    QRectF rect;
};

class QGIShadedImage final : public QGIPrimPath
{
public:
    explicit QGIShadedImage(ShadedImage shaded)
        : m_image(std::move(shaded.image))
        , m_rect(shaded.rect)
    {
        QPainterPath outline;
        outline.addRect(m_rect);
        setPath(outline);
        setFlag(QGraphicsItem::ItemIsSelectable, false);
        setFlag(QGraphicsItem::ItemIsFocusable, false);
        setAcceptHoverEvents(false);
    }

    void paint(QPainter* painter,
               const QStyleOptionGraphicsItem*,
               QWidget*) override
    {
        painter->save();
        painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter->drawImage(m_rect, m_image);
        painter->restore();
    }

private:
    QImage m_image;
    QRectF m_rect;
};

double edgeFunction(const QPointF& a, const QPointF& b, double x, double y)
{
    return (x - a.x()) * (b.y() - a.y()) - (y - a.y()) * (b.x() - a.x());
}

QColor shadePixel(const QColor& base,
                  gp_Vec normal,
                  const gp_Vec& lightDirection,
                  const gp_Vec& viewDirection,
                  const gp_Vec& halfDirection)
{
    if (normal.SquareMagnitude() <= Precision::SquareConfusion()) {
        return base;
    }
    normal.Normalize();

    // TechDraw shades both sides of open shells. Make the normal face the camera
    // before applying a camera-relative light.
    if (normal.Dot(viewDirection) < 0.0) {
        normal.Reverse();
    }

    const double diffuse = std::max(0.0, normal.Dot(lightDirection));
    const double specular = std::pow(std::max(0.0, normal.Dot(halfDirection)), 24.0);
    const double intensity = std::clamp(0.28 + 0.62 * diffuse + 0.18 * specular, 0.0, 1.15);

    auto channel = [intensity](int value) {
        return std::clamp(static_cast<int>(std::lround(value * intensity)), 0, 255);
    };
    return QColor(channel(base.red()), channel(base.green()), channel(base.blue()), base.alpha());
}

std::optional<ShadedImage> makeShadedImage(DrawViewPart* viewPart, QColor baseColor)
{
    TopoDS_Shape sourceShape = viewPart->getSourceShape();
    if (sourceShape.IsNull()) {
        return {};
    }

    // Work on a topology copy so tessellation never modifies the source object's mesh.
    BRepBuilderAPI_Copy copier(sourceShape, false, false);
    TopoDS_Shape displayShape = copier.Shape();
    DrawViewPart::centerScaleRotate(
        viewPart, displayShape, viewPart->getCurrentCentroid());

    Bnd_Box shapeBox;
    BRepBndLib::Add(displayShape, shapeBox);
    if (shapeBox.IsVoid()) {
        return {};
    }

    double xMin = 0.0;
    double yMin = 0.0;
    double zMin = 0.0;
    double xMax = 0.0;
    double yMax = 0.0;
    double zMax = 0.0;
    shapeBox.Get(xMin, yMin, zMin, xMax, yMax, zMax);
    const double extent =
        std::max({xMax - xMin, yMax - yMin, zMax - zMin});
    const double deflection = std::max(Precision::Confusion(), extent / 500.0);
    BRepMesh_IncrementalMesh mesher(
        displayShape, deflection, false, ShadedAngularDeflection, true);
    if (!mesher.IsDone()) {
        return {};
    }

    const gp_Ax2 projection = viewPart->getProjectionCS();
    const gp_Pnt origin = projection.Location();
    const gp_Dir xDirection = projection.XDirection();
    const gp_Dir yDirection = projection.YDirection();
    const gp_Dir projectionDirection = projection.Direction();

    gp_Vec lightDirection(projectionDirection);
    lightDirection += gp_Vec(xDirection).Multiplied(-0.35);
    lightDirection += gp_Vec(yDirection).Multiplied(-0.45);
    lightDirection.Normalize();
    gp_Vec viewDirection(projectionDirection);
    gp_Vec halfDirection = lightDirection + viewDirection;
    halfDirection.Normalize();

    std::vector<ShadedTriangle> triangles;
    double sceneMinX = std::numeric_limits<double>::infinity();
    double sceneMinY = std::numeric_limits<double>::infinity();
    double sceneMaxX = -std::numeric_limits<double>::infinity();
    double sceneMaxY = -std::numeric_limits<double>::infinity();

    for (TopExp_Explorer explorer(displayShape, TopAbs_FACE);
         explorer.More();
         explorer.Next()) {
        const TopoDS_Face face = TopoDS::Face(explorer.Current());
        TopLoc_Location location;
        Handle(Poly_Triangulation) triangulation =
            BRep_Tool::Triangulation(face, location);
        if (triangulation.IsNull()) {
            continue;
        }

        std::vector<gp_Vec> normals;
        Part::Tools::getPointNormals(face, triangulation, normals);
        Part::Tools::applyTransformationOnNormals(location, normals);

#if OCC_VERSION_HEX < 0x070600
        const TColgp_Array1OfPnt& nodes = triangulation->Nodes();
        const Poly_Array1OfTriangle& meshTriangles = triangulation->Triangles();
#endif
        std::vector<ShadedVertex> vertices;
        vertices.reserve(triangulation->NbNodes());
        for (int nodeIndex = 1; nodeIndex <= triangulation->NbNodes(); ++nodeIndex) {
#if OCC_VERSION_HEX < 0x070600
            gp_Pnt point = nodes(nodeIndex);
#else
            gp_Pnt point = triangulation->Node(nodeIndex);
#endif
            if (!location.IsIdentity()) {
                point.Transform(location.Transformation());
            }

            const gp_Vec relative(origin, point);
            const QPointF projected(
                Rez::guiX(relative.Dot(gp_Vec(xDirection))),
                Rez::guiX(-relative.Dot(gp_Vec(yDirection))));
            vertices.push_back(
                {projected,
                 relative.Dot(gp_Vec(projectionDirection)),
                 normals.at(static_cast<size_t>(nodeIndex - 1))});

            sceneMinX = std::min(sceneMinX, projected.x());
            sceneMinY = std::min(sceneMinY, projected.y());
            sceneMaxX = std::max(sceneMaxX, projected.x());
            sceneMaxY = std::max(sceneMaxY, projected.y());
        }

        for (int triangleIndex = 1;
             triangleIndex <= triangulation->NbTriangles();
             ++triangleIndex) {
            int indices[3];
#if OCC_VERSION_HEX < 0x070600
            meshTriangles(triangleIndex).Get(
                indices[0], indices[1], indices[2]);
#else
            triangulation->Triangle(triangleIndex).Get(
                indices[0], indices[1], indices[2]);
#endif
            ShadedTriangle triangle;
            for (int vertexIndex = 0; vertexIndex < 3; ++vertexIndex) {
                triangle.vertices[vertexIndex] =
                    vertices.at(static_cast<size_t>(indices[vertexIndex] - 1));
            }
            triangles.push_back(std::move(triangle));
        }
    }

    const QRectF sceneBoundsUnpadded(
        QPointF(sceneMinX, sceneMinY),
        QPointF(sceneMaxX, sceneMaxY));
    if (triangles.empty() || sceneBoundsUnpadded.isEmpty()) {
        return {};
    }

    QRectF sceneBounds = sceneBoundsUnpadded;
    double pixelsPerSceneUnit = ShadedPixelsPerMillimetre / Rez::getRezFactor();
    const double longestSceneSide = std::max(sceneBounds.width(), sceneBounds.height());
    if (longestSceneSide * pixelsPerSceneUnit > ShadedMaxImageDimension) {
        pixelsPerSceneUnit = ShadedMaxImageDimension / longestSceneSide;
    }
    pixelsPerSceneUnit = std::max(pixelsPerSceneUnit, 0.01);

    const double margin = 1.0 / pixelsPerSceneUnit;
    sceneBounds.adjust(-margin, -margin, margin, margin);
    const int imageWidth =
        std::max(2, static_cast<int>(std::ceil(sceneBounds.width() * pixelsPerSceneUnit)));
    const int imageHeight =
        std::max(2, static_cast<int>(std::ceil(sceneBounds.height() * pixelsPerSceneUnit)));

    QImage image(imageWidth, imageHeight, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    std::vector<double> depthBuffer(
        static_cast<size_t>(imageWidth) * static_cast<size_t>(imageHeight),
        -std::numeric_limits<double>::infinity());

    auto toImagePoint = [&sceneBounds, pixelsPerSceneUnit](const QPointF& point) {
        return QPointF(
            (point.x() - sceneBounds.left()) * pixelsPerSceneUnit,
            (point.y() - sceneBounds.top()) * pixelsPerSceneUnit);
    };

    for (const auto& triangle : triangles) {
        QPointF points[3] = {
            toImagePoint(triangle.vertices[0].point),
            toImagePoint(triangle.vertices[1].point),
            toImagePoint(triangle.vertices[2].point)
        };
        const double area = edgeFunction(points[0], points[1], points[2].x(), points[2].y());
        if (std::abs(area) <= std::numeric_limits<double>::epsilon()) {
            continue;
        }

        const int left = std::max(
            0, static_cast<int>(std::floor(std::min({points[0].x(), points[1].x(), points[2].x()}))));
        const int right = std::min(
            imageWidth - 1,
            static_cast<int>(std::ceil(std::max({points[0].x(), points[1].x(), points[2].x()}))));
        const int top = std::max(
            0, static_cast<int>(std::floor(std::min({points[0].y(), points[1].y(), points[2].y()}))));
        const int bottom = std::min(
            imageHeight - 1,
            static_cast<int>(std::ceil(std::max({points[0].y(), points[1].y(), points[2].y()}))));

        for (int y = top; y <= bottom; ++y) {
            auto* scanline = reinterpret_cast<QRgb*>(image.scanLine(y));
            for (int x = left; x <= right; ++x) {
                const double sampleX = x + 0.5;
                const double sampleY = y + 0.5;
                const double w0 =
                    edgeFunction(points[1], points[2], sampleX, sampleY) / area;
                const double w1 =
                    edgeFunction(points[2], points[0], sampleX, sampleY) / area;
                const double w2 = 1.0 - w0 - w1;
                constexpr double edgeTolerance = -1.0e-8;
                if (w0 < edgeTolerance || w1 < edgeTolerance || w2 < edgeTolerance) {
                    continue;
                }

                const double depth =
                    w0 * triangle.vertices[0].depth
                    + w1 * triangle.vertices[1].depth
                    + w2 * triangle.vertices[2].depth;
                const size_t bufferIndex =
                    static_cast<size_t>(y) * static_cast<size_t>(imageWidth)
                    + static_cast<size_t>(x);
                if (depth <= depthBuffer[bufferIndex]) {
                    continue;
                }

                depthBuffer[bufferIndex] = depth;
                const gp_Vec normal(
                    w0 * triangle.vertices[0].normal.X()
                        + w1 * triangle.vertices[1].normal.X()
                        + w2 * triangle.vertices[2].normal.X(),
                    w0 * triangle.vertices[0].normal.Y()
                        + w1 * triangle.vertices[1].normal.Y()
                        + w2 * triangle.vertices[2].normal.Y(),
                    w0 * triangle.vertices[0].normal.Z()
                        + w1 * triangle.vertices[1].normal.Z()
                        + w2 * triangle.vertices[2].normal.Z());
                const QColor shaded =
                    shadePixel(baseColor, normal, lightDirection, viewDirection, halfDirection);
                scanline[x] = qRgba(
                    shaded.red(), shaded.green(), shaded.blue(), shaded.alpha());
            }
        }
    }

    return ShadedImage{std::move(image), sceneBounds};
}

}  // namespace

QGIViewPart::QGIViewPart()
{
    setCacheMode(QGraphicsItem::NoCache);
    setHandlesChildEvents(false);
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setFlag(QGraphicsItem::ItemIsFocusable, true);

    showSection = false;
    m_pathBuilder = new PathBuilder(this);
    m_dashedLineGenerator = new LineGenerator();
}

QGIViewPart::~QGIViewPart()
{
    tidy();
    delete m_pathBuilder;
    delete m_dashedLineGenerator;
}

QVariant QGIViewPart::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemSelectedHasChanged && scene()) {
        bool selectState = value.toBool();
        if (!selectState && !isUnderMouse()) {
            // hide everything
            bool hideCenters = hideCenterMarks();
            for (auto& child : childItems()) {
                if (child->type() == UserType::QGIVertex) {
                    child->hide();
                    continue;
                }

                if (child->type() == UserType::QGICMark &&
                    hideCenters) {
                    child->hide();
                }
            }
            return QGIView::itemChange(change, value);
        }
        // we are selected, don't change anything?
    }
    else if (change == ItemSceneChange && scene()) {
        // Disconnect the signal to prevent callbacks during teardown
        if (m_selectionChangedConnection) {
            QObject::disconnect(m_selectionChangedConnection);
            // Reset the connection handle so it's not holding a stale reference
            m_selectionChangedConnection = QMetaObject::Connection();
        }
        // This means we are finished?
        tidy();
    }
    else if (change == QGraphicsItem::ItemSceneHasChanged) {
        if (scene()) {
            // added to scene
            m_selectionChangedConnection = connect(scene(), &QGraphicsScene::selectionChanged, this, [this]() {
                if (!scene()) {
                    return;
                }
                // When selection changes, if the mouse is not over the view,
                // hide any non-selected vertices.
                if (!isUnderMouse()) {
                    bool hideCenters = hideCenterMarks();
                    for (auto* child : childItems()) {
                        if (child->type() == UserType::QGIVertex &&
                            !child->isSelected()) {
                            child->hide();
                        }
                        if (child->type() == UserType::QGICMark &&
                            hideCenters) {
                            child->hide();
                        }
                    }
                }
            });
        }
    }

    return QGIView::itemChange(change, value);
}

bool QGIViewPart::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
{
    if (event->type() == QEvent::ShortcutOverride) {
        // if we accept this event, we should get a regular keystroke event next
        // which will be processed by QGVPage/QGVNavStyle keypress logic, but not forwarded to
        // Std_Delete
        auto *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->matches(QKeySequence::Delete))  {
            bool success = removeSelectedCosmetic();
            if (success) {
                updateView(true);
                event->accept();
                return true;
            }
        }
    }

    return QGraphicsItem::sceneEventFilter(watched, event);
}

//! called when a DEL shortcut event is received.  If a cosmetic edge or vertex is
//! selected, remove it from the view.
bool QGIViewPart::removeSelectedCosmetic() const
{
    auto dvp(dynamic_cast<TechDraw::DrawViewPart*>(getViewObject()));
    if (!dvp) {
        throw Base::RuntimeError("Graphic has no feature!");
    }
    char* defaultDocument{nullptr};
    std::vector<Gui::SelectionObject> selectionAll = Gui::Selection().getSelectionEx(
        defaultDocument, TechDraw::DrawViewPart::getClassTypeId(), Gui::ResolveMode::OldStyleElement);
    if (selectionAll.empty()) {
        return false;
    }
    std::vector<std::string> subElements = selectionAll.front().getSubNames();
    if (subElements.empty()) {
        return false;
    }

    dvp->deleteCosmeticElements(subElements);
    dvp->refreshCEGeoms();
    dvp->refreshCLGeoms();
    dvp->refreshCVGeoms();

    return true;
}


//obs?
void QGIViewPart::tidy()
{
    //Delete any leftover items
    for (QList<QGraphicsItem*>::iterator it = deleteItems.begin(); it != deleteItems.end(); ++it) {
        delete *it;
    }
    deleteItems.clear();
}

void QGIViewPart::setViewPartFeature(TechDraw::DrawViewPart* obj)
{
    if (!obj)
        return;

    setViewFeature(static_cast<TechDraw::DrawView*>(obj));
}

QPainterPath QGIViewPart::drawPainterPath(TechDraw::BaseGeomPtr baseGeom) const
{
    double rot = getViewObject()->Rotation.getValue();
    return m_pathBuilder->geomToPainterPath(baseGeom, rot);
}

void QGIViewPart::updateView(bool update)
{
    auto viewPart(dynamic_cast<TechDraw::DrawViewPart*>(getViewObject()));
    if (!viewPart) {
        return;
    }

    auto vp = static_cast<ViewProviderViewPart*>(getViewProvider(getViewObject()));
    if (!vp) {
        return;
    }

    if (update) {
        draw();
    }

    QGIView::updateView(update);
}

void QGIViewPart::draw()
{
    auto viewPart(dynamic_cast<TechDraw::DrawViewPart*>(getViewObject()));
    if (!viewPart) {
        return;
    }

    auto doc = viewPart->getDocument();
    if (!doc || doc->testStatus(App::Document::Status::Restoring)) {
        // if the document is still restoring, we may not have all the information
        // we need to draw the source objects, so we wait until restore is finished.
        // Base::Console().message("QGIVP::draw - document is restoring, do not draw\n");
        return;
    }

    if (!isVisible())
        return;

    drawViewPart();
    drawAllHighlights();
    drawBreakLines();
    drawMatting();
    //this is old C/L
    drawCenterLines(true);//have to draw centerlines after border to get size correct.
    drawAllSectionLines();//same for section lines

    prepareGeometryChange();
}

void QGIViewPart::drawViewPart()
{
    auto viewPart(dynamic_cast<TechDraw::DrawViewPart*>(getViewObject()));
    if (!viewPart)
        return;
    //    Base::Console().message("QGIVP::DVP() - %s / %s\n", viewPart->getNameInDocument(), viewPart->Label.getValue());
    if (!viewPart->hasGeometry()) {
        removePrimitives();//clean the slate
        removeDecorations();
        return;
    }

    auto vp = static_cast<ViewProviderViewPart*>(getViewProvider(getViewObject()));
    if (!vp)
        return;

    prepareGeometryChange();
    removePrimitives();//clean the slate
    removeDecorations();

    if (viewPart->hasShadedDisplay()) {
        drawShaded();
    }
    else if (viewPart->handleFaces() && !viewPart->CoarseView.getValue()) {
        drawAllFaces();
    }

    drawAllEdges();

    drawAllVertexes();
}

void QGIViewPart::drawShaded()
{
    auto* viewPart = static_cast<TechDraw::DrawViewPart*>(getViewObject());
    auto* viewProvider =
        dynamic_cast<ViewProviderViewPart*>(getViewProvider(getViewObject()));
    if (!viewProvider) {
        return;
    }

    QColor faceColor = viewProvider->FaceColor.getValue().asValue<QColor>();
    faceColor.setAlpha(
        (100 - viewProvider->FaceTransparency.getValue()) * 255 / 100);

    auto shaded = makeShadedImage(viewPart, faceColor);
    if (!shaded) {
        Base::Console().warning(
            "Could not create shaded image for %s\n",
            viewPart->getNameInDocument());
        return;
    }

    auto* item = new QGIShadedImage(std::move(*shaded));
    addToGroupWithoutUpdate(item);
    item->setPos(0.0, 0.0);
    item->setZValue(ZVALUE::FACE);
}

void QGIViewPart::drawAllFaces(void)
{
    // dvp already validated
    auto dvp(static_cast<TechDraw::DrawViewPart*>(getViewObject()));

    QColor faceColor;
    auto vpp = dynamic_cast<ViewProviderViewPart *>(getViewProvider(getViewObject()));
    if (vpp) {
        faceColor = vpp->FaceColor.getValue().asValue<QColor>();
        faceColor.setAlpha((100 - vpp->FaceTransparency.getValue())*255/100);
    }

    std::vector<TechDraw::DrawHatch*> regularHatches = dvp->getHatches();
    std::vector<TechDraw::DrawGeomHatch*> geomHatches = dvp->getGeomHatches();
    const std::vector<TechDraw::FacePtr>& faceGeoms = dvp->getFaceGeometry();
    int iFace(0);
    for (auto& face : faceGeoms) {
        QGIFace* newFace = drawFace(face, iFace);
        if (faceColor.isValid()) {
            newFace->setFillColor(faceColor);
            newFace->setFillMode(faceColor.alpha() ? FillMode::PlainFill : FillMode::NoFill);
        }

        TechDraw::DrawHatch* fHatch = faceIsHatched(iFace, regularHatches);
        TechDraw::DrawGeomHatch* fGeom = faceIsGeomHatched(iFace, geomHatches);
        if (fGeom) {
            // geometric hatch (from PAT hatch specification)
            newFace->isHatched(true);
            newFace->setFillMode(FillMode::GeomHatchFill);
            std::vector<LineSet> lineSets = fGeom->getTrimmedLines(iFace);
            if (!lineSets.empty()) {
                // this face has geometric hatch lines
                for (auto& ls : lineSets) {
                    newFace->addLineSet(ls);
                }
            }
            double hatchScale = fGeom->ScalePattern.getValue();
            if (hatchScale > 0.0) {
                newFace->setHatchScale(fGeom->ScalePattern.getValue());
            }
            newFace->setHatchRotation(fGeom->PatternRotation.getValue());
            newFace->setHatchOffset(fGeom->PatternOffset.getValue());
            newFace->setHatchFile(fGeom->PatIncluded.getValue());
            Gui::ViewProvider* gvp = QGIView::getViewProvider(fGeom);
            ViewProviderGeomHatch* geomVp = freecad_cast<ViewProviderGeomHatch*>(gvp);
            if (geomVp) {
                newFace->setHatchColor(geomVp->ColorPattern.getValue());
                newFace->setLineWeight(geomVp->WeightPattern.getValue());
            }
        } else if (fHatch) {
            // svg or bitmap hatch
            newFace->isHatched(true);
            if (!fHatch->SvgIncluded.isEmpty()) {
                newFace->setHatchFile(fHatch->SvgIncluded.getValue());
            }
            if (fHatch->isSvgHatch()) {
                // svg tile hatch
                newFace->setFillMode(FillMode::SvgFill);
            } else {
                //bitmap hatch
                newFace->setFillMode(FillMode::BitmapFill);
            }

            // get the properties from the hatch viewprovider
            Gui::ViewProvider* gvp = QGIView::getViewProvider(fHatch);
            ViewProviderHatch* hatchVp = freecad_cast<ViewProviderHatch*>(gvp);
            if (hatchVp) {
                if (hatchVp->HatchScale.getValue() > 0.0) {
                    newFace->setHatchScale(hatchVp->HatchScale.getValue());
                }
                newFace->setHatchColor(hatchVp->HatchColor.getValue());
                newFace->setHatchRotation(hatchVp->HatchRotation.getValue());
                newFace->setHatchOffset(hatchVp->HatchOffset.getValue());
            }
        }

        newFace->setDrawEdges(prefFaceEdges());
        newFace->setZValue(ZVALUE::FACE);
        newFace->setPrettyNormal();
        newFace->draw();
        iFace++;
    }
}

void QGIViewPart::drawAllEdges()
{
    // dvp and vp already validated
    auto dvp(static_cast<TechDraw::DrawViewPart*>(getViewObject()));
    auto vp = static_cast<ViewProviderViewPart*>(getViewProvider(getViewObject()));

    const TechDraw::BaseGeomPtrVector& geoms = dvp->getEdgeGeometry();
    TechDraw::BaseGeomPtrVector::const_iterator itGeom = geoms.begin();
    QGIEdge* item{};
    for (int iEdge = 0; itGeom != geoms.end(); itGeom++, iEdge++) {
        bool showItem = true;
        if (!showThisEdge(*itGeom)) {
            continue;
        }

        item = new QGIEdge(iEdge);
        addToGroupWithoutUpdate(item);      //item is created at scene(0, 0), not group(0, 0)
        item->setPath(drawPainterPath(*itGeom));
        item->setSource((*itGeom)->source());

        item->setNormalColor(PreferencesGui::getAccessibleQColor(PreferencesGui::normalQColor()));
        if ((*itGeom)->getCosmetic()) {
            // cosmetic edge - format appropriately
            TechDraw::SourceType source = (*itGeom)->source();
            if (source == TechDraw::SourceType::COSMETICEDGE) {
                std::string cTag = (*itGeom)->getCosmeticTag();
                showItem = formatGeomFromCosmetic(cTag, item);
            }
            else if (source == TechDraw::SourceType::CENTERLINE) {
                std::string cTag = (*itGeom)->getCosmeticTag();
                showItem = formatGeomFromCenterLine(cTag, item);
            }
            else {
                // there are 3 source types (GEOMETRY, COSMETICEDGE, CENTERLINE). Something broke if we
                // get here for for an edge that claims to be cosmetic.
                Base::Console().warning("In %s, cosmetic edge: %d is neither COSMETICEDGE nor CENTERLINE - actual source type: %d\n",
                                        dvp->Label.getValue(), iEdge, static_cast<int>(source));
            }
        } else {
            // geometry edge - apply format if applicable
            TechDraw::GeomFormat* gf = dvp->getGeomFormatBySelection(iEdge);
            if (gf) {
                Base::Color  color = Preferences::getAccessibleColor(gf->m_format.getColor());
                item->setNormalColor(color.asValue<QColor>());
                int lineNumber = gf->m_format.getLineNumber();
                int qtStyle = gf->m_format.getStyle();
                item->setLinePen(m_dashedLineGenerator->getBestPen(lineNumber, (Qt::PenStyle)qtStyle,
                                                     gf->m_format.getWidth()));
                // but we need to actually draw the lines in QGScene coords (0.1 mm).
                item->setWidth(Rez::guiX(gf->m_format.getWidth()));
                showItem = gf->m_format.getVisible();
            } else {
                if (!(*itGeom)->getHlrVisible()) {
                    // hidden line without a format
                    if (dvp->hiddenEdgesAreSolid()) {
                        item->setLinePen(
                            m_dashedLineGenerator->getLinePen(
                                1, vp->LineWidth.getValue()));
                    }
                    else {
                        item->setLinePen(
                            m_dashedLineGenerator->getLinePen(
                                Preferences::HiddenLineStyle(),
                                vp->HiddenWidth.getValue()));
                    }
                    item->setHiddenEdge(true);
                    const double hiddenWidth = dvp->hiddenEdgesAreSolid()
                        ? vp->LineWidth.getValue()
                        : vp->HiddenWidth.getValue();
                    item->setWidth(Rez::guiX(hiddenWidth));
                    item->setZValue(ZVALUE::HIDEDGE);
                } else {
                    // unformatted visible line, draw as continuous line
                    // "smooth" edges should use the "thin" width as used for hidden lines.
                    double width = (*itGeom)->getClassOfEdge() == EdgeClass::SMOOTH ?
                                            vp->HiddenWidth.getValue() : vp->LineWidth.getValue();
                    item->setLinePen(m_dashedLineGenerator->getLinePen(1, width));
                    item->setWidth(Rez::guiX(width));
                }
            }
        }

        if ((*itGeom)->getClassOfEdge() == EdgeClass::UVISO) {
            // we don't have a style option for iso-parametric lines so draw continuous
            item->setLinePen(m_dashedLineGenerator->getLinePen(1, vp->IsoWidth.getValue()));
            item->setWidth(Rez::guiX(vp->IsoWidth.getValue()));   //graphic
        }

        item->setPos(0.0, 0.0);//now at group(0, 0)
        item->setZValue(ZVALUE::EDGE);
        if (!dvp->showsVisibleEdges()
            && (*itGeom)->source() == TechDraw::SourceType::GEOMETRY) {
            // Retain real edge items for hover, selection, and dimensions while
            // suppressing their normal paint in the edge-free shaded style.
            item->setHiddenEdge(false);
            item->setNormalColor(Qt::transparent);
        }
        item->setPrettyNormal();

        if (!vp->ShowAllEdges.getValue() && !showItem) {
             //view level "show" status  && individual edge "show" status
             item->hide();
        }

        //debug a path
        //            QPainterPath edgePath=drawPainterPath(*itGeom);
        //            std::stringstream edgeId;
        //            edgeId << "QGIVP.edgePath" << i;
        //            dumpPath(edgeId.str().c_str(), edgePath);
    }
}

void QGIViewPart::drawAllVertexes()
{
    // dvp and vp already validated
    auto dvp(static_cast<TechDraw::DrawViewPart*>(getViewObject()));
    auto vp(static_cast<ViewProviderViewPart*>(getViewProvider(getViewObject())));
    ViewProviderPage* vpPage = vp->getViewProviderPage();
    QColor vertexColor = PreferencesGui::getAccessibleQColor(PreferencesGui::vertexQColor());

    const std::vector<TechDraw::VertexPtr>& verts = dvp->getVertexGeometry();
    auto vert = verts.begin();
    for (int i = 0; vert != verts.end(); ++vert, i++) {
        if ((*vert)->isCenter()) {
            auto* cmItem = new QGICMark(i);
            addToGroupWithoutUpdate(cmItem);
            cmItem->setPos(Rez::guiX((*vert)->x()), Rez::guiX((*vert)->y()));
            cmItem->setThick(0.5F * getLineWidth());    //need minimum?
            cmItem->setSize(getVertexSize() * vp->CenterScale.getValue());
            cmItem->setPrettyNormal();
            cmItem->setZValue(ZVALUE::VERTEX);
            bool showMark =
                ( (!isExporting() && vp->ArcCenterMarks.getValue()) ||
                  (isExporting() && Preferences::printCenterMarks()) ||
                  (vpPage->getFrameState() && PreferencesGui::getViewFrameMode() == ViewFrameMode::Manual));
            cmItem->setVisible(showMark);
        } else {
            //regular Vertex
            if (showVertices()) {
                auto* item = new QGIVertex(i);
                addToGroupWithoutUpdate(item);
                item->setPos(Rez::guiX((*vert)->x()), Rez::guiX((*vert)->y()));
                item->setNormalColor(vertexColor);
                item->setFillColor(vertexColor);
                item->setRadius(getVertexSize());
                item->setPrettyNormal();
                item->setZValue(ZVALUE::VERTEX);
                item->setVisible(shouldShowFrame());
            }
        }
    }
}

bool QGIViewPart::showThisEdge(BaseGeomPtr geom)
{
    // dvp and vp already validated
    auto dvp(static_cast<TechDraw::DrawViewPart*>(getViewObject()));

    if (geom->getHlrVisible()) {
        if ((geom->getClassOfEdge() == EdgeClass::HARD) || (geom->getClassOfEdge() == EdgeClass::OUTLINE)
            || ((geom->getClassOfEdge() == EdgeClass::SMOOTH) && dvp->SmoothVisible.getValue())
            || ((geom->getClassOfEdge() == EdgeClass::SEAM) && dvp->SeamVisible.getValue())
            || ((geom->getClassOfEdge() == EdgeClass::UVISO) && dvp->IsoVisible.getValue())) {
            return true;
        }
    } else {
        if (((geom->getClassOfEdge() == EdgeClass::HARD) && (dvp->HardHidden.getValue()))
            || ((geom->getClassOfEdge() == EdgeClass::OUTLINE) && (dvp->HardHidden.getValue()))
            || ((geom->getClassOfEdge() == EdgeClass::SMOOTH) && (dvp->SmoothHidden.getValue()))
            || ((geom->getClassOfEdge() == EdgeClass::SEAM) && (dvp->SeamHidden.getValue()))
            || ((geom->getClassOfEdge() == EdgeClass::UVISO) && (dvp->IsoHidden.getValue()))) {
            return true;
        }
    }

    return false;
}


bool QGIViewPart::formatGeomFromCosmetic(std::string cTag, QGIEdge* item)
{
    //    Base::Console().message("QGIVP::formatGeomFromCosmetic(%s)\n", cTag.c_str());
    bool result = true;
    auto partFeat(dynamic_cast<TechDraw::DrawViewPart*>(getViewObject()));
    TechDraw::CosmeticEdge* ce = partFeat ? partFeat->getCosmeticEdge(cTag) : nullptr;
    if (ce) {
        Base::Color color = Preferences::getAccessibleColor(ce->m_format.getColor());
        item->setNormalColor(color.asValue<QColor>());
        item->setLinePen(m_dashedLineGenerator->getBestPen(ce->m_format.getLineNumber(),
                                                     (Qt::PenStyle)ce->m_format.getStyle(),
                                                     ce->m_format.getWidth()));
        item->setWidth(Rez::guiX(ce->m_format.getWidth()));
        result = ce->m_format.getVisible();
    }
    return result;
}


bool QGIViewPart::formatGeomFromCenterLine(std::string cTag, QGIEdge* item)
{
//    Base::Console().message("QGIVP::formatGeomFromCenterLine()\n");
    bool result = true;
    auto partFeat(dynamic_cast<TechDraw::DrawViewPart*>(getViewObject()));
    TechDraw::CenterLine* cl = partFeat ? partFeat->getCenterLine(cTag) : nullptr;
    if (cl) {
        Base::Color color = Preferences::getAccessibleColor(cl->m_format.getColor());
        item->setNormalColor(color.asValue<QColor>());
        item->setLinePen(m_dashedLineGenerator->getBestPen(cl->m_format.getLineNumber(),
                                                     (Qt::PenStyle)cl->m_format.getStyle(),
                                                     cl->m_format.getWidth()));
        item->setWidth(Rez::guiX(cl->m_format.getWidth()));
        result = cl->m_format.getVisible();
    }
    return result;
}

QGIFace* QGIViewPart::drawFace(TechDraw::FacePtr f, int idx)
{
    //    Base::Console().message("QGIVP::drawFace - %d\n", idx);
    std::vector<TechDraw::Wire*> fWires = f->wires;
    QPainterPath facePath;
    for (std::vector<TechDraw::Wire*>::iterator wire = fWires.begin(); wire != fWires.end();
         ++wire) {
        TechDraw::BaseGeomPtrVector geoms = (*wire)->geoms;
        if (geoms.empty())
            continue;

        TechDraw::BaseGeomPtr firstGeom = geoms.front();
        QPainterPath wirePath;
        //QPointF startPoint(firstGeom->getStartPoint().x, firstGeom->getStartPoint().y);
        //wirePath.moveTo(startPoint);
        QPainterPath firstSeg = drawPainterPath(firstGeom);
        wirePath.connectPath(firstSeg);
        for (TechDraw::BaseGeomPtrVector::iterator edge = ((*wire)->geoms.begin()) + 1;
             edge != (*wire)->geoms.end(); ++edge) {
            QPainterPath edgePath = drawPainterPath(*edge);
            //handle section faces differently
            if (idx == -1) {
                QPointF wEnd = wirePath.currentPosition();
                auto element = edgePath.elementAt(0);
                QPointF eStart(element.x, element.y);
                QPointF eEnd = edgePath.currentPosition();
                QPointF sVec = wEnd - eStart;
                QPointF eVec = wEnd - eEnd;
                double sDist2 = sVec.x() * sVec.x() + sVec.y() * sVec.y();
                double eDist2 = eVec.x() * eVec.x() + eVec.y() * eVec.y();
                if (sDist2 > eDist2) {
                    edgePath = edgePath.toReversed();
                }
            }
            wirePath.connectPath(edgePath);
        }
        //        dumpPath("wirePath:", wirePath);
        facePath.addPath(wirePath);
    }
    facePath.setFillRule(Qt::OddEvenFill);

    QGIFace* gFace = new QGIFace(idx);
    addToGroupWithoutUpdate(gFace);
    gFace->setPos(0.0, 0.0);
    gFace->setOutline(facePath);
    //debug a path
    //std::stringstream faceId;
    //faceId << "facePath " << idx;
    //dumpPath(faceId.str().c_str(), facePath);

    return gFace;
}

//! Remove all existing QGIPrimPath items(Vertex, Edge, Face)
//note this triggers scene selectionChanged signal if vertex/edge/face is selected
void QGIViewPart::removePrimitives()
{
    QList<QGraphicsItem*> children = childItems();
    MDIViewPage* mdi = getMDIViewPage();
    if (mdi) {
        getMDIViewPage()->blockSceneSelection(true);
    }
    for (auto& c : children) {
        QGIPrimPath* prim = dynamic_cast<QGIPrimPath*>(c);
        if (prim) {
            prim->hide();
            scene()->removeItem(prim);
            delete prim;
        }
    }
    if (mdi) {
        getMDIViewPage()->blockSceneSelection(false);
    }
}

//! Remove all existing QGIDecoration items(SectionLine, SectionMark, ...)
void QGIViewPart::removeDecorations()
{
    QList<QGraphicsItem*> children = childItems();
    for (auto& c : children) {
        QGIDecoration* decor = dynamic_cast<QGIDecoration*>(c);
        QGIMatting* mat = dynamic_cast<QGIMatting*>(c);
        if (decor) {
            decor->hide();
            scene()->removeItem(decor);
            delete decor;
        }
        else if (mat) {
            mat->hide();
            scene()->removeItem(mat);
            delete mat;
        }
    }
}

void QGIViewPart::drawAllSectionLines()
{
    TechDraw::DrawViewPart* viewPart = static_cast<TechDraw::DrawViewPart*>(getViewObject());
    if (!viewPart)
        return;

    auto vp = static_cast<ViewProviderViewPart*>(getViewProvider(getViewObject()));
    if (!vp) {
        return;
    }

    if (vp->ShowSectionLine.getValue()) {
        auto refs = viewPart->getSectionRefs();
        for (auto& r : refs) {
            if (r->isDerivedFrom<DrawComplexSection>()) {
                drawComplexSectionLine(r, true);
            }
            else {
                drawSectionLine(r, true);
            }
        }
    }
}

void QGIViewPart::drawSectionLine(TechDraw::DrawViewSection* viewSection, bool b)
{
//    Base::Console().message("QGIVP::drawSectionLine()\n");
    TechDraw::DrawViewPart* viewPart = static_cast<TechDraw::DrawViewPart*>(getViewObject());
    if (!viewPart)
        return;
    if (!viewSection)
        return;

    if (!viewSection->hasGeometry())
        return;

    auto vp = static_cast<ViewProviderViewPart*>(getViewProvider(viewPart));
    if (!vp) {
        return;
    }

    auto sectionVp = static_cast<ViewProviderViewSection*>(getViewProvider(viewSection));
    if (!sectionVp) {
        return;
    }

    if (b) {
        //find the ends of the section line
        double scale = viewPart->getScale();
        std::pair<Base::Vector3d, Base::Vector3d> sLineEnds = viewSection->sectionLineEnds();
        Base::Vector3d l1 = Rez::guiX(sLineEnds.first) * scale;
        Base::Vector3d l2 = Rez::guiX(sLineEnds.second) * scale;
        if (l1.IsEqual(l2, EWTOLERANCE) ) {
            Base::Console().message("QGIVP::drawSectionLine - line endpoints are equal. No section line created.\n");
            return;
        }

        QGISectionLine* sectionLine = new QGISectionLine();
        addToGroupWithoutUpdate(sectionLine);
        sectionLine->setSymbol(const_cast<char*>(viewSection->SectionSymbol.getValue()));
        sectionLine->setPathMode(false);

        //make the section line a little longer
        double fudge = 2.0 * Preferences::dimFontSizeMM();
        Base::Vector3d lineDir = l2 - l1;
        lineDir.Normalize();
        sectionLine->setEnds(l1 - lineDir * Rez::guiX(fudge), l2 + lineDir * Rez::guiX(fudge));

        //which way do the arrows point?
        Base::Vector3d arrowDir = viewSection->SectionNormal.getValue();
        arrowDir = -viewPart->projectPoint(arrowDir);      //arrows point reverse of sectionNormal
        sectionLine->setDirection(arrowDir.x, -arrowDir.y);//3d direction needs Y inversion

        if (vp->SectionLineMarks.getValue()) {
            ChangePointVector points = viewSection->getChangePointsFromSectionLine();
            //extend the changePoint locations to match the fudged section line ends
            QPointF location0 = points.front().getLocation() * scale;
            location0 = location0 - DU::toQPointF(lineDir) * fudge;
            QPointF location1 = points.back().getLocation() * scale;
            location1 = location1 + DU::toQPointF(lineDir) * fudge;
            //change points have Rez::guiX applied in sectionLine
            points.front().setLocation(location0);
            points.back().setLocation(location1);
            sectionLine->setChangePoints(points);
        }
        else {
            sectionLine->clearChangePoints();
        }

        //set the general parameters
        sectionLine->setPos(0.0, 0.0);

        if (vp->IncludeCutLine.getValue()) {
            sectionLine->setShowLine(true);
            // sectionLines are typically ISO 8 (long dash, short dash) or ISO 4 (long dash, dot)
            sectionLine->setLinePen(
                    m_dashedLineGenerator->getLinePen((size_t)vp->SectionLineStyle.getValue(),
                                                        vp->HiddenWidth.getValue()));
            sectionLine->setWidth(Rez::guiX(vp->HiddenWidth.getValue()));
        } else {
            sectionLine->setShowLine(false);
        }

        Base::Color color = Preferences::getAccessibleColor(vp->SectionLineColor.getValue());
        sectionLine->setSectionColor(color.asValue<QColor>());

        auto font = sectionVp->SectionLineFont.getValue();
        auto fontSize = sectionVp->SectionLineFontsize.getValue();
        auto arrowSize = sectionVp->SectionLineArrowsize.getValue();

        QFont symFont;
        symFont.setFamily(QString::fromUtf8(font));
        symFont.setPixelSize(exactFontSize(font, std::max(1.0, fontSize)));

        sectionLine->setFont(symFont);
        sectionLine->setArrowSize(arrowSize);
        sectionLine->setZValue(ZVALUE::SECTIONLINE);
        sectionLine->setRotation(-viewPart->Rotation.getValue());
        sectionLine->draw();
    }
}

void QGIViewPart::drawComplexSectionLine(TechDraw::DrawViewSection* viewSection, bool b)
{
    Q_UNUSED(b);

    TechDraw::DrawViewPart* viewPart = static_cast<TechDraw::DrawViewPart*>(getViewObject());
    if (!viewPart)
        return;
    if (!viewSection)
        return;
    auto vp = static_cast<ViewProviderViewPart*>(getViewProvider(viewPart));
    if (!vp) {
        return;
    }
    auto sectionVp = static_cast<ViewProviderViewSection*>(getViewProvider(viewSection));
    if (!sectionVp) {
        return;
    }

    auto dcs = static_cast<DrawComplexSection*>(viewSection);
    std::pair<Base::Vector3d, Base::Vector3d> ends = dcs->sectionLineEnds();
    Base::Vector3d vStart = Rez::guiX(ends.first);//already scaled by dcs
    Base::Vector3d vEnd = Rez::guiX(ends.second);
    if (vStart.IsEqual(vEnd, EWTOLERANCE) ) {
        Base::Console().message("QGIVP::drawComplexSectionLine - line endpoints are equal. No section line created.\n");
        return;
    }


    BaseGeomPtrVector edges = dcs->makeSectionLineGeometry();
    QPainterPath wirePath;
    QPainterPath firstSeg = drawPainterPath(edges.front());
    wirePath.connectPath(firstSeg);
    int edgeCount = edges.size();
    //NOTE: if the edges are not in nose to tail order, Qt will insert extra segments
    //that will overlap the segments we add. for interrupted line styles, this
    //will make the line look continuous.  This is prevented in
    //DrawComplexSection::makeSectionLineGeometry by calling makeNoseToTailWire
    for (int i = 1; i < edgeCount; i++) {
        QPainterPath edgePath = drawPainterPath(edges.at(i));
        wirePath.connectPath(edgePath);
    }


    QGISectionLine* sectionLine = new QGISectionLine();
    addToGroupWithoutUpdate(sectionLine);
    sectionLine->setSymbol(const_cast<char*>(viewSection->SectionSymbol.getValue()));

    sectionLine->setPathMode(true);
    sectionLine->setPath(wirePath);
    sectionLine->setEnds(vStart, vEnd);
    if (vp->SectionLineMarks.getValue()) {
        sectionLine->setChangePoints(dcs->getChangePointsFromSectionLine());
    }
    else {
        sectionLine->clearChangePoints();
    }

    std::pair<Base::Vector3d, Base::Vector3d> dirsDCS = dcs->sectionLineArrowDirsMapped();
    sectionLine->setArrowDirections(dirsDCS.first, dirsDCS.second);

    //set the general parameters
    sectionLine->setPos(0.0, 0.0);

    if (vp->IncludeCutLine.getValue()) {
        sectionLine->setShowLine(true);
        // sectionLines are typically ISO 8 (long dash, short dash) or ISO 4 (long dash, dot)
        sectionLine->setLinePen(
                m_dashedLineGenerator->getLinePen((size_t)vp->SectionLineStyle.getValue(),
                                                    vp->HiddenWidth.getValue()));
        sectionLine->setWidth(Rez::guiX(vp->HiddenWidth.getValue()));
    } else {
        sectionLine->setShowLine(false);
    }

    Base::Color color = Preferences::getAccessibleColor(vp->SectionLineColor.getValue());
    sectionLine->setSectionColor(color.asValue<QColor>());

    auto font = sectionVp->SectionLineFont.getValue();
    auto fontSize = sectionVp->SectionLineFontsize.getValue();
    auto arrowSize = sectionVp->SectionLineArrowsize.getValue();

    QFont symFont;
    symFont.setFamily(QString::fromUtf8(font));
    symFont.setPixelSize(exactFontSize(font, std::max(1.0, fontSize)));

    sectionLine->setFont(symFont);
    sectionLine->setArrowSize(arrowSize);
    sectionLine->setZValue(ZVALUE::SECTIONLINE);
    sectionLine->setRotation(-viewPart->Rotation.getValue());
    sectionLine->draw();
}

//TODO: use Cosmetic::CenterLine object for this to make it usable for dims.
// these are the view center lines (ie x,y axes)
void QGIViewPart::drawCenterLines(bool b)
{
    TechDraw::DrawViewPart* viewPart = dynamic_cast<TechDraw::DrawViewPart*>(getViewObject());
    if (!viewPart)
        return;

    auto vp = static_cast<ViewProviderViewPart*>(getViewProvider(getViewObject()));
    if (!vp)
        return;

    if (b) {
        bool horiz = vp->HorizCenterLine.getValue();
        bool vert = vp->VertCenterLine.getValue();
        const QColor centerColor = PreferencesGui::getAccessibleQColor(PreferencesGui::centerQColor());

        QGICenterLine* centerLine;
        double sectionSpan;
        double sectionFudge = Rez::guiX(10.0);
        double xVal, yVal;
        if (horiz) {
            centerLine = new QGICenterLine();
            addToGroupWithoutUpdate(centerLine);
            centerLine->setPos(0.0, 0.0);
            double width = Rez::guiX(viewPart->getBoxX());
            sectionSpan = width + sectionFudge;
            xVal = sectionSpan / 2.0;
            yVal = 0.0;
            centerLine->setIntersection(horiz && vert);
            centerLine->setBounds(-xVal, -yVal, xVal, yVal);
            centerLine->setLinePen(m_dashedLineGenerator->getLinePen((size_t)Preferences::CenterLineStyle(),
                                  vp->HiddenWidth.getValue()));
            centerLine->setWidth(Rez::guiX(vp->HiddenWidth.getValue()));
            centerLine->setColor(centerColor);
            centerLine->setZValue(ZVALUE::SECTIONLINE);
            centerLine->draw();
        }
        if (vert) {
            centerLine = new QGICenterLine();
            addToGroupWithoutUpdate(centerLine);
            centerLine->setPos(0.0, 0.0);
            double height = Rez::guiX(viewPart->getBoxY());
            sectionSpan = height + sectionFudge;
            xVal = 0.0;
            yVal = sectionSpan / 2.0;
            centerLine->setIntersection(horiz && vert);
            centerLine->setBounds(-xVal, -yVal, xVal, yVal);
            centerLine->setLinePen(m_dashedLineGenerator->getLinePen((size_t)Preferences::CenterLineStyle(),
                                  vp->HiddenWidth.getValue()));
            centerLine->setWidth(Rez::guiX(vp->HiddenWidth.getValue()));
            centerLine->setColor(centerColor);
            centerLine->setZValue(ZVALUE::SECTIONLINE);
            centerLine->draw();
        }
    }
}

void QGIViewPart::drawAllHighlights()
{
    // dvp and vp already validated
    auto dvp(static_cast<TechDraw::DrawViewPart*>(getViewObject()));

    auto drefs = dvp->getDetailRefs();
    for (auto& r : drefs) {
        drawHighlight(r, true);
    }
}

void QGIViewPart::drawHighlight(TechDraw::DrawViewDetail* viewDetail, bool b)
{
    auto* viewPart = static_cast<TechDraw::DrawViewPart*>(getViewObject());
    if (!viewPart || !viewDetail) {
        return;
    }

    auto vp = static_cast<ViewProviderViewPart*>(getViewProvider(getViewObject()));
    if (!vp) {
        return;
    }
    auto vpDetail = static_cast<ViewProviderViewPart*>(getViewProvider(viewDetail));
    if (!vpDetail) {
        return;
    }

    if (!viewDetail->ShowHighlight.getValue()) {
        return;
    }

    if (b) {
        double fontSize = Preferences::labelFontSizeMM();
        auto* highlight = new QGIHighlight();

        scene()->addItem(highlight);
        highlight->setReference(viewDetail->Reference.getValue());

        highlight->setFeatureName(viewDetail->getNameInDocument());

        highlight->setInteractive(false);

        addToGroupWithoutUpdate(highlight);
        highlight->setPos(0.0, 0.0);//sb setPos(center.x, center.y)?

        Base::Vector3d center = viewDetail->AnchorPoint.getValue() * viewPart->getScale();
        double rotationRad = Base::toRadians(viewPart->Rotation.getValue());
        center.RotateZ(rotationRad);

        double radius = viewDetail->Radius.getValue() * viewPart->getScale();
        highlight->setBounds(center.x - radius, center.y + radius, center.x + radius,
                             center.y - radius);
        highlight->setLinePen(m_dashedLineGenerator->getLinePen((size_t)vp->HighlightLineStyle.getValue(),
                             vp->IsoWidth.getValue()));
        highlight->setWidth(Rez::guiX(vp->IsoWidth.getValue()));
        highlight->setFont(getFont(), fontSize);
        Base::Color color = Preferences::getAccessibleColor(vp->HighlightLineColor.getValue());
        highlight->setColor(color.asValue<QColor>());
        highlight->setZValue(ZVALUE::HIGHLIGHT);
        highlight->setReferenceAngle(vp->HighlightAdjust.getValue());

        //handle conversion of apparent X,Y to rotated
        QPointF rotCenter = highlight->mapFromParent(transformOriginPoint());
        highlight->setTransformOriginPoint(rotCenter);

        double rotation = viewPart->Rotation.getValue();
        highlight->setRotation(rotation);
        highlight->draw();
    }
}

//! this method is no longer used due to conflicts with TaskDetail dialog highlight drag
void QGIViewPart::highlightMoved(QGIHighlight* highlight, QPointF newPos)
{
    std::string highlightName = highlight->getFeatureName();
    App::Document* doc = getViewObject()->getDocument();
    App::DocumentObject* docObj = doc->getObject(highlightName.c_str());
    auto detail = freecad_cast<DrawViewDetail*>(docObj);
    auto baseView = freecad_cast<DrawViewPart*>(getViewObject());
    if (detail && baseView) {
        auto oldAnchor = detail->AnchorPoint.getValue();
        Base::Vector3d delta = Rez::appX(DrawUtil::toVector3d(newPos)) / getViewObject()->getScale();
        delta = DrawUtil::invertY(delta);
        Base::Vector3d newAnchorPoint = oldAnchor + delta;
                newAnchorPoint = baseView->snapHighlightToVertex(newAnchorPoint,
                                                                 detail->Radius.getValue());
        detail->AnchorPoint.setValue(newAnchorPoint);
    }
}


void QGIViewPart::drawMatting()
{
    auto viewPart(dynamic_cast<TechDraw::DrawViewPart*>(getViewObject()));
    TechDraw::DrawViewDetail* dvd = nullptr;
    if (viewPart && viewPart->isDerivedFrom<TechDraw::DrawViewDetail>()) {
        dvd = static_cast<TechDraw::DrawViewDetail*>(viewPart);
    }
    else {
        return;
    }

    if (!dvd->ShowMatting.getValue()) {
        return;
    }

    double scale = dvd->getScale();
    double radius = dvd->Radius.getValue() * scale;
    QGIMatting* mat = new QGIMatting();
    addToGroupWithoutUpdate(mat);
    mat->setRadius(Rez::guiX(radius));
    mat->setPos(0.0, 0.0);
    mat->draw();
    mat->show();
}


//! if this is a broken view, draw the break lines.
void QGIViewPart::drawBreakLines()
{
    // Base::Console().message("QGIVP::drawBreakLines()\n");

    auto dbv = dynamic_cast<TechDraw::DrawBrokenView*>(getViewObject());
    if (!dbv) {
        return;
    }

    auto vp = static_cast<ViewProviderViewPart*>(getViewProvider(getViewObject()));
    if (!vp) {
        return;
    }

    DrawBrokenView::BreakType breakType = static_cast<DrawBrokenView::BreakType>(vp->BreakLineType.getValue());
    auto breaks = dbv->Breaks.getValues();
    for (auto& breakObj : breaks) {
        QGIBreakLine* breakLine = new QGIBreakLine();
        addToGroupWithoutUpdate(breakLine);

        Base::Vector3d direction = dbv->guiDirectionFromObj(*breakObj);
        breakLine->setDirection(direction);
        // the bounds describe two corners of the removed area in the view
        std::pair<Base::Vector3d, Base::Vector3d> bounds = dbv->breakBoundsFromObj(*breakObj);
        // the bounds are in 3d form, so we need to invert & rez them
        Base::Vector3d topLeft     = Rez::guiX(DU::invertY(bounds.first));
        Base::Vector3d bottomRight = Rez::guiX(DU::invertY(bounds.second));
        breakLine->setBounds(topLeft, bottomRight);
        breakLine->setPos(0.0, 0.0);
        breakLine->setLinePen(
            m_dashedLineGenerator->getLinePen(vp->BreakLineStyle.getValue(), vp->HiddenWidth.getValue()));
        breakLine->setWidth(Rez::guiX(vp->HiddenWidth.getValue()));
        breakLine->setBreakType(breakType);
        breakLine->setZValue(ZVALUE::SECTIONLINE);
        Base::Color color = vp->BreakLineColor.getValue();
        breakLine->setBreakColor(color.asValue<QColor>());
        breakLine->setRotation(-dbv->Rotation.getValue());
        breakLine->draw();
    }
}


void QGIViewPart::toggleCache(bool state)
{
    QList<QGraphicsItem*> items = childItems();
    for (QList<QGraphicsItem*>::iterator it = items.begin(); it != items.end(); it++) {
        //(*it)->setCacheMode((state)? DeviceCoordinateCache : NoCache);        //TODO: fiddle cache settings if req'd for performance
        Q_UNUSED(state);
        (*it)->setCacheMode(NoCache);
        (*it)->update();
    }
}

void QGIViewPart::toggleCosmeticLines(bool state)
{
    QList<QGraphicsItem*> items = childItems();
    for (QList<QGraphicsItem*>::iterator it = items.begin(); it != items.end(); it++) {
        QGIEdge* edge = dynamic_cast<QGIEdge*>(*it);
        if (edge) {
            edge->setCosmetic(state);
        }
    }
}

//get hatchObj for face i if it exists
TechDraw::DrawHatch* QGIViewPart::faceIsHatched(int i,
                                                std::vector<TechDraw::DrawHatch*> hatchObjs) const
{
    TechDraw::DrawHatch* result = nullptr;
    bool found = false;
    for (auto& h : hatchObjs) {
        const std::vector<std::string>& sourceNames = h->Source.getSubValues();
        for (auto& s : sourceNames) {
            int fdx = TechDraw::DrawUtil::getIndexFromName(s);
            if (fdx == i) {
                result = h;
                found = true;
                break;
            }
        }
        if (found) {
            break;
        }
    }
    return result;
}

TechDraw::DrawGeomHatch*
QGIViewPart::faceIsGeomHatched(int i, std::vector<TechDraw::DrawGeomHatch*> geomObjs) const
{
    TechDraw::DrawGeomHatch* result = nullptr;
    bool found = false;
    for (auto& h : geomObjs) {
        const std::vector<std::string>& sourceNames = h->Source.getSubValues();
        for (auto& sn : sourceNames) {
            int fdx = TechDraw::DrawUtil::getIndexFromName(sn);
            if (fdx == i) {
                result = h;
                found = true;
                break;
            }
            if (found) {
                break;
            }
        }
    }
    return result;
}



void QGIViewPart::dumpPath(const char* text, QPainterPath path)
{
    QPainterPath::Element elem;
    Base::Console().message(">>>%s has %d elements\n", text, path.elementCount());
    const char* typeName;
    for (int iElem = 0; iElem < path.elementCount(); iElem++) {
        elem = path.elementAt(iElem);
        if (elem.isMoveTo()) {
            typeName = "MoveTo";
        }
        else if (elem.isLineTo()) {
            typeName = "LineTo";
        }
        else if (elem.isCurveTo()) {
            typeName = "CurveTo";
        }
        else {
            typeName = "CurveData";
        }
        Base::Console().message(">>>>> element %d: type:%d/%s pos(%.3f, %.3f) M:%d L:%d C:%d\n",
                                iElem, static_cast<int>(elem.type), typeName, elem.x, elem.y, static_cast<int>(elem.isMoveTo()),
                                static_cast<int>(elem.isLineTo()), static_cast<int>(elem.isCurveTo()));
    }
}

QRectF QGIViewPart::boundingRect() const
{
    //    return childrenBoundingRect();
    //    return customChildrenBoundingRect();
    return QGIView::boundingRect();
}
void QGIViewPart::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    //    painter->drawRect(boundingRect());          //good for debugging

    QGIView::paint(painter, &myOption, widget);
}

//QGIViewPart derived classes do not need a rotate view method as rotation is handled on App side.
void QGIViewPart::rotateView() {}

bool QGIViewPart::prefFaceEdges()
{
    bool result = false;
    result = Preferences::getPreferenceGroup("General")->GetBool("DrawFaceEdges", false);
    return result;
}


Base::Color QGIViewPart::prefBreaklineColor()
{
    return  Preferences::getAccessibleColor(PreferencesGui::breaklineColor());
}

QGraphicsItem *QGIViewPart::getQGISubItemByName(const std::string &subName) const
{
    int scanType = 0;
    try {
        const std::string &subType = TechDraw::DrawUtil::getGeomTypeFromName(subName);
        if (subType == "Vertex") {
            scanType = QGIVertex::Type;
        }
        else if (subType == "Edge") {
            scanType = QGIEdge::Type;
        }
        else if (subType == "Face") {
            scanType = QGIFace::Type;
        }
    }
    catch (Base::ValueError&) {
        // No action
    }
    if (!scanType) {
        return nullptr;
    }

    int scanIndex = -1;
    try {
        scanIndex = TechDraw::DrawUtil::getIndexFromName(subName);
    }
    catch (Base::ValueError&) {
        // No action
    }
    if (scanIndex < 0) {
        return nullptr;
    }

    for (auto child : childItems()) {
        if (child->type() != scanType) {
            continue;
        }

        int projIndex;
        switch (scanType) {
            case QGIVertex::Type:
                projIndex = static_cast<QGIVertex *>(child)->getProjIndex();
                break;
            case QGIEdge::Type:
                projIndex = static_cast<QGIEdge *>(child)->getProjIndex();
                break;
            case QGIFace::Type:
                projIndex = static_cast<QGIFace *>(child)->getProjIndex();
                break;
            default:
                projIndex = -1;
                break;
        }

        if (projIndex == scanIndex) {
            return child;
        }
    }

    return nullptr;
}

void QGIViewPart::addToGroupWithoutUpdate(QGraphicsItem* item)
{
    // Implementation taken from QGraphicsItemGroup::addToGroup,
    // But delaying the costly bounding box calculation and update until all items are added

    bool ok;
    QTransform itemTransform = item->itemTransform(this, &ok);
    if (!ok) {
        qWarning("QGIViewPart::addToGroup: could not find a valid transformation from item to group coordinates");
        return;
    }
    QTransform newItemTransform(itemTransform);
    item->setPos(mapFromItem(item, 0, 0));
    item->setParentItem(this);
    if (!item->pos().isNull()) {
        newItemTransform *= QTransform::fromTranslate(-item->x(), -item->y());
    }

    // removing additional transformations properties applied with itemTransform()
    QPointF origin = item->transformOriginPoint();
    QMatrix4x4 m;
    QList<QGraphicsTransform*> transformList = item->transformations();
    for (int i = 0; i < transformList.size(); ++i)
    {
        transformList.at(i)->applyTo(&m);
    }
    newItemTransform *= m.toTransform().inverted();
    newItemTransform.translate(origin.x(), origin.y());
    newItemTransform.rotate(-item->rotation());
    newItemTransform.scale(1/item->scale(), 1/item->scale());
    newItemTransform.translate(-origin.x(), -origin.y());
    item->setTransform(newItemTransform);
}

bool QGIViewPart::getGroupSelection() {
    return DrawGuiUtil::isSelectedInTree(this);
}

void QGIViewPart::setGroupSelection(bool isSelected) {
    DrawGuiUtil::setSelectedTree(this, isSelected);
}

void QGIViewPart::setGroupSelection(bool isSelected, const std::vector<std::string> &subNames)
{
    if (subNames.empty()) {
        setSelected(isSelected);
        return;
    }

    for (const std::string &subName : subNames) {
        if (subName.empty()) {
            setSelected(isSelected);
            continue;
        }

        QGraphicsItem *subItem = getQGISubItemByName(subName);
        if (subItem) {
            subItem->setSelected(isSelected);
        }
    }
}

double QGIViewPart::getLineWidth() {
    auto vp{static_cast<ViewProviderViewPart*>(getViewProvider(getViewObject()))};

    return vp->LineWidth.getValue() * lineScaleFactor; // Thick
}

double QGIViewPart::getVertexSize() {
    return getLineWidth() * Preferences::vertexScale();
}

void QGIViewPart::updateFrameVisibility()
{
    QGIView::updateFrameVisibility();

    bool showDecorations = shouldShowFrame();
    
    for (auto& child : childItems()) {
        if (child->type() == UserType::QGIVertex) {
            child->setVisible(showDecorations || child->isSelected());
        }
        if (child->type() == UserType::QGICMark) {
            child->setVisible(showDecorations || child->isSelected() || !hideCenterMarks());
        }
    }
}
void QGIViewPart::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    QGIView::hoverEnterEvent(event);

    bool showDecorations = shouldShowFrame();

    for (auto& child : childItems()) {
        if (child->type() == UserType::QGIVertex) {
            child->setVisible(showDecorations);
            continue;
        }
        if (child->type() == UserType::QGICMark && !hideCenterMarks()) {
            child->show();
        }
    }
    update();
}

void QGIViewPart::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    QGIView::hoverLeaveEvent(event);

    bool showDecorations = shouldShowFrame();

    for (auto& child : childItems()) {
        if (child->type() == UserType::QGIVertex) {
            if (child->isSelected()) continue;
            child->setVisible(showDecorations);
            continue;
        }

        if (child->type() == UserType::QGICMark) {
            if (child->isSelected()) continue;
            if (hideCenterMarks() || !showDecorations) {
                child->hide();
            }
        }
    }
    update();
}


// returns true if vertex dots should be shown
// note this is only one of the "rules" around showing or hiding vertices.
bool QGIViewPart::showVertices() const
{
    // dvp already validated
    auto dvp(static_cast<TechDraw::DrawViewPart*>(getViewObject()));
    return !dvp->CoarseView.getValue();
}


// returns true if arc center marks should be shown
bool QGIViewPart::showCenterMarks() const
{
    // dvp and vp already validated
    auto dvp(static_cast<TechDraw::DrawViewPart*>(getViewObject()));
    auto vp(static_cast<ViewProviderViewPart*>(getViewProvider(dvp)));

    if (isExporting() && Preferences::printCenterMarks()) {
        return true;
    }

    return vp->ArcCenterMarks.getValue();
}

//! true if center marks (type of vertex) should be hidden
bool QGIViewPart::hideCenterMarks() const
{
    // printing
    if (isExporting() &&
        Preferences::printCenterMarks()) {
        return false;
    }

    // on screen
    if (showCenterMarks()) {
        return false;
    }

    return true;
}

void QGIViewPart::setMovableFlag()
{
    auto* dvp(dynamic_cast<TechDraw::DrawViewPart*>(getViewObject()));
    if (TechDraw::DrawView::isProjGroupItem(dvp)) {
        setMovableFlagProjGroupItem();
        return;
    }
    QGIView::setMovableFlag();
}

void QGIViewPart::setMovableFlagProjGroupItem()
{
    auto* dpgi(dynamic_cast<TechDraw::DrawProjGroupItem*>(getViewObject()));
    if (!dpgi) {
        return;
    }

    if (dpgi->isLocked()) {
        setFlag(QGraphicsItem::ItemIsMovable, false);
        return;
    }

    bool isAutoDist{dpgi->getPGroup()->AutoDistribute.getValue()};
    if (isAutoDist) {
        setFlag(QGraphicsItem::ItemIsMovable, false);
        return;
    }

    // not locked, not autoDistribute
    setFlag(QGraphicsItem::ItemIsMovable, true);
}
