// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2026 AstoCAD     <hello@astocad.com>                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include <algorithm>
#include <cmath>
#include <set>

#include <QCloseEvent>
#include <QColor>
#include <QImage>
#include <QLayout>
#include <QMetaObject>
#include <QMdiSubWindow>
#include <QPainter>

#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoTextureCoordinate2.h>
#include <Precision.hxx>

#include <App/DocumentObject.h>
#include <App/ElementNamingUtils.h>
#include <Base/Color.h>
#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Navigation/NavigationStyle.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/ViewProvider.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <Mod/Sketcher/App/ExternalGeometryFacade.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/Geometry.h>
#include <Mod/TechDraw/App/Preferences.h>

#include "MDIViewPage.h"
#include "QGISketch.h"
#include "QGIView.h"
#include "QGSPage.h"
#include "QGVPage.h"
#include "Rez.h"
#include "TechDrawSketchEditView.h"

using namespace TechDrawGui;

namespace
{
QColor techDrawBackgroundColor()
{
    Base::Color color;
    color.setPackedValue(
        TechDraw::Preferences::getPreferenceGroup("Colors")
            ->GetUnsigned("Background", 0x70707000)
    );
    return color.asValue<QColor>();
}

QPointF sceneToSketchPoint(const QPointF& scenePoint,
                           TechDraw::DrawView* owner,
                           QGISketch* sketchItem)
{
    // mapFromScene includes the effective projection-group position and every
    // other QGraphicsItem parent transform. The resulting point is in the
    // coordinate system in which QGISketch draws its geometry.
    const QPointF localPoint =
        sketchItem ? sketchItem->mapFromScene(scenePoint) : scenePoint;
    const double transformedX = Rez::appX(localPoint.x());
    const double transformedY = -Rez::appX(localPoint.y());
    if (!owner) {
        return {transformedX, transformedY};
    }

    const double angle = Base::toRadians(owner->Rotation.getValue());
    const double cosine = std::cos(angle);
    const double sine = std::sin(angle);
    const double scale = owner->getScale();
    if (scale <= Precision::Confusion()) {
        return {transformedX, transformedY};
    }

    return {
        (cosine * transformedX + sine * transformedY) / scale,
        (-sine * transformedX + cosine * transformedY) / scale
    };
}
}

class TechDrawGui::PageBackgroundViewProvider final: public Gui::ViewProvider
{
public:
    explicit PageBackgroundViewProvider(SoSeparator* background)
        : m_background(background)
    {
        m_background->ref();
    }

    ~PageBackgroundViewProvider() override
    {
        m_background->unref();
    }

    SoSeparator* getRoot() const override
    {
        return m_background;
    }

    bool isPartOfPhysicalObject() const override
    {
        return false;
    }

private:
    SoSeparator* m_background;
};

TechDrawSketchEditView::TechDrawSketchEditView(Gui::Document* document,
                                               MDIViewPage* pageView,
                                               App::DocumentObject* sketch,
                                               TechDraw::DrawView* owner)
    : Gui::View3DInventor(document, Gui::getMainWindow())
    , m_pageView(pageView)
    , m_sketch(sketch)
    , m_owner(owner)
{
    setWindowTitle(pageView ? pageView->windowTitle() : tr("TechDraw Sketch"));
    setWindowIcon(pageView ? pageView->windowIcon() : windowIcon());
    setProperty("SketcherExternalGeometryEnabled", false);
    setProperty("SketcherLightBackground", true);
    setProperty("SketcherDeferredGridUpdate", true);
}

TechDrawSketchEditView::~TechDrawSketchEditView()
{
    m_resetEditConnection.disconnect();
    removePageBackground();
}

const char* TechDrawSketchEditView::getName() const
{
    return "TechDrawSketchEditView";
}

bool TechDrawSketchEditView::startSketchEdit()
{
    if (!getGuiDocument() || !m_sketch) {
        return false;
    }

    m_sketchViewProvider = Gui::Application::Instance->getViewProvider(m_sketch);
    auto* documentViewProvider =
        freecad_cast<Gui::ViewProviderDocumentObject*>(m_sketchViewProvider);
    if (!documentViewProvider) {
        return false;
    }
    m_previousWorkbench = Gui::Command::assureWorkbench("SketcherWorkbench");

    if (!installOwnerExternalGeometry()) {
        restoreWorkbench();
        return false;
    }

    auto* mainWindow = Gui::getMainWindow();
    if (!installEditingView()) {
        restoreWorkbench();
        return false;
    }
    mainWindow->setActiveWindow(this);

    auto* viewer = getViewer();
    viewer->setGradientBackground(Gui::View3DInventorViewer::Background::NoGradient);
    viewer->setBackgroundColor(techDrawBackgroundColor());
    // Configure the final camera before Sketcher enters edit mode. Sketcher
    // attaches an SoNodeSensor to the current camera; replacing that camera
    // afterward can leave its delete/reattach callback in Coin's delay queue.
    viewer->setCameraType(SoOrthographicCamera::getClassTypeId());
    viewer->setAxisCross(false);
    // The normal animated viewAll() spins nested Qt event loops. Besides adding
    // roughly 200 ms to startup, that can deliver deferred destruction events
    // from the previous temporary edit view while this one is still being built.
    viewer->setAnimationEnabled(false);

    viewer->addViewProvider(m_sketchViewProvider);

    // Supplying a non-empty self subname prevents edit-parent inference from
    // redirecting a page-selected sketch to its TechDraw owner.
    if (!getGuiDocument()->setEdit(documentViewProvider, 0, "TechDrawSketchEdit")) {
        getViewer()->removeViewProvider(m_sketchViewProvider);
        restorePage();
        return false;
    }

    if (auto* mainLayout = mainWindow->layout()) {
        mainLayout->activate();
    }

    if (!m_owner) {
        alignCameraToPage();
    }

    if (auto* camera = getViewer()->getSoRenderManager()->getCamera()) {
        // Owner geometry and the page background are expressed in the
        // owner's unrotated local coordinate system. Roll the camera by the
        // inverse presentation rotation so a rotated view (notably a section
        // view whose Rotation is commonly 90 degrees) still appears on screen
        // in the same horizontal orientation as the TechDraw page.
        const double ownerRotation = m_owner ? m_owner->Rotation.getValue() : 0.0;
        camera->orientation = SbRotation(
            SbVec3f(0.0F, 0.0F, 1.0F),
            static_cast<float>(-Base::toRadians(ownerRotation))
        );
    }
    if (auto* navigation = viewer->navigationStyle()) {
        navigation->setRotationEnabled(false);
        navigation->setOrientationLocked(true);
    }

    installPageBackground();

    m_resetEditConnection = getGuiDocument()->signalResetEdit.connect(
        [this](const Gui::ViewProviderDocumentObject& viewProvider) {
            if (viewProvider.getObject() != m_sketch) {
                return;
            }
            const QPointer<TechDrawSketchEditView> guardedThis(this);
            QMetaObject::invokeMethod(this, [guardedThis]() {
                if (guardedThis) {
                    guardedThis->restorePage();
                }
            }, Qt::QueuedConnection);
        }
    );
    return true;
}

bool TechDrawSketchEditView::installOwnerExternalGeometry()
{
    auto* partOwner = freecad_cast<TechDraw::DrawViewPart*>(m_owner);
    if (!partOwner) {
        return true;
    }

    auto* sketch = freecad_cast<Sketcher::SketchObject*>(m_sketch);
    if (!sketch) {
        return false;
    }

    const auto projectedEdges = partOwner->getEdgeGeometry();
    std::set<std::string> desiredEdges;
    for (std::size_t edgeIndex = 0; edgeIndex < projectedEdges.size(); ++edgeIndex) {
        const auto& edge = projectedEdges[edgeIndex];
        if (edge && edge->getHlrVisible()) {
            desiredEdges.insert("Edge" + std::to_string(edgeIndex));
        }
    }

    const auto linkedObjects = sketch->ExternalGeometry.getValues();
    const auto linkedSubElements = sketch->ExternalGeometry.getSubValues(false);
    const auto& externalGeometry = sketch->getExternalGeometry();
    std::set<std::string> linkedOwnerEdges;
    std::vector<int> obsoleteExternalIds;
    const std::size_t linkCount = std::min(linkedObjects.size(), linkedSubElements.size());
    for (std::size_t linkIndex = 0; linkIndex < linkCount; ++linkIndex) {
        if (linkedObjects[linkIndex] != partOwner) {
            continue;
        }

        const auto& edgeName = linkedSubElements[linkIndex];
        if (desiredEdges.find(edgeName) == desiredEdges.end()) {
            // delExternal() addresses the expanded ExternalGeo array, not the
            // LinkSub property row. Find an entity carrying this row's actual
            // stable reference key; delExternal() will remove the complete
            // expanded group. getRefIndex() cannot be used here because it is
            // a migration field that Sketcher resets after resolving links.
            const std::string expectedReference =
                std::string(partOwner->getNameInDocument()) + '.'
                + Data::newElementName(edgeName.c_str());
            // ExternalGeo slots 0 and 1 are the horizontal and vertical
            // sketch axes. delExternal(0) addresses slot 2.
            for (std::size_t externalIndex = 2;
                 externalIndex < externalGeometry.size();
                 ++externalIndex) {
                const auto facade = Sketcher::ExternalGeometryFacade::getFacade(
                    externalGeometry[externalIndex]
                );
                if (facade && facade->getRef() == expectedReference) {
                    obsoleteExternalIds.push_back(
                        static_cast<int>(externalIndex) - 2
                    );
                    break;
                }
            }
        }
        else {
            linkedOwnerEdges.insert(edgeName);
        }
    }

    std::vector<std::string> missingEdges;
    for (const auto& edgeName : desiredEdges) {
        if (linkedOwnerEdges.find(edgeName) == linkedOwnerEdges.end()) {
            missingEdges.push_back(edgeName);
        }
    }

    if (obsoleteExternalIds.empty() && missingEdges.empty()) {
        // Rebuild linked geometry so changes in the TechDraw-to-Sketcher
        // coordinate conversion are also applied to existing sketches.
        sketch->rebuildExternalGeometry();
        return true;
    }

    const int transactionId = Gui::Command::openActiveDocumentCommand(
        QT_TRANSLATE_NOOP("Command", "Update TechDraw external geometry")
    );

    bool success = true;
    if (!obsoleteExternalIds.empty()) {
        // Remove dead property links before anything asks Sketcher to rebuild
        // them. This avoids one projection error and one missing-reference
        // error for every edge that disappeared from the TechDraw view.
        success = sketch->delExternal(obsoleteExternalIds) == 0;
    }
    if (success && !missingEdges.empty()) {
        success = sketch->addExternals(partOwner, missingEdges) > 0;
    }
    else if (success) {
        sketch->rebuildExternalGeometry();
    }

    if (success) {
        Gui::Command::commitCommand(transactionId);
    }
    else {
        Gui::Command::abortCommand(transactionId);
    }
    return success;
}

bool TechDrawSketchEditView::installEditingView()
{
    if (!m_pageView) {
        return false;
    }

    m_pageSubWindow = qobject_cast<QMdiSubWindow*>(m_pageView->parentWidget());
    if (!m_pageSubWindow) {
        return false;
    }

    const QString pageTitle = m_pageView->windowTitle();
    m_pageView->hide();
    m_pageSubWindow->setWidget(this);
    setWindowTitle(pageTitle);
    m_pageSubWindow->setWindowTitle(pageTitle);
    show();
    return true;
}

void TechDrawSketchEditView::installPageBackground()
{
    if (!m_pageView || !getViewer()) {
        return;
    }

    auto* pageGraphicsView = qobject_cast<QGVPage*>(m_pageView->centralWidget());
    QGSPage* pageScene = pageGraphicsView ? pageGraphicsView->getScene() : nullptr;
    if (!pageGraphicsView || !pageScene) {
        return;
    }

    QGISketch* editedSketch =
        pageScene->findSketchForDocObj(m_sketch);
    const bool sketchWasVisible = editedSketch && editedSketch->isVisible();
    if (editedSketch) {
        editedSketch->setVisible(false);
    }
    // Part views are replaced by their projected external geometry while
    // editing. A projection group has no direct geometry and stays visible.
    auto* partOwner = freecad_cast<TechDraw::DrawViewPart*>(m_owner);
    QGIView* ownerView = partOwner ? pageScene->findQViewForDocObj(partOwner) : nullptr;
    const bool ownerWasVisible = ownerView && ownerView->isVisible();
    if (ownerView) {
        ownerView->setVisible(false);
    }

    QRectF sourceRect = pageScene->itemsBoundingRect();
    if (sourceRect.isEmpty()) {
        sourceRect = pageScene->sceneRect();
    }
    const double margin =
        std::max(Rez::guiX(10.0), 0.02 * std::max(sourceRect.width(), sourceRect.height()));
    sourceRect.adjust(-margin, -margin, margin, margin);

    constexpr double preferredPixelsPerSceneUnit = 1.0;
    constexpr double maximumImageDimension = 4096.0;
    const double imageScale = std::min({
        preferredPixelsPerSceneUnit,
        maximumImageDimension / sourceRect.width(),
        maximumImageDimension / sourceRect.height()
    });
    const QSize imageSize(
        std::max(1, static_cast<int>(std::ceil(sourceRect.width() * imageScale))),
        std::max(1, static_cast<int>(std::ceil(sourceRect.height() * imageScale)))
    );
    QImage pageImage(imageSize, QImage::Format_RGB32);
    pageImage.fill(techDrawBackgroundColor());
    {
        QPainter painter(&pageImage);
        painter.setRenderHints(
            QPainter::Antialiasing | QPainter::TextAntialiasing
                | QPainter::SmoothPixmapTransform
        );
        pageScene->render(
            &painter,
            QRectF(QPointF(0.0, 0.0), QSizeF(imageSize)),
            sourceRect,
            Qt::IgnoreAspectRatio
        );
    }
    if (editedSketch) {
        editedSketch->setVisible(sketchWasVisible);
    }
    if (ownerView) {
        ownerView->setVisible(ownerWasVisible);
    }
    auto* background = new SoSeparator;
    auto* pickStyle = new SoPickStyle;
    pickStyle->style = SoPickStyle::UNPICKABLE;
    background->addChild(pickStyle);

    auto* lightModel = new SoLightModel;
    lightModel->model = SoLightModel::BASE_COLOR;
    background->addChild(lightModel);

    // Coin's default diffuse material is 0.8 grey and would darken every
    // texture pixel. White preserves the exact TechDraw scene RGB values.
    auto* material = new SoMaterial;
    material->diffuseColor.setValue(1.0F, 1.0F, 1.0F);
    background->addChild(material);

    auto* texture = new SoTexture2;
    texture->wrapS = SoTexture2::CLAMP;
    texture->wrapT = SoTexture2::CLAMP;
    Gui::BitmapFactory().convert(pageImage, texture->image);
    background->addChild(texture);

    auto* textureCoordinates = new SoTextureCoordinate2;
    textureCoordinates->point.set1Value(0, 0.0F, 0.0F);
    textureCoordinates->point.set1Value(1, 1.0F, 0.0F);
    textureCoordinates->point.set1Value(2, 1.0F, 1.0F);
    textureCoordinates->point.set1Value(3, 0.0F, 1.0F);
    background->addChild(textureCoordinates);

    const QPointF bottomLeft =
        sceneToSketchPoint({sourceRect.left(), sourceRect.bottom()}, m_owner, editedSketch);
    const QPointF bottomRight =
        sceneToSketchPoint({sourceRect.right(), sourceRect.bottom()}, m_owner, editedSketch);
    const QPointF topRight =
        sceneToSketchPoint({sourceRect.right(), sourceRect.top()}, m_owner, editedSketch);
    const QPointF topLeft =
        sceneToSketchPoint({sourceRect.left(), sourceRect.top()}, m_owner, editedSketch);

    auto* coordinates = new SoCoordinate3;
    coordinates->point.set1Value(
        0,
        static_cast<float>(bottomLeft.x()),
        static_cast<float>(bottomLeft.y()),
        -1.0F
    );
    coordinates->point.set1Value(
        1,
        static_cast<float>(bottomRight.x()),
        static_cast<float>(bottomRight.y()),
        -1.0F
    );
    coordinates->point.set1Value(
        2,
        static_cast<float>(topRight.x()),
        static_cast<float>(topRight.y()),
        -1.0F
    );
    coordinates->point.set1Value(
        3,
        static_cast<float>(topLeft.x()),
        static_cast<float>(topLeft.y()),
        -1.0F
    );
    background->addChild(coordinates);

    auto* face = new SoFaceSet;
    face->numVertices.set1Value(0, 4);
    background->addChild(face);

    m_pageBackgroundProvider = new PageBackgroundViewProvider(background);
    getViewer()->addViewProvider(m_pageBackgroundProvider);
}

void TechDrawSketchEditView::removePageBackground()
{
    if (!m_pageBackgroundProvider) {
        return;
    }

    if (getViewer()) {
        getViewer()->removeViewProvider(m_pageBackgroundProvider);
    }
    delete m_pageBackgroundProvider;
    m_pageBackgroundProvider = nullptr;
}

void TechDrawSketchEditView::restoreWorkbench()
{
    if (m_previousWorkbench.empty()) {
        return;
    }

    Gui::Command::assureWorkbench(m_previousWorkbench.c_str());
    m_previousWorkbench.clear();
}

void TechDrawSketchEditView::alignCameraToPage()
{
    if (!m_pageView) {
        return;
    }

    auto* pageGraphicsView = qobject_cast<QGVPage*>(m_pageView->centralWidget());
    auto* camera = dynamic_cast<SoOrthographicCamera*>(
        getViewer()->getSoRenderManager()->getCamera()
    );
    if (!pageGraphicsView || !pageGraphicsView->viewport() || !camera) {
        getViewer()->viewAll();
        return;
    }

    const QRect viewportRect = pageGraphicsView->viewport()->rect();
    const QPointF sceneCenter = pageGraphicsView->mapToScene(viewportRect.center());
    const double pageScale = std::abs(pageGraphicsView->transform().m22());
    const double sceneUnitsPerMillimetre = Rez::guiX(1.0);
    if (pageScale <= 0.0 || sceneUnitsPerMillimetre <= 0.0) {
        getViewer()->viewAll();
        return;
    }

    const SbVec3f oldPosition = camera->position.getValue();
    camera->position.setValue(
        static_cast<float>(Rez::appX(sceneCenter.x())),
        static_cast<float>(-Rez::appX(sceneCenter.y())),
        oldPosition[2]
    );
    camera->height.setValue(
        static_cast<float>(
            viewportRect.height() / pageScale / sceneUnitsPerMillimetre
        )
    );
}

void TechDrawSketchEditView::restorePage()
{
    if (m_restoring) {
        return;
    }
    m_restoring = true;
    m_resetEditConnection.disconnect();

    if (m_pageView && m_pageSubWindow) {
        removePageBackground();
        hide();
        m_pageSubWindow->setWidget(m_pageView);
        m_pageView->show();
        m_pageSubWindow->setWindowTitle(m_pageView->windowTitle());
        Gui::getMainWindow()->setActiveWindow(m_pageView);
    }
    restoreWorkbench();

    m_pageSubWindow = nullptr;
    setParent(Gui::getMainWindow());
    close();
}

void TechDrawSketchEditView::closeEvent(QCloseEvent* event)
{
    if (!m_restoring && getGuiDocument()
        && getGuiDocument()->getEditViewProvider() == m_sketchViewProvider) {
        event->ignore();
        getGuiDocument()->resetEdit();
        return;
    }

    Gui::View3DInventor::closeEvent(event);
}
