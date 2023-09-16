/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <QApplication>
#include <QMenu>
#include <QMessageBox>

#include <Inventor/SoPickedPoint.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/events/SoButtonEvent.h>
#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoMaterialBinding.h>
#include <Inventor/nodes/SoNormal.h>
#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/nodes/SoShapeHints.h>
#endif

#include <Gui/View3DInventorViewer.h>

#include <App/GeoFeature.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Flag.h>
#include <Gui/MainWindow.h>
#include <Gui/SoFCColorBar.h>

#include <Gui/Widgets.h>
#include <Mod/Inspection/App/InspectionFeature.h>
#include <Mod/Points/App/Properties.h>

#include "ViewProviderInspection.h"


using namespace InspectionGui;


bool ViewProviderInspection::addflag = false;
App::PropertyFloatConstraint::Constraints ViewProviderInspection::floatRange = {1.0, 64.0, 1.0};

PROPERTY_SOURCE(InspectionGui::ViewProviderInspection, Gui::ViewProviderDocumentObject)

ViewProviderInspection::ViewProviderInspection()
{
    ADD_PROPERTY_TYPE(OutsideGrayed,
                      (false),
                      "",
                      (App::PropertyType)(App::Prop_Output | App::Prop_Hidden),
                      "");
    ADD_PROPERTY_TYPE(PointSize,
                      (1.0),
                      "Display",
                      (App::PropertyType)(App::Prop_None /*App::Prop_Hidden*/),
                      "");
    PointSize.setConstraints(&floatRange);

    pcColorRoot = new SoSeparator();
    pcColorRoot->ref();
    pcMatBinding = new SoMaterialBinding;
    pcMatBinding->ref();
    pcColorMat = new SoMaterial;
    pcColorMat->ref();
    pcColorStyle = new SoDrawStyle();
    pcColorRoot->addChild(pcColorStyle);
    pcCoords = new SoCoordinate3;
    pcCoords->ref();
    // simple color bar
    pcColorBar = new Gui::SoFCColorBar;
    pcColorBar->Attach(this);
    pcColorBar->ref();
    pcColorBar->setRange(-0.1f, 0.1f, 3);
    pcLinkRoot = new SoGroup;
    pcLinkRoot->ref();

    pcPointStyle = new SoDrawStyle();
    pcPointStyle->ref();
    pcPointStyle->style = SoDrawStyle::POINTS;
    pcPointStyle->pointSize = PointSize.getValue();
    SelectionStyle.setValue(1);  // BBOX
}

ViewProviderInspection::~ViewProviderInspection()
{
    pcColorRoot->unref();
    pcCoords->unref();
    pcMatBinding->unref();
    pcColorMat->unref();
    pcColorBar->Detach(this);
    pcColorBar->unref();
    pcLinkRoot->unref();
    pcPointStyle->unref();
}

void ViewProviderInspection::onChanged(const App::Property* prop)
{
    if (prop == &OutsideGrayed) {
        if (pcColorBar) {
            pcColorBar->setOutsideGrayed(OutsideGrayed.getValue());
            pcColorBar->Notify(0);
        }
    }
    else if (prop == &PointSize) {
        pcPointStyle->pointSize = PointSize.getValue();
    }
    else {
        inherited::onChanged(prop);
    }
}

void ViewProviderInspection::hide()
{
    inherited::hide();
    pcColorStyle->style = SoDrawStyle::INVISIBLE;
}

void ViewProviderInspection::show()
{
    inherited::show();
    pcColorStyle->style = SoDrawStyle::FILLED;
}

void ViewProviderInspection::attach(App::DocumentObject* pcFeat)
{
    // creates the standard viewing modes
    inherited::attach(pcFeat);

    SoShapeHints* flathints = new SoShapeHints;
    flathints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    flathints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;

    SoGroup* pcColorShadedRoot = new SoGroup();
    pcColorShadedRoot->addChild(flathints);

    // color shaded  ------------------------------------------
    SoDrawStyle* pcFlatStyle = new SoDrawStyle();
    pcFlatStyle->style = SoDrawStyle::FILLED;
    pcColorShadedRoot->addChild(pcFlatStyle);

    pcColorShadedRoot->addChild(pcColorMat);
    pcColorShadedRoot->addChild(pcMatBinding);
    pcColorShadedRoot->addChild(pcLinkRoot);

    addDisplayMaskMode(pcColorShadedRoot, "ColorShaded");

    // Check for an already existing color bar
    Gui::SoFCColorBar* pcBar =
        ((Gui::SoFCColorBar*)findFrontRootOfType(Gui::SoFCColorBar::getClassTypeId()));
    if (pcBar) {
        float fMin = pcColorBar->getMinValue();
        float fMax = pcColorBar->getMaxValue();

        // Attach to the foreign color bar and delete our own bar
        pcBar->Attach(this);
        pcBar->ref();
        pcBar->setRange(fMin, fMax, 3);
        pcBar->Notify(0);
        pcColorBar->Detach(this);
        pcColorBar->unref();
        pcColorBar = pcBar;
    }

    pcColorRoot->addChild(pcColorBar);
}

bool ViewProviderInspection::setupFaces(const Data::ComplexGeoData* data)
{
    std::vector<Base::Vector3d> points;
    std::vector<Data::ComplexGeoData::Facet> faces;

    // set the Distance property to the correct size to sync size of material node with number
    // of vertices/points of the referenced geometry
    double accuracy = data->getAccuracy();
    data->getFaces(points, faces, accuracy);
    if (faces.empty()) {
        return false;
    }

    setupCoords(points);
    setupFaceIndexes(faces);
    return true;
}

bool ViewProviderInspection::setupLines(const Data::ComplexGeoData* data)
{
    std::vector<Base::Vector3d> points;
    std::vector<Data::ComplexGeoData::Line> lines;

    double accuracy = data->getAccuracy();
    data->getLines(points, lines, accuracy);
    if (lines.empty()) {
        return false;
    }

    setupCoords(points);
    setupLineIndexes(lines);
    return true;
}

bool ViewProviderInspection::setupPoints(const Data::ComplexGeoData* data,
                                         App::PropertyContainer* container)
{
    std::vector<Base::Vector3d> points;
    std::vector<Base::Vector3f> normals;
    std::vector<Base::Vector3d> normals_d;
    double accuracy = data->getAccuracy();
    data->getPoints(points, normals_d, accuracy);
    if (points.empty()) {
        return false;
    }

    normals.reserve(normals_d.size());
    std::transform(normals_d.cbegin(),
                   normals_d.cend(),
                   std::back_inserter(normals),
                   [](const Base::Vector3d& p) {
                       return Base::toVector<float>(p);
                   });

    // If getPoints() doesn't deliver normals check a second property
    if (normals.empty() && container) {
        App::Property* propN = container->getPropertyByName("Normal");
        if (propN
            && propN->getTypeId().isDerivedFrom(Points::PropertyNormalList::getClassTypeId())) {
            normals = static_cast<Points::PropertyNormalList*>(propN)->getValues();
        }
    }

    setupCoords(points);
    if (!normals.empty() && normals.size() == points.size()) {
        setupNormals(normals);
    }

    this->pcLinkRoot->addChild(this->pcPointStyle);
    this->pcLinkRoot->addChild(new SoPointSet());

    return true;
}

void ViewProviderInspection::setupCoords(const std::vector<Base::Vector3d>& points)
{
    this->pcLinkRoot->addChild(this->pcCoords);
    this->pcCoords->point.setNum(points.size());
    SbVec3f* pts = this->pcCoords->point.startEditing();
    for (size_t i = 0; i < points.size(); i++) {
        const Base::Vector3d& p = points[i];
        pts[i].setValue((float)p.x, (float)p.y, (float)p.z);
    }
    this->pcCoords->point.finishEditing();
}

void ViewProviderInspection::setupNormals(const std::vector<Base::Vector3f>& normals)
{
    SoNormal* normalNode = new SoNormal();
    normalNode->vector.setNum(normals.size());
    SbVec3f* norm = normalNode->vector.startEditing();

    std::size_t i = 0;
    for (const auto& it : normals) {
        norm[i++].setValue(it.x, it.y, it.z);
    }

    normalNode->vector.finishEditing();
    this->pcLinkRoot->addChild(normalNode);
}

void ViewProviderInspection::setupLineIndexes(const std::vector<Data::ComplexGeoData::Line>& lines)
{
    SoIndexedLineSet* line = new SoIndexedLineSet();
    this->pcLinkRoot->addChild(line);
    line->coordIndex.setNum(3 * lines.size());
    int32_t* indices = line->coordIndex.startEditing();
    unsigned long j = 0;
    for (const auto& it : lines) {
        indices[3 * j + 0] = it.I1;
        indices[3 * j + 1] = it.I2;
        indices[3 * j + 2] = SO_END_LINE_INDEX;
        j++;
    }
    line->coordIndex.finishEditing();
}

void ViewProviderInspection::setupFaceIndexes(const std::vector<Data::ComplexGeoData::Facet>& faces)
{
    SoIndexedFaceSet* face = new SoIndexedFaceSet();
    this->pcLinkRoot->addChild(face);
    face->coordIndex.setNum(4 * faces.size());
    int32_t* indices = face->coordIndex.startEditing();
    unsigned long j = 0;
    for (const auto& it : faces) {
        indices[4 * j + 0] = it.I1;
        indices[4 * j + 1] = it.I2;
        indices[4 * j + 2] = it.I3;
        indices[4 * j + 3] = SO_END_FACE_INDEX;
        j++;
    }
    face->coordIndex.finishEditing();
}

void ViewProviderInspection::updateData(const App::Property* prop)
{
    // set to the expected size
    if (prop->getTypeId().isDerivedFrom(App::PropertyLink::getClassTypeId())) {
        App::GeoFeature* object =
            static_cast<const App::PropertyLink*>(prop)->getValue<App::GeoFeature*>();
        const App::PropertyComplexGeoData* propData =
            object ? object->getPropertyOfGeometry() : nullptr;
        if (propData) {
            Gui::coinRemoveAllChildren(this->pcLinkRoot);

            const Data::ComplexGeoData* data = propData->getComplexData();
            if (!setupFaces(data)) {
                if (!setupLines(data)) {
                    setupPoints(data, object);
                }
            }
        }
    }
    else if (prop->getTypeId() == Inspection::PropertyDistanceList::getClassTypeId()) {
        // force an update of the Inventor data nodes
        if (this->pcObject) {
            App::Property* link = this->pcObject->getPropertyByName("Actual");
            if (link) {
                updateData(link);
            }
            setDistances();
        }
    }
    else if (prop->getTypeId() == App::PropertyFloat::getClassTypeId()) {
        if (strcmp(prop->getName(), "SearchRadius") == 0) {
            float fSearchRadius = ((App::PropertyFloat*)prop)->getValue();
            this->search_radius = fSearchRadius;
            pcColorBar->setRange(-fSearchRadius, fSearchRadius, 4);
            pcColorBar->Notify(0);
        }
    }
}

SoSeparator* ViewProviderInspection::getFrontRoot() const
{
    return pcColorRoot;
}

void ViewProviderInspection::setDistances()
{
    if (!pcObject) {
        return;
    }

    App::Property* pDistances = pcObject->getPropertyByName("Distances");
    if (!pDistances) {
        SoDebugError::post("ViewProviderInspection::setDistances", "Unknown property 'Distances'");
        return;
    }
    if (pDistances->getTypeId() != Inspection::PropertyDistanceList::getClassTypeId()) {
        SoDebugError::post(
            "ViewProviderInspection::setDistances",
            "Property 'Distances' has type %s (Inspection::PropertyDistanceList was expected)",
            pDistances->getTypeId().getName());
        return;
    }

    // distance values
    const std::vector<float>& fValues =
        static_cast<Inspection::PropertyDistanceList*>(pDistances)->getValues();
    if ((int)fValues.size() != this->pcCoords->point.getNum()) {
        pcMatBinding->value = SoMaterialBinding::OVERALL;
        return;
    }

    if (pcColorMat->diffuseColor.getNum() != static_cast<int>(fValues.size())) {
        pcColorMat->diffuseColor.setNum(static_cast<int>(fValues.size()));
    }
    if (pcColorMat->transparency.getNum() != static_cast<int>(fValues.size())) {
        pcColorMat->transparency.setNum(static_cast<int>(fValues.size()));
    }

    SbColor* cols = pcColorMat->diffuseColor.startEditing();
    float* tran = pcColorMat->transparency.startEditing();

    unsigned long j = 0;
    for (std::vector<float>::const_iterator jt = fValues.begin(); jt != fValues.end(); ++jt, j++) {
        App::Color col = pcColorBar->getColor(*jt);
        cols[j] = SbColor(col.r, col.g, col.b);
        if (pcColorBar->isVisible(*jt)) {
            tran[j] = 0.0f;
        }
        else {
            tran[j] = 0.8f;
        }
    }

    pcColorMat->diffuseColor.finishEditing();
    pcColorMat->transparency.finishEditing();
    pcMatBinding->value = SoMaterialBinding::PER_VERTEX_INDEXED;
}

QIcon ViewProviderInspection::getIcon() const
{
    // Get the icon of the view provider to the associated feature
    QIcon px = inherited::getIcon();
    App::Property* pActual = pcObject->getPropertyByName("Actual");
    if (pActual && pActual->getTypeId().isDerivedFrom(App::PropertyLink::getClassTypeId())) {
        App::DocumentObject* docobj = ((App::PropertyLink*)pActual)->getValue();
        if (docobj) {
            Gui::Document* doc = Gui::Application::Instance->getDocument(docobj->getDocument());
            Gui::ViewProvider* view = doc->getViewProvider(docobj);
            px = view->getIcon();
        }
    }

    return px;
}

void ViewProviderInspection::setDisplayMode(const char* ModeName)
{
    if (strcmp("Visual Inspection", ModeName) == 0) {
        setDistances();
        setDisplayMaskMode("ColorShaded");
    }

    inherited::setDisplayMode(ModeName);
}

std::vector<std::string> ViewProviderInspection::getDisplayModes() const
{
    // add modes
    std::vector<std::string> StrList;
    StrList.emplace_back("Visual Inspection");
    return StrList;
}

void ViewProviderInspection::OnChange(Base::Subject<int>& /*rCaller*/, int /*rcReason*/)
{
    setActiveMode();
}

namespace InspectionGui
{
// Proxy class that receives an asynchronous custom event
class ViewProviderProxyObject: public QObject
{
public:
    explicit ViewProviderProxyObject(QWidget* w)
        : QObject(nullptr)
        , widget(w)
    {}
    ~ViewProviderProxyObject() override = default;
    void customEvent(QEvent*) override
    {
        if (!widget.isNull()) {
            QList<Gui::Flag*> flags = widget->findChildren<Gui::Flag*>();
            if (!flags.isEmpty()) {
                int ret =
                    QMessageBox::question(Gui::getMainWindow(),
                                          QObject::tr("Remove annotations"),
                                          QObject::tr("Do you want to remove all annotations?"),
                                          QMessageBox::Yes,
                                          QMessageBox::No);
                if (ret == QMessageBox::Yes) {
                    for (auto it : flags) {
                        it->deleteLater();
                    }
                }
            }
        }

        this->deleteLater();
    }

    static void
    addFlag(Gui::View3DInventorViewer* view, const QString& text, const SoPickedPoint* point)
    {
        Gui::Flag* flag = new Gui::Flag;
        QPalette p;
        p.setColor(QPalette::Window, QColor(85, 0, 127));
        p.setColor(QPalette::Text, QColor(220, 220, 220));
        flag->setPalette(p);
        flag->setText(text);
        flag->setOrigin(point->getPoint());
        Gui::GLFlagWindow* flags = nullptr;
        std::list<Gui::GLGraphicsItem*> glItems =
            view->getGraphicsItemsOfType(Gui::GLFlagWindow::getClassTypeId());
        if (glItems.empty()) {
            flags = new Gui::GLFlagWindow(view);
            view->addGraphicsItem(flags);
        }
        else {
            flags = static_cast<Gui::GLFlagWindow*>(glItems.front());
        }
        flags->addFlag(flag, Gui::FlagLayout::BottomLeft);
    }

private:
    QPointer<QWidget> widget;
};
}  // namespace InspectionGui

void ViewProviderInspection::inspectCallback(void* ud, SoEventCallback* n)
{
    Gui::View3DInventorViewer* view = static_cast<Gui::View3DInventorViewer*>(n->getUserData());
    const SoEvent* ev = n->getEvent();
    if (ev->getTypeId() == SoMouseButtonEvent::getClassTypeId()) {
        const SoMouseButtonEvent* mbe = static_cast<const SoMouseButtonEvent*>(ev);

        // Mark all incoming mouse button events as handled, especially, to deactivate the selection
        // node
        n->getAction()->setHandled();
        n->setHandled();
        if (mbe->getButton() == SoMouseButtonEvent::BUTTON2
            && mbe->getState() == SoButtonEvent::UP) {
            n->setHandled();
            // context-menu
            QMenu menu;
            QAction* fl = menu.addAction(QObject::tr("Annotation"));
            fl->setCheckable(true);
            fl->setChecked(addflag);
            QAction* cl = menu.addAction(QObject::tr("Leave info mode"));
            QAction* id = menu.exec(QCursor::pos());
            if (fl == id) {
                addflag = fl->isChecked();
            }
            else if (cl == id) {
                // post an event to a proxy object to make sure to avoid problems
                // when opening a modal dialog
                QApplication::postEvent(new ViewProviderProxyObject(view->getGLWidget()),
                                        new QEvent(QEvent::User));
                view->setEditing(false);
                view->getWidget()->setCursor(QCursor(Qt::ArrowCursor));
                view->setRedirectToSceneGraph(false);
                view->setRedirectToSceneGraphEnabled(false);
                view->setSelectionEnabled(true);
                view->removeEventCallback(SoButtonEvent::getClassTypeId(), inspectCallback, ud);
            }
        }
        else if (mbe->getButton() == SoMouseButtonEvent::BUTTON1
                 && mbe->getState() == SoButtonEvent::UP) {
            const SoPickedPoint* point = n->getPickedPoint();
            if (!point) {
                Base::Console().Message("No point picked.\n");
                return;
            }

            n->setHandled();

            // check if we have picked one a node of the view provider we are insterested in
            Gui::ViewProvider* vp = view->getViewProviderByPathFromTail(point->getPath());
            if (vp && vp->getTypeId().isDerivedFrom(ViewProviderInspection::getClassTypeId())) {
                ViewProviderInspection* that = static_cast<ViewProviderInspection*>(vp);
                QString info = that->inspectDistance(point);
                Gui::getMainWindow()->setPaneText(1, info);
                if (addflag) {
                    ViewProviderProxyObject::addFlag(view, info, point);
                }
                else {
                    Gui::ToolTip::showText(QCursor::pos(), info);
                }
            }
            else {
                // the nearest picked point was not part of the view provider
                SoRayPickAction action(view->getSoRenderManager()->getViewportRegion());
                action.setPickAll(true);
                action.setPoint(mbe->getPosition());
                action.apply(view->getSoRenderManager()->getSceneGraph());

                const SoPickedPointList& pps = action.getPickedPointList();
                for (int i = 0; i < pps.getLength(); ++i) {
                    const SoPickedPoint* point = pps[i];
                    vp = view->getViewProviderByPathFromTail(point->getPath());
                    if (vp
                        && vp->getTypeId().isDerivedFrom(
                            ViewProviderInspection::getClassTypeId())) {
                        ViewProviderInspection* self = static_cast<ViewProviderInspection*>(vp);
                        QString info = self->inspectDistance(point);
                        Gui::getMainWindow()->setPaneText(1, info);
                        if (addflag) {
                            ViewProviderProxyObject::addFlag(view, info, point);
                        }
                        else {
                            Gui::ToolTip::showText(QCursor::pos(), info);
                        }
                        break;
                    }
                }
            }
        }
    }
    // toggle between inspection and navigation mode
    else if (ev->getTypeId().isDerivedFrom(SoKeyboardEvent::getClassTypeId())) {
        const SoKeyboardEvent* const ke = static_cast<const SoKeyboardEvent*>(ev);
        if (ke->getState() == SoButtonEvent::DOWN && ke->getKey() == SoKeyboardEvent::ESCAPE) {
            SbBool toggle = view->isRedirectedToSceneGraph();
            view->setRedirectToSceneGraph(!toggle);
            n->setHandled();
        }
    }
}

namespace InspectionGui
{
float calcArea(const SbVec3f& v1, const SbVec3f& v2, const SbVec3f& v3)
{
    SbVec3f a = v2 - v1;
    SbVec3f b = v3 - v1;
    return a.cross(b).length() / 2.0f;
}

bool calcWeights(const SbVec3f& v1,
                 const SbVec3f& v2,
                 const SbVec3f& v3,
                 const SbVec3f& p,
                 float& w0,
                 float& w1,
                 float& w2)
{
    float fAreaABC = calcArea(v1, v2, v3);
    float fAreaPBC = calcArea(p, v2, v3);
    float fAreaPCA = calcArea(p, v3, v1);
    float fAreaPAB = calcArea(p, v1, v2);

    w0 = fAreaPBC / fAreaABC;
    w1 = fAreaPCA / fAreaABC;
    w2 = fAreaPAB / fAreaABC;

    return fabs(w0 + w1 + w2 - 1.0f) < 0.001f;
}
}  // namespace InspectionGui

QString ViewProviderInspection::inspectDistance(const SoPickedPoint* pp) const
{
    QString info;
    const SoDetail* detail = pp->getDetail(pp->getPath()->getTail());
    if (detail && detail->getTypeId() == SoFaceDetail::getClassTypeId()) {
        // get the distances of the three points of the picked facet
        const SoFaceDetail* facedetail = static_cast<const SoFaceDetail*>(detail);
        App::Property* pDistance = this->pcObject->getPropertyByName("Distances");
        if (pDistance
            && pDistance->getTypeId() == Inspection::PropertyDistanceList::getClassTypeId()) {
            Inspection::PropertyDistanceList* dist =
                static_cast<Inspection::PropertyDistanceList*>(pDistance);
            int index1 = facedetail->getPoint(0)->getCoordinateIndex();
            int index2 = facedetail->getPoint(1)->getCoordinateIndex();
            int index3 = facedetail->getPoint(2)->getCoordinateIndex();
            float fVal1 = (*dist)[index1];
            float fVal2 = (*dist)[index2];
            float fVal3 = (*dist)[index3];

            App::Property* pActual = this->pcObject->getPropertyByName("Actual");
            if (pActual
                && pActual->getTypeId().isDerivedFrom(App::PropertyLink::getClassTypeId())) {
                float fSearchRadius = this->search_radius;
                if (fVal1 > fSearchRadius || fVal2 > fSearchRadius || fVal3 > fSearchRadius) {
                    info = QObject::tr("Distance: > %1").arg(fSearchRadius);
                }
                else if (fVal1 < -fSearchRadius || fVal2 < -fSearchRadius
                         || fVal3 < -fSearchRadius) {
                    info = QObject::tr("Distance: < %1").arg(-fSearchRadius);
                }
                else {
                    SoSearchAction searchAction;
                    searchAction.setType(SoCoordinate3::getClassTypeId());
                    searchAction.setInterest(SoSearchAction::FIRST);
                    searchAction.apply(pp->getPath()->getNodeFromTail(1));
                    SoPath* selectionPath = searchAction.getPath();

                    if (selectionPath) {
                        SoCoordinate3* coords =
                            static_cast<SoCoordinate3*>(selectionPath->getTail());
                        const SbVec3f& v1 = coords->point[index1];
                        const SbVec3f& v2 = coords->point[index2];
                        const SbVec3f& v3 = coords->point[index3];
                        const SbVec3f& p = pp->getObjectPoint();
                        // get the weights
                        float w1, w2, w3;
                        calcWeights(v1, v2, v3, p, w1, w2, w3);
                        float fVal = w1 * fVal1 + w2 * fVal2 + w3 * fVal3;
                        info = QObject::tr("Distance: %1").arg(fVal);
                    }
                }
            }
        }
    }
    else if (detail && detail->getTypeId() == SoPointDetail::getClassTypeId()) {
        // safe downward cast, know the type
        const SoPointDetail* pointdetail = static_cast<const SoPointDetail*>(detail);

        // get the distance of the picked point
        int index = pointdetail->getCoordinateIndex();
        App::Property* prop = this->pcObject->getPropertyByName("Distances");
        if (prop && prop->getTypeId() == Inspection::PropertyDistanceList::getClassTypeId()) {
            Inspection::PropertyDistanceList* dist =
                static_cast<Inspection::PropertyDistanceList*>(prop);
            float fVal = (*dist)[index];
            info = QObject::tr("Distance: %1").arg(fVal);
        }
    }

    return info;
}

// -----------------------------------------------

PROPERTY_SOURCE(InspectionGui::ViewProviderInspectionGroup, Gui::ViewProviderDocumentObjectGroup)


/**
 * Creates the view provider for an object group.
 */
ViewProviderInspectionGroup::ViewProviderInspectionGroup() = default;

ViewProviderInspectionGroup::~ViewProviderInspectionGroup() = default;

/**
 * Returns the pixmap for the opened list item.
 */
QIcon ViewProviderInspectionGroup::getIcon() const
{
    // clang-format off
    static const char * const ScanViewOpen[]={
        "16 16 10 1",
        "c c #000000",
        ". c None",
        "h c #808000",
        "# c #808080",
        "a c #ffff00",
        "r c #ff0000",
        "o c #ffff00",
        "g c #00ff00",
        "t c #00ffff",
        "b c #0000ff",
        "................",
        "...#####........",
        "..#hhhhh#.......",
        ".#hhhhhhh######.",
        ".#hhhhhhhhhhhh#c",
        ".#hhhhhhhhhhhh#c",
        ".#hhhhhhhhhhhh#c",
        "#############h#c",
        "#aaaaaaaaaa##h#c",
        "#aarroggtbbac##c",
        ".#arroggtbbaac#c",
        ".#aarroggtbbac#c",
        "..#aaaaaaaaaa#cc",
        "..#############c",
        "...ccccccccccccc",
        "................"};

    static const char * const ScanViewClosed[] = {
        "16 16 9 1",
        "c c #000000",
        ". c None",
        "# c #808080",
        "a c #ffff00",
        "r c #ff0000",
        "o c #ffff00",
        "g c #00ff00",
        "t c #00ffff",
        "b c #0000ff",
        "................",
        "...#####........",
        "..#aaaaa#.......",
        ".#aaaaaaa######.",
        ".#aaaaaaaaaaaa#c",
        ".#aarroggtbbaa#c",
        ".#aarroggtbbaa#c",
        ".#aarroggtbbaa#c",
        ".#aarroggtbbaa#c",
        ".#aarroggtbbaa#c",
        ".#aarroggtbbaa#c",
        ".#aarroggtbbaa#c",
        ".#aaaaaaaaaaaa#c",
        ".##############c",
        "..cccccccccccccc",
        "................"};
    // clang-format on

    QIcon groupIcon;
    groupIcon.addPixmap(QPixmap(ScanViewClosed), QIcon::Normal, QIcon::Off);
    groupIcon.addPixmap(QPixmap(ScanViewOpen), QIcon::Normal, QIcon::On);
    return groupIcon;
}
