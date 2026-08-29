// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 David Friedli <david[at]friedli-be.ch>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "Gui/Application.h"
#include "Gui/MDIView.h"

#include <algorithm>
#include <cstring>
#include <cmath>
#include <functional>
#include <limits>
#include <numbers>
#include <set>
#include <string_view>
#include <utility>

#include <BRepAdaptor_Curve.hxx>
#include <Precision.hxx>
#include <Standard_Failure.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>

#include <Inventor/SoPath.h>
#include <Inventor/details/SoDetail.h>

#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/nodes/SoAnnotation.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoDepthBuffer.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoMarkerSet.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/draggers/SoTranslate2Dragger.h>
#include <Inventor/engines/SoComposeMatrix.h>
#include <Inventor/engines/SoTransformVec3f.h>
#include <Inventor/engines/SoConcatenate.h>
#include <Inventor/SbViewportRegion.h>

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/GeoFeature.h>
#include <App/GeoFeatureGroupExtension.h>
#include <App/PropertyLinks.h>
#include <Base/Console.h>
#include <Base/UnitsApi.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Document.h>
#include <Gui/SoLabelNodes.h>
#include <Gui/ViewParams.h>
#include <Gui/Inventor/MarkerBitmaps.h>
#include <Gui/Selection/SelectionColors.h>
#include <Gui/Selection/SoFCUnifiedSelection.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/PropertyTopoShape.h>
#include <Mod/Measure/App/MeasureArea.h>
#include <Mod/Measure/App/MeasureDiameter.h>
#include <Mod/Measure/App/MeasureLength.h>
#include <Mod/Measure/App/MeasureRadius.h>
#include <Mod/Measure/App/Preferences.h>
#include "MeasuredGeometryHelper.h"
#include "ViewProviderMeasureBase.h"

using namespace MeasureGui;
using namespace Measure;


PROPERTY_SOURCE(MeasureGui::ViewProviderMeasureGroup, Gui::ViewProviderDocumentObjectGroup)

ViewProviderMeasureGroup::ViewProviderMeasureGroup()
{}

ViewProviderMeasureGroup::~ViewProviderMeasureGroup() = default;

QIcon ViewProviderMeasureGroup::getIcon() const
{
    return Gui::BitmapFactory().pixmap("Measurement-Group.svg");
}


namespace
{

Part::TopoShape resolveMeasurementElementShape(App::DocumentObject* object, const std::string& subname)
{
    if (!object || subname.empty()) {
        return {};
    }

    App::SubObjectT subject(object, subname.c_str());
    const auto objectPath = subject.getSubObjectList();
    const char* elementName = subject.getElementName();
    if (objectPath.empty() || !elementName || !elementName[0]) {
        return {};
    }

    App::DocumentObject* owner = objectPath.back();
    std::string localElementName = subject.getOldElementName();
    if (localElementName.empty()) {
        localElementName = elementName;
    }

    Part::TopoShape ownerShape;
    constexpr std::string_view internalPrefix("Internal");
    if (localElementName.starts_with(internalPrefix)) {
        localElementName.erase(0, internalPrefix.size());
        auto* internalShape = owner->getPropertyByName<Part::PropertyPartShape>("InternalShape");
        if (internalShape) {
            ownerShape = internalShape->getShape();
        }
    }
    else {
        ownerShape = Part::Feature::getTopoShape(owner, Part::ShapeOption::ResolveLink);
    }

    if (ownerShape.isNull()) {
        return {};
    }

    return ownerShape.getSubTopoShape(localElementName.c_str(), true);
}

bool sampleClosestEdgePoint(
    const Part::TopoShape& shape,
    const Base::Vector3d& targetPoint,
    Base::Vector3d& resultPoint
)
{
    if (shape.isNull() || shape.getShape().ShapeType() != TopAbs_EDGE) {
        return false;
    }

    try {
        BRepAdaptor_Curve curve(TopoDS::Edge(shape.getShape()));
        const double first = curve.FirstParameter();
        const double last = curve.LastParameter();
        if (!std::isfinite(first) || !std::isfinite(last)) {
            return false;
        }

        constexpr int sampleCount = 48;
        const gp_Pnt target(targetPoint.x, targetPoint.y, targetPoint.z);
        double bestDistance = std::numeric_limits<double>::max();
        gp_Pnt bestPoint;
        bool found = false;

        for (int i = 0; i <= sampleCount; ++i) {
            const double t = first + (last - first) * static_cast<double>(i) / sampleCount;
            const gp_Pnt point = curve.Value(t);
            const double distance = point.Distance(target);
            if (distance < bestDistance) {
                bestDistance = distance;
                bestPoint = point;
                found = true;
            }
        }

        if (!found) {
            return false;
        }

        resultPoint = Base::Vector3d(bestPoint.X(), bestPoint.Y(), bestPoint.Z());
        return true;
    }
    catch (const Standard_Failure&) {
        return false;
    }
}

}  // namespace


// NOLINTBEGIN
PROPERTY_SOURCE(MeasureGui::ViewProviderMeasureBase, Gui::ViewProviderDocumentObject)
// NOLINTEND

ViewProviderMeasureBase::ViewProviderMeasureBase()
{
    static const char* agroup = "Appearance";
    // NOLINTBEGIN
    ADD_PROPERTY_TYPE(
        TextColor,
        (Preferences::defaultTextColor()),
        agroup,
        App::Prop_None,
        "Color for the measurement text"
    );
    ADD_PROPERTY_TYPE(
        TextBackgroundColor,
        (Preferences::defaultTextBackgroundColor()),
        agroup,
        App::Prop_None,
        "Color for the measurement text background"
    );
    ADD_PROPERTY_TYPE(
        LineColor,
        (Preferences::defaultLineColor()),
        agroup,
        App::Prop_None,
        "Color for the measurement lines"
    );
    ADD_PROPERTY_TYPE(
        FontSize,
        (Preferences::defaultFontSize()),
        agroup,
        App::Prop_None,
        "Size of measurement text"
    );
    ADD_PROPERTY_TYPE(
        ArrowHeight,
        (Preferences::defaultArrowHeight()),
        agroup,
        App::Prop_None,
        "Height of arrow indicators"
    );
    ADD_PROPERTY_TYPE(
        ArrowRadius,
        (Preferences::defaultArrowRadius()),
        agroup,
        App::Prop_None,
        "Radius of arrow indicators"
    );
    ADD_PROPERTY_TYPE(
        LabelPosition,
        (Base::Vector3d(0, 0, 0)),
        agroup,
        App::Prop_None,
        "Position of measurement label"
    );
    // NOLINTEND

    pGlobalSeparator = new SoSeparator();
    pGlobalSeparator->ref();

    // Connect visibility of delta measurements to the ModeSwitch
    auto visibilitySwitch = new SoSwitch();
    getRoot()->insertChild(visibilitySwitch, 0);
    visibilitySwitch->addChild(pGlobalSeparator);
    visibilitySwitch->whichChild.connectFrom(&pcModeSwitch->whichChild);

    // setupAnnoSceneGraph() - sets up the annotation scene graph
    pLabel = new Gui::SoFrameLabel();
    pLabel->ref();
    pColor = new SoBaseColor();
    pColor->ref();
    pLabelTranslation = new SoTransform();
    pLabelTranslation->ref();

    auto ps = getSoPickStyle();

    // Dragger
    SoSeparator* dragSeparator = new SoSeparator();
    pDragger = new SoTranslate2Dragger();
    pDragger->ref();
    pDraggerFrame = new SoTransform();
    pDraggerFrame->ref();
    dragSeparator->addChild(pDraggerFrame);
    dragSeparator->addChild(pDragger);

    // Transform drag location by dragger local orientation and connect to labelTranslation
    auto matrixEngine = new SoComposeMatrix();
    matrixEngine->translation.connectFrom(&pDraggerFrame->translation);
    matrixEngine->rotation.connectFrom(&pDraggerFrame->rotation);
    auto transformEngine = new SoTransformVec3f();
    transformEngine->vector.connectFrom(&pDragger->translation);
    transformEngine->matrix.connectFrom(&matrixEngine->matrix);
    pLabelTranslation->translation.connectFrom(&transformEngine->point);

    auto pTextPickStyle = new SoPickStyle();
    pTextPickStyle->style = SoPickStyle::SHAPE_ON_TOP;

    pTextSeparator = new SoSeparator();
    pTextSeparator->ref();
    pTextSeparator->addChild(pTextPickStyle);
    pTextSeparator->addChild(dragSeparator);
    pTextSeparator->addChild(pLabelTranslation);
    pTextSeparator->addChild(pLabel);

    // Empty line separator which can be populated by inherited class
    pLineSeparator = new SoSeparator();
    pLineSeparator->ref();
    pLineSeparator->addChild(ps);
    pLineSeparator->addChild(getSoLineStylePrimary());
    pLineSeparator->addChild(pColor);

    // Secondary line separator
    pLineSeparatorSecondary = new SoSeparator();
    pLineSeparatorSecondary->ref();
    pLineSeparatorSecondary->addChild(ps);
    pLineSeparatorSecondary->addChild(getSoLineStyleSecondary());
    pLineSeparatorSecondary->addChild(pColor);

    pRootSeparator = new SoAnnotation();
    pRootSeparator->ref();
    pRootSeparator->addChild(pLineSeparator);
    pRootSeparator->addChild(pLineSeparatorSecondary);
    pRootSeparator->addChild(pTextSeparator);
    addDisplayMaskMode(pRootSeparator, "Base");

    pRootSeparator->touch();
    pTextSeparator->touch();
    pLineSeparator->touch();

    // Register dragger callback
    auto dragger = pDragger;

    dragger->addValueChangedCallback(draggerChangedCallback, this);
    dragger->addStartCallback(draggerStartCallback, this);
    dragger->addFinishCallback(draggerFinishCallback, this);

    // Use the label node as the transform handle
    SoSearchAction sa;
    sa.setInterest(SoSearchAction::FIRST);
    sa.setSearchingAll(true);
    sa.setNode(pLabel);
    sa.apply(pcRoot);
    SoPath* labelPath = sa.getPath();
    assert(labelPath);
    dragger->setPartAsPath("translator", labelPath);

    // Hide the dragger feedback during translation
    dragger->setPart("translatorActive", NULL);
    dragger->setPart("xAxisFeedback", NULL);
    dragger->setPart("yAxisFeedback", NULL);
    // end setupSceneGraph

    // these touches cause onChanged to run which then updates pLabel and pColor with the initial
    // values
    TextColor.touch();
    TextBackgroundColor.touch();
    FontSize.touch();
    LineColor.touch();
    fieldFontSize.setValue(FontSize.getValue());
    // Arrow properties
    ArrowHeight.touch();
    ArrowRadius.touch();
    fieldArrowHeight.setValue(ArrowHeight.getValue());
    fieldArrowRadius.setValue(ArrowRadius.getValue());
}

ViewProviderMeasureBase::~ViewProviderMeasureBase()
{
    pDragger->removeValueChangedCallback(draggerChangedCallback, this);
    pDragger->removeStartCallback(draggerStartCallback, this);
    pDragger->removeFinishCallback(draggerFinishCallback, this);
    _mVisibilityChangedConnection.disconnect();
    pGlobalSeparator->unref();
    pLabel->unref();
    pColor->unref();
    pDragger->unref();
    pDraggerFrame->unref();
    pLabelTranslation->unref();
    pTextSeparator->unref();
    pLineSeparator->unref();
    pRootSeparator->unref();
}

std::vector<std::string> ViewProviderMeasureBase::getDisplayModes() const
{
    // add modes
    std::vector<std::string> StrList;
    StrList.emplace_back("Base");
    return StrList;
}

void ViewProviderMeasureBase::setDisplayMode(const char* ModeName)
{
    if (strcmp(ModeName, "Base") == 0) {
        setDisplayMaskMode("Base");
    }
    ViewProviderDocumentObject::setDisplayMode(ModeName);
}


void ViewProviderMeasureBase::finishRestoring()
{
    // Restore dragger position from saved property
    Base::Vector3d pos = LabelPosition.getValue();
    setLabelTranslation(toSbVec3f(pos));

    if (Visibility.getValue() && isSubjectVisible()) {
        show();
    }
    ViewProviderDocumentObject::finishRestoring();
}


void ViewProviderMeasureBase::onChanged(const App::Property* prop)
{
    if (prop == &TextColor) {
        const Base::Color& color = TextColor.getValue();
        pLabel->textColor.setValue(color.r, color.g, color.b);
        updateIcon();
    }
    else if (prop == &TextBackgroundColor) {
        const Base::Color& color = TextBackgroundColor.getValue();
        pLabel->backgroundColor.setValue(color.r, color.g, color.b);
    }
    else if (prop == &LineColor) {
        const Base::Color& color = LineColor.getValue();
        pColor->rgb.setValue(color.r, color.g, color.b);
    }
    else if (prop == &FontSize) {
        pLabel->size = FontSize.getValue();
        fieldFontSize.setValue(FontSize.getValue());
    }
    else if (prop == &ArrowHeight) {
        fieldArrowHeight.setValue(ArrowHeight.getValue());
    }
    else if (prop == &ArrowRadius) {
        fieldArrowRadius.setValue(ArrowRadius.getValue());
    }

    ViewProviderDocumentObject::onChanged(prop);
}

void ViewProviderMeasureBase::draggerChangedCallback(void* data, SoDragger*)
{
    auto me = static_cast<ViewProviderMeasureBase*>(data);
    me->onLabelMoved();
}

void ViewProviderMeasureBase::draggerStartCallback(void* data, SoDragger*)
{
    auto me = static_cast<ViewProviderMeasureBase*>(data);
    me->onLabelMoveStart();
}

void ViewProviderMeasureBase::draggerFinishCallback(void* data, SoDragger*)
{
    auto me = static_cast<ViewProviderMeasureBase*>(data);
    me->onLabelMoveFinish();
}

void ViewProviderMeasureBase::onLabelMoveStart()
{
    Gui::View3DInventor* view = nullptr;
    try {
        view = dynamic_cast<Gui::View3DInventor*>(this->getActiveView());
    }
    catch (const Base::RuntimeError&) {
        return;
    }
    if (!view) {
        return;
    }

    auto* cam = view->getViewer()->getSoRenderManager()->getCamera();
    if (!cam) {
        return;
    }

    pDraggerFrame->rotation.setValue(cam->orientation.getValue());
}

void ViewProviderMeasureBase::onLabelMoveFinish()
{
    SbVec3f currentLabelPos = pLabelTranslation->translation.getValue();
    pDraggerFrame->translation.setValue(currentLabelPos);
    pDragger->translation.setValue(SbVec3f(0.0f, 0.0f, 0.0f));
    LabelPosition.setValue(Base::Vector3d(currentLabelPos[0], currentLabelPos[1], currentLabelPos[2]));
}

void ViewProviderMeasureBase::setLabelValue(const Base::Quantity& value)
{
    pLabel->string.setValue(value.getUserString().c_str());
}

void ViewProviderMeasureBase::setLabelValue(const std::string& value)
{
    const auto userString = Base::UnitsApi::toUnicodeSuperscript(value);
    const auto lines = QString::fromStdString(userString).split(QStringLiteral("\n"));

    int i = 0;
    for (auto& it : lines) {
        pLabel->string.set1Value(i, it.toUtf8().constData());
        i++;
    }
}

void ViewProviderMeasureBase::setLabelTranslation(const SbVec3f& position)
{
    pDraggerFrame->translation.setValue(position);
    pDragger->translation.setValue(SbVec3f(0.0f, 0.0f, 0.0f));
}


SoPickStyle* ViewProviderMeasureBase::getSoPickStyle()
{
    auto ps = new SoPickStyle();
    ps->style = SoPickStyle::UNPICKABLE;
    return ps;
}

SoDrawStyle* ViewProviderMeasureBase::getSoLineStylePrimary()
{
    auto style = new SoDrawStyle();
    style->lineWidth = 2.0f;
    return style;
}

SoDrawStyle* ViewProviderMeasureBase::getSoLineStyleSecondary()
{
    auto style = new SoDrawStyle();
    style->lineWidth = 1.0f;
    return style;
}

SoSeparator* ViewProviderMeasureBase::getSoSeparatorText()
{
    return pTextSeparator;
}


void ViewProviderMeasureBase::positionAnno(const Measure::MeasureBase* measureObject)
{
    (void)measureObject;
}


void ViewProviderMeasureBase::updateIcon()
{
    // This assumes the icons main color is black

    Gui::ColorMap colorMap {
        {0x000000, TextColor.getValue().getPackedRGB() >> 8},
    };
    pLabel->setIcon(Gui::BitmapFactory().pixmapFromSvg(sPixmap, QSize(20, 20), colorMap));
}

void ViewProviderMeasureBase::attach(App::DocumentObject* pcObj)
{
    ViewProviderDocumentObject::attach(pcObj);
    updateIcon();
}


//! handle changes to the feature's properties
void ViewProviderMeasureBase::updateData(const App::Property* prop)
{
    bool doUpdate = false;

    auto obj = getMeasureObject();
    if (!obj) {
        return;
    }

    if (strcmp(prop->getName(), "Label") == 0 || prop == &obj->DisplayUnit) {
        doUpdate = true;
    }

    // Check if one of the input properties has been changed
    auto inputProps = obj->getInputProps();
    if (std::ranges::find(inputProps, std::string(prop->getName())) != inputProps.end()) {
        doUpdate = true;

        // Add connections to be notified when the measured objects are changed
        connectToSubject(obj->getSubject());
    }

    // Check if the result prop has been changed
    auto resultProp = obj->getResultProp();
    if (resultProp && prop == resultProp) {
        doUpdate = true;
    }

    if (doUpdate) {
        redrawAnnotation();

        // Update label
        std::string userLabel(obj->Label.getValue());
        std::string name = userLabel.substr(0, userLabel.find(":"));
        obj->Label.setValue((name + ": ") + obj->getResultString());
    }

    ViewProviderDocumentObject::updateData(prop);
}


// TODO: should this be pure virtual?
void ViewProviderMeasureBase::redrawAnnotation()
{
    // Base::Console().message("VPMB::redrawAnnotation()\n");
}

//! connect to the subject to receive visibility updates
void ViewProviderMeasureBase::connectToSubject(App::DocumentObject* subject)
{
    if (!subject) {
        return;
    }

    // disconnect any existing connection
    if (_mVisibilityChangedConnection.connected()) {
        _mVisibilityChangedConnection.disconnect();
    }

    App::Document* document = subject->getDocument();
    if (!document) {
        return;
    }

    _mVisibilityChangedConnection = document->signalChangedObject.connect(
        [this, subject](const App::DocumentObject& obj, const App::Property& prop) {
            if (&obj == subject) {
                onSubjectVisibilityChanged(obj, prop);
            }
        }
    );
}

//! connect to the subject to receive visibility updates
void ViewProviderMeasureBase::connectToSubject(std::vector<App::DocumentObject*> subject)
{
    if (subject.empty()) {
        return;
    }

    // TODO: should we connect to all the subject objects when there is >1?
    auto proxy = subject.front();
    connectToSubject(proxy);
}


//! retrieve the feature
Measure::MeasureBase* ViewProviderMeasureBase::getMeasureObject()
{
    // Note: Cast to MeasurePropertyBase once we use it to provide the needed values e.g.
    // basePosition textPosition etc.
    auto feature = dynamic_cast<Measure::MeasureBase*>(pcObject);
    if (!feature) {
        throw Base::RuntimeError("Feature not found for ViewProviderMeasureBase");
    }
    return feature;
}


//! calculate a good direction from the elements being measured to the annotation text based on the
//! layout of the elements and relationship with the cardinal axes and the view direction.
//! elementDirection is expected to be a normalized vector. an example of an elementDirection would
//! be the vector from the start of a line to the end.
Base::Vector3d ViewProviderMeasureBase::getTextDirection(Base::Vector3d elementDirection, double tolerance)
{
    // TODO: this can fail if the active view is not a 3d view (spreadsheet, techdraw page) and
    // something causes a measure to try to update we need to search through the mdi views for a 3d
    // view and take the direction from it (or decide that if the active view is not 3d, assume we
    // are looking from the front).
    Base::Vector3d viewDirection;
    Base::Vector3d upDirection;

    Gui::View3DInventor* view = nullptr;
    try {
        view = dynamic_cast<Gui::View3DInventor*>(this->getActiveView());
    }
    catch (const Base::RuntimeError&) {
        Base::Console().log("ViewProviderMeasureBase::getTextDirection: Could not get active view\n");
    }

    if (view) {
        Gui::View3DInventorViewer* viewer = view->getViewer();
        viewDirection = toVector3d(viewer->getViewDirection()).Normalize();
        upDirection = toVector3d(viewer->getUpDirection()).Normalize();
        // Measure doesn't work with this kind of active view.  Might be dependency graph, might be
        // TechDraw, or ????
        // throw Base::RuntimeError("Measure doesn't work with this kind of active view.");
    }
    else {
        viewDirection = Base::Vector3d(0.0, 1.0, 0.0);
        upDirection = Base::Vector3d(0.0, 0.0, 1.0);
    }

    Base::Vector3d textDirection = elementDirection.Cross(viewDirection);
    if (textDirection.Length() < tolerance) {
        // either elementDirection and viewDirection are parallel or one of them is null.
        textDirection = elementDirection.Cross(upDirection);
    }

    return textDirection.Normalize();
}


//! true if the subject of this measurement is visible.  For Measures that have multiple object
//! subject, all of the subjects must be visible.
bool ViewProviderMeasureBase::isSubjectVisible()
{
    Gui::Document* guiDoc = nullptr;
    try {
        guiDoc = this->getDocument();
    }
    catch (const Base::RuntimeError&) {
        Base::Console().log("ViewProviderMeasureBase::isSubjectVisible: Could not get document\n");
        return false;
    }

    // we need these things to proceed
    if (!getMeasureObject() || !guiDoc) {
        return false;
    }

    // Show the measurement if it doesn't track any subjects
    if (getMeasureObject()->getSubject().empty()) {
        return true;
    }

    for (auto& obj : getMeasureObject()->getSubject()) {
        Gui::ViewProvider* vp = guiDoc->getViewProvider(obj);
        if (!vp || !vp->isVisible()) {
            return false;
        }
    }

    // all of the subject objects are visible
    return true;
}


//! gets called when the subject object issues a signalChanged (ie a property change).  We are only
//! interested in the subject's Visibility property
void ViewProviderMeasureBase::onSubjectVisibilityChanged(
    const App::DocumentObject& docObj,
    const App::Property& prop
)
{
    if (docObj.isRemoving()) {
        return;
    }


    std::string propName = prop.getName();
    if (propName == "Visibility") {
        if (!docObj.Visibility.getValue()) {
            // show ourselves only if subject is visible
            setVisible(false);
        }
        else {
            // here, we don't know if we should be visible or not, so we have to check the whole
            // subject
            setVisible(isSubjectVisible() && Visibility.getValue());
        }
    }
}


float ViewProviderMeasureBase::getViewScale()
{
    float scale = 1.0;

    Gui::View3DInventor* view = dynamic_cast<Gui::View3DInventor*>(this->getActiveView());
    if (!view) {
        Base::Console().log("ViewProviderMeasureBase::getViewScale: Could not get active view\n");
        return scale;
    }
    Gui::View3DInventorViewer* viewer = view->getViewer();

    SoCamera* const camera = viewer->getSoRenderManager()->getCamera();
    if (!camera) {
        return false;
    }

    SbViewVolume volume(camera->getViewVolume());
    SbVec3f center(volume.getSightPoint(camera->focalDistance.getValue()));
    scale = volume.getWorldToScreenScale(center, 1.0);
    return scale;
}


// NOLINTBEGIN
PROPERTY_SOURCE(MeasureGui::ViewProviderMeasure, MeasureGui::ViewProviderMeasureBase)
// NOLINTEND

//! the general purpose view provider.  handles area, length, etc - any measure without a
//! specialized VP
ViewProviderMeasure::ViewProviderMeasure()
{
    sPixmap = "umf-measurement";

    // setupSceneGraph for leader?
    const size_t lineCount(3);

    // indexes used to create the edges
    // this makes a line from verts[0] to verts[1]
    static const int32_t lines[lineCount] = {0, 1, -1};

    pCoords = new SoCoordinate3();
    pCoords->ref();
    pLeaderLineStartOffset = new SoSFVec3f();
    pLeaderLineStartOffset->setValue(0.0f, 0.0f, 0.0f);

    // Combine coordinates from the leader start offset and label translation. The first point can
    // be moved to a visible boundary point (for example a spline edge) while the label keeps its
    // current world-space position.
    auto engineCat = new SoConcatenate(SoMFVec3f::getClassTypeId());
    engineCat->input[0]->connectFrom(pLeaderLineStartOffset);
    engineCat->input[1]->connectFrom(&pLabelTranslation->translation);
    pCoords->point.setNum(engineCat->output->getNumConnections());
    pCoords->point.connectFrom(engineCat->output);

    pLines = new SoIndexedLineSet();
    pLines->ref();
    pLines->coordIndex.setNum(lineCount);
    pLines->coordIndex.setValues(0, lineCount, lines);

    auto lineSep = pLineSeparator;
    lineSep->addChild(pCoords);
    lineSep->addChild(pLines);
    auto points = new SoMarkerSet();
    points->markerIndex = Gui::Inventor::MarkerBitmaps::getMarkerIndex(
        "CROSS",
        Gui::ViewParams::instance()->getMarkerSize()
    );
    points->numPoints = 1;
    lineSep->addChild(points);
}

ViewProviderMeasure::~ViewProviderMeasure()
{
    _mDocumentRecomputedConnection.disconnect();
    detachMeasuredGeometryHighlightFromViews();
    clearMeasuredGeometryHighlight();
    if (pMeasuredGeometryVisibilitySwitch) {
        pMeasuredGeometryVisibilitySwitch->removeAllChildren();
        pMeasuredGeometryVisibilitySwitch->unref();
        pMeasuredGeometryVisibilitySwitch = nullptr;
    }
    if (pMeasuredGeometryHighlight) {
        pMeasuredGeometryHighlight->unref();
    }
    delete pLeaderLineStartOffset;
    pCoords->unref();
    pLines->unref();
}

void ViewProviderMeasure::attach(App::DocumentObject* pcObj)
{
    ViewProviderMeasureBase::attach(pcObj);

    _mDocumentRecomputedConnection.disconnect();
    if (!pcObj || !pcObj->getDocument()) {
        return;
    }

    // Rebuild persistent measured-geometry overlays only after the document is fully stable.
    // Part Design can change the Body tip and hide predecessor features while a recompute is still
    // being finalized. Rebuilding on signalRecomputed is too early in that case and can leave the
    // overlay attached to scene paths that disappear a moment later.
    _mDocumentRecomputedConnection = pcObj->getDocument()->signalBecameStable.connect(
        [this](const App::Document&) {
            if (!pcObject || pcObject->isRemoving()) {
                return;
            }

            attachMeasuredGeometryHighlightToViews();
            updateMeasuredGeometryHighlight();
            ViewProviderDocumentObject::updateView();
        }
    );
}

void ViewProviderMeasure::beforeDelete()
{
    _mDocumentRecomputedConnection.disconnect();
    detachMeasuredGeometryHighlightFromViews();
    ViewProviderMeasureBase::beforeDelete();
}

void ViewProviderMeasure::positionAnno(const Measure::MeasureBase* measureObject)
{
    (void)measureObject;

    // Initialize the text position
    Base::Vector3d textPos = getTextPosition();
    auto srcVec = SbVec3f(textPos.x, textPos.y, textPos.z);

    setLabelTranslation(srcVec);

    // Persist the initial COM label offset; its Python object may not trigger another GUI update
    // while a document is being restored.
    if (dynamic_cast<ViewProviderMeasureCOM*>(this)) {
        LabelPosition.setValue(textPos);
    }

    updateView();
}

void ViewProviderMeasure::onChanged(const App::Property* prop)
{
    if (pcObject == nullptr) {
        return;
    }

    ViewProviderMeasureBase::onChanged(prop);
}


//! repaint the annotation
void ViewProviderMeasure::redrawAnnotation()
{
    // point on element
    Base::Vector3d basePos = getBasePosition();
    pcTransform->translation.setValue(SbVec3f(basePos.x, basePos.y, basePos.z));

    const SbVec3f labelOffset = pLabelTranslation->translation.getValue();
    const Base::Vector3d labelWorldPosition(
        basePos.x + labelOffset[0],
        basePos.y + labelOffset[1],
        basePos.z + labelOffset[2]
    );
    pLeaderLineStartOffset->setValue(getLeaderLineStartOffset(basePos, labelWorldPosition));

    setLabelValue(getMeasureObject()->getResultString());

    ViewProviderMeasureBase::redrawAnnotation();
    updateMeasuredGeometryHighlight();
    ViewProviderDocumentObject::updateView();
}

void ViewProviderMeasure::finishRestoring()
{
    ViewProviderMeasureBase::finishRestoring();

    if (dynamic_cast<ViewProviderMeasureCOM*>(this)) {
        // Older COM measurements may restore with the default zero offset. Recreate the display
        // offset without rewriting the restored property just by opening the document.
        const Base::Vector3d savedPos = LabelPosition.getValue();
        if (savedPos.Length() < 1.0e-12) {
            const Base::Vector3d textPos = getTextPosition();
            if (textPos.Length() >= 1.0e-12) {
                setLabelTranslation(toSbVec3f(textPos));
            }
        }
    }

    // Rebuild the saved measurement annotation and persistent geometry highlight after restore.
    redrawAnnotation();
}


Base::Vector3d ViewProviderMeasure::getBasePosition()
{
    auto measureObject = getMeasureObject();
    Base::Placement placement = measureObject->getPlacement();
    return placement.getPosition();
}

Base::Vector3d ViewProviderMeasure::getTextPosition()
{
    // Return the initial position relative to the base position
    auto basePoint = getBasePosition();

    Gui::View3DInventor* view = dynamic_cast<Gui::View3DInventor*>(this->getActiveView());
    if (!view) {
        Base::Console().log("ViewProviderMeasureBase::getTextPosition: Could not get active view\n");
        return Base::Vector3d();
    }

    Gui::View3DInventorViewer* viewer = view->getViewer();

    // Convert to screenspace, offset and convert back to world space
    SbVec2s screenPos = viewer->getPointOnViewport(SbVec3f(basePoint.x, basePoint.y, basePoint.z));
    SbVec3f vec = viewer->getPointOnFocalPlane(screenPos + SbVec2s(30.0, 30.0));
    Base::Vector3d textPos(vec[0], vec[1], vec[2]);

    return textPos - basePoint;
}

SbVec3f ViewProviderMeasure::getLeaderLineStartOffset(
    const Base::Vector3d& /*basePosition*/,
    const Base::Vector3d& /*labelWorldPosition*/
) const
{
    return SbVec3f(0.0f, 0.0f, 0.0f);
}

SbVec3f ViewProviderMeasureArea::getLeaderLineStartOffset(
    const Base::Vector3d& basePosition,
    const Base::Vector3d& labelWorldPosition
) const
{
    auto* areaMeasure = dynamic_cast<Measure::MeasureArea*>(pcObject);
    if (!areaMeasure) {
        return ViewProviderMeasure::getLeaderLineStartOffset(basePosition, labelWorldPosition);
    }

    const auto& objects = areaMeasure->Elements.getValues();
    const auto subnames = areaMeasure->Elements.getSubValues(true);
    if (objects.empty() || subnames.empty() || objects.size() != subnames.size()) {
        return ViewProviderMeasure::getLeaderLineStartOffset(basePosition, labelWorldPosition);
    }

    Base::Vector3d bestPoint;
    double bestDistance = std::numeric_limits<double>::max();
    bool found = false;
    bool hasInternalSketchFace = false;

    for (std::size_t i = 0; i < objects.size(); ++i) {
        App::SubObjectT subject(objects[i], subnames[i].c_str());
        std::string elementName = subject.getOldElementName();
        if (elementName.empty()) {
            if (const char* rawElementName = subject.getElementName()) {
                elementName = rawElementName;
            }
        }
        if (!elementName.starts_with("Internal")) {
            continue;
        }

        hasInternalSketchFace = true;
        const auto boundarySubnames
            = MeasuredGeometryHelper::getBoundarySubnames(objects[i], subnames[i]);
        for (const auto& boundarySubname : boundarySubnames) {
            const Part::TopoShape boundaryShape
                = resolveMeasurementElementShape(objects[i], boundarySubname);
            Base::Vector3d candidate;
            if (!sampleClosestEdgePoint(boundaryShape, labelWorldPosition, candidate)) {
                continue;
            }

            const double distance = (candidate - labelWorldPosition).Length();
            if (distance < bestDistance) {
                bestDistance = distance;
                bestPoint = candidate;
                found = true;
            }
        }
    }

    if (!hasInternalSketchFace || !found) {
        return ViewProviderMeasure::getLeaderLineStartOffset(basePosition, labelWorldPosition);
    }

    const Base::Vector3d offset = bestPoint - basePosition;
    return SbVec3f(offset.x, offset.y, offset.z);
}

//! called by the system when it is time to display this measure
void ViewProviderMeasureBase::show()
{
    if (isSubjectVisible()) {
        // only show the annotation if subject is visible.
        // this avoids disconnected annotations floating in space.
        ViewProviderDocumentObject::show();
    }
}


PROPERTY_SOURCE(MeasureGui::ViewProviderMeasureArea, MeasureGui::ViewProviderMeasure)
PROPERTY_SOURCE(MeasureGui::ViewProviderMeasureLength, MeasureGui::ViewProviderMeasure)

namespace
{

bool buildMeasuredGeometryHighlightPath(
    App::DocumentObject* sourceObject,
    const std::string& subName,
    SoTempPath& sourcePath,
    SoDetail*& detail
)
{
    detail = nullptr;
    if (!sourceObject || sourceObject->isRemoving() || !sourceObject->isAttachedToDocument()
        || !Gui::Application::Instance) {
        return false;
    }

    auto* sourceViewProvider = freecad_cast<Gui::ViewProviderDocumentObject*>(
        Gui::Application::Instance->getViewProvider(sourceObject)
    );
    if (!sourceViewProvider || !sourceViewProvider->isShow()) {
        return false;
    }

    // If the stored reference points through a concrete feature (for example Body.Pad.Edge3),
    // only replay that ViewProvider path while the referenced feature is actually visible.
    // After a newer Part Design feature becomes the Body tip, the predecessor is hidden even
    // though the Body itself stays visible. In that case the exact saved edge is drawn by the
    // independent fallback below instead of translating it to a possibly different successor edge.
    if (!subName.empty()) {
        App::SubObjectT savedReference(sourceObject, subName.c_str());
        const auto savedPath = savedReference.getSubObjectList();
        if (!savedPath.empty() && savedPath.back() != sourceObject) {
            auto* ownerViewProvider = freecad_cast<Gui::ViewProviderDocumentObject*>(
                Gui::Application::Instance->getViewProvider(savedPath.back())
            );
            if (!ownerViewProvider || !ownerViewProvider->isShow()) {
                return false;
            }
        }
    }

    // Recreate the parent-group path used to display the source ViewProvider on top.
    std::vector<Gui::ViewProviderDocumentObject*> groups;
    auto* groupViewProvider = sourceViewProvider;
    std::set<Gui::ViewProvider*> visited;

    for (auto* childViewProvider = sourceViewProvider;; childViewProvider = groupViewProvider) {
        auto* group = App::GeoFeatureGroupExtension::getGroupOfObject(childViewProvider->getObject());
        if (!group || !group->isAttachedToDocument()) {
            break;
        }

        groupViewProvider = freecad_cast<Gui::ViewProviderDocumentObject*>(
            Gui::Application::Instance->getViewProvider(group)
        );
        if (!groupViewProvider) {
            break;
        }

        if (!visited.insert(childViewProvider).second) {
            break;
        }

        auto* childRoot = groupViewProvider->getChildRoot();
        auto* modeSwitch = groupViewProvider->getModeSwitch();
        const int activeMode = modeSwitch->whichChild.getValue();
        if (activeMode < 0 || activeMode >= modeSwitch->getNumChildren()
            || modeSwitch->getChild(activeMode) != childRoot
            || childRoot->findChild(childViewProvider->getRoot()) < 0) {
            return false;
        }

        groups.push_back(groupViewProvider);
    }

    for (auto it = groups.rbegin(); it != groups.rend(); ++it) {
        auto* group = *it;
        sourcePath.append(group->getRoot());
        sourcePath.append(group->getModeSwitch());
        sourcePath.append(group->getChildRoot());
    }

    // Whole-object references have no SoDetail; the scene path is sufficient.
    const char* detailSubName = subName.empty() ? nullptr : subName.c_str();
    return sourceViewProvider->getDetailPath(detailSubName, &sourcePath, true, detail)
        && sourcePath.getLength() > 0;
}

bool addIndependentMeasuredEdgeHighlight(
    Gui::SoFCSelectionRoot* selectionRoot,
    const TopoDS_Edge& locatedEdge,
    const SbColor& color,
    bool renderOnTop = false
)
{
    if (!selectionRoot || locatedEdge.IsNull()) {
        return false;
    }

    try {
        BRepAdaptor_Curve curve(locatedEdge);
        const double first = curve.FirstParameter();
        const double last = curve.LastParameter();
        if (!std::isfinite(first) || !std::isfinite(last)) {
            return false;
        }

        // Use enough segments for large circles to remain visually as smooth as the normal
        // preselection rendering used by measured splines.
        constexpr int segmentCount = 512;
        std::vector<SbVec3f> points;
        points.reserve(segmentCount + 1);
        for (int i = 0; i <= segmentCount; ++i) {
            const double t = first + (last - first) * static_cast<double>(i) / segmentCount;
            const gp_Pnt point = curve.Value(t);
            points.emplace_back(
                static_cast<float>(point.X()),
                static_cast<float>(point.Y()),
                static_cast<float>(point.Z())
            );
        }
        if (points.size() < 2) {
            return false;
        }

        auto* separator = new SoSeparator();
        auto* pickStyle = new SoPickStyle();
        pickStyle->style = SoPickStyle::UNPICKABLE;
        separator->addChild(pickStyle);

        if (!renderOnTop) {
            // Keep normal depth testing so measured edges behind a solid stay hidden.
            auto* depth = new SoDepthBuffer();
            depth->test = true;
            depth->write = false;
            depth->function = SoDepthBuffer::LEQUAL;
            depth->range.setValue(SbVec2f(0.0f, 0.99998f));
            separator->addChild(depth);
        }

        auto* baseColor = new SoBaseColor();
        baseColor->rgb.setValue(color);
        separator->addChild(baseColor);

        auto* drawStyle = new SoDrawStyle();
        // The original Sketcher outline is anti-aliased and remains underneath this overlay.
        // A one-pixel wider circular highlight fully covers its edge pixels instead of allowing
        // white fragments to remain visible around the blue curve.
        drawStyle->lineWidth = renderOnTop ? 3.0f : 2.0f;
        separator->addChild(drawStyle);

        auto* coordinates = new SoCoordinate3();
        coordinates->point.setValues(0, static_cast<int>(points.size()), points.data());
        separator->addChild(coordinates);

        std::vector<int32_t> indices;
        indices.reserve(points.size() + 1);
        for (std::size_t i = 0; i < points.size(); ++i) {
            indices.push_back(static_cast<int32_t>(i));
        }
        indices.push_back(-1);

        auto* lines = new SoIndexedLineSet();
        lines->coordIndex.setValues(0, static_cast<int>(indices.size()), indices.data());
        separator->addChild(lines);

        if (renderOnTop) {
            // Saved spline highlights are replayed as delayed annotations with depth testing
            // disabled. Do the same for circular measurements so the original white Sketcher
            // outline cannot alternate with the blue line.
            auto* annotation = new SoAnnotation();
            annotation->addChild(separator);
            selectionRoot->addChild(annotation);
        }
        else {
            selectionRoot->addChild(separator);
        }
        return true;
    }
    catch (const Standard_Failure&) {
        return false;
    }
}

bool addIndependentMeasuredEdgeHighlight(
    Gui::SoFCSelectionRoot* selectionRoot,
    App::DocumentObject* sourceObject,
    const std::string& subName,
    const SbColor& color
)
{
    if (!selectionRoot || !sourceObject || subName.empty()) {
        return false;
    }

    // Resolve the stored element from the object that actually owns it.  Do not reduce the
    // reference to its old EdgeN name: after a later Part Design feature is created, the old
    // numeric index can name a different circle (for example the bottom edge of a cylinder).
    App::SubObjectT subject(sourceObject, subName.c_str());
    const auto objectPath = subject.getSubObjectList();
    App::DocumentObject* owner = objectPath.empty() ? nullptr : objectPath.back();
    const char* mappedElementName = subject.getElementName();

    TopoDS_Shape locatedEdge;
    if (owner && mappedElementName && mappedElementName[0]) {
        std::string elementName(mappedElementName);
        constexpr std::string_view internalPrefix("Internal");

        Part::TopoShape ownerShape;
        if (elementName.starts_with(internalPrefix)) {
            elementName.erase(0, internalPrefix.size());
            if (auto* internalShape
                = owner->getPropertyByName<Part::PropertyPartShape>("InternalShape")) {
                ownerShape = internalShape->getShape();
            }
        }
        else if (auto* partFeature = dynamic_cast<Part::Feature*>(owner)) {
            ownerShape = partFeature->Shape.getShape();
        }
        else {
            ownerShape = Part::Feature::getTopoShape(owner, Part::ShapeOption::ResolveLink);
        }

        if (!ownerShape.isNull()) {
            ownerShape.setPlacement(
                App::GeoFeature::getGlobalPlacement(owner, subject.getObject(), subject.getSubName())
            );
            Part::TopoShape edgeShape = ownerShape.getSubTopoShape(elementName.c_str(), true);
            if (!edgeShape.isNull() && edgeShape.getShape().ShapeType() == TopAbs_EDGE) {
                locatedEdge = edgeShape.getShape();
            }
        }
    }

    // Keep the standard TNP-aware lookup only as a fallback for link-like references that are not
    // directly owned by a Part feature.
    if (locatedEdge.IsNull()) {
        try {
            locatedEdge = Part::Feature::getShape(
                sourceObject,
                Part::ShapeOption::NeedSubElement | Part::ShapeOption::ResolveLink
                    | Part::ShapeOption::Transform,
                subName.c_str()
            );
        }
        catch (const Standard_Failure&) {
            locatedEdge.Nullify();
        }
    }

    if (locatedEdge.IsNull() || locatedEdge.ShapeType() != TopAbs_EDGE) {
        return false;
    }

    return addIndependentMeasuredEdgeHighlight(selectionRoot, TopoDS::Edge(locatedEdge), color);
}

bool replayMeasuredGeometryHighlight(
    Gui::SoFCSelectionRoot* selectionRoot,
    App::DocumentObject* sourceObject,
    const std::string& subName,
    const SbColor& color,
    bool selectOnlyThisElement
);

bool edgesHaveSameGeometry(const TopoDS_Edge& measuredEdge, const TopoDS_Edge& candidateEdge)
{
    try {
        const BRepAdaptor_Curve measuredCurve(measuredEdge);
        const BRepAdaptor_Curve candidateCurve(candidateEdge);
        if (measuredCurve.GetType() != candidateCurve.GetType()) {
            return false;
        }

        const double measuredFirst = measuredCurve.FirstParameter();
        const double measuredLast = measuredCurve.LastParameter();
        const double candidateFirst = candidateCurve.FirstParameter();
        const double candidateLast = candidateCurve.LastParameter();
        if (!std::isfinite(measuredFirst) || !std::isfinite(measuredLast)
            || !std::isfinite(candidateFirst) || !std::isfinite(candidateLast)) {
            return false;
        }

        double scale = 1.0;
        if (measuredCurve.GetType() == GeomAbs_Circle) {
            const auto measuredCircle = measuredCurve.Circle();
            const auto candidateCircle = candidateCurve.Circle();
            scale = std::max(scale, measuredCircle.Radius());
            const double tolerance = std::max(Precision::Confusion(), scale * 1.0e-7);
            if (std::abs(candidateCircle.Radius() - measuredCircle.Radius()) > tolerance
                || candidateCircle.Location().Distance(measuredCircle.Location()) > tolerance
                || !candidateCircle.Axis().Direction().IsParallel(
                    measuredCircle.Axis().Direction(),
                    Precision::Angular()
                )) {
                return false;
            }

            // A closed circular edge is fully defined by its plane, center and radius. Its OCCT
            // parameter origin may legitimately differ after a Part Design operation.
            constexpr double fullCircle = 2.0 * std::numbers::pi;
            const bool measuredIsFullCircle
                = std::abs(std::abs(measuredLast - measuredFirst) - fullCircle) < 1.0e-7;
            const bool candidateIsFullCircle
                = std::abs(std::abs(candidateLast - candidateFirst) - fullCircle) < 1.0e-7;
            if (measuredIsFullCircle != candidateIsFullCircle) {
                return false;
            }
            if (measuredIsFullCircle) {
                return true;
            }
        }

        const gp_Pnt measuredStart = measuredCurve.Value(measuredFirst);
        const gp_Pnt measuredEnd = measuredCurve.Value(measuredLast);
        scale = std::max(scale, measuredStart.Distance(measuredEnd));
        const double tolerance = std::max(Precision::Confusion() * 10.0, scale * 1.0e-7);

        constexpr int sampleCount = 12;
        const auto samplesMatch = [&](bool reverseCandidate) {
            for (int i = 0; i <= sampleCount; ++i) {
                const double fraction = static_cast<double>(i) / sampleCount;
                const double measuredParameter = measuredFirst
                    + (measuredLast - measuredFirst) * fraction;
                const double candidateFraction = reverseCandidate ? 1.0 - fraction : fraction;
                const double candidateParameter = candidateFirst
                    + (candidateLast - candidateFirst) * candidateFraction;
                if (measuredCurve.Value(measuredParameter)
                        .Distance(candidateCurve.Value(candidateParameter))
                    > tolerance) {
                    return false;
                }
            }
            return true;
        };

        return samplesMatch(false) || samplesMatch(true);
    }
    catch (const Standard_Failure&) {
        return false;
    }
}

bool addMeasuredEdgeHighlights(
    Gui::SoFCSelectionRoot* selectionRoot,
    App::DocumentObject* sourceObject,
    const std::string& subName,
    const SbColor& color,
    bool circularOnly = false
)
{
    if (!selectionRoot || !sourceObject || subName.empty()) {
        return false;
    }

    // Resolve the original measured reference exactly as the measurement type handler does. This
    // avoids relying on a generated EdgeN/InternalEdgeN name, which may not exist for the filled
    // face used to select a closed Sketcher circle.
    TopoDS_Shape measuredShape;
    try {
        measuredShape = Part::Feature::getShape(
            sourceObject,
            Part::ShapeOption::NeedSubElement | Part::ShapeOption::ResolveLink
                | Part::ShapeOption::Transform,
            subName.c_str()
        );
    }
    catch (const Standard_Failure&) {
        measuredShape.Nullify();
    }

    if (measuredShape.IsNull()) {
        return false;
    }

    std::vector<TopoDS_Edge> measuredEdges;
    const auto appendMeasuredEdge = [&](const TopoDS_Edge& edge) {
        if (circularOnly) {
            try {
                if (BRepAdaptor_Curve(edge).GetType() != GeomAbs_Circle) {
                    return;
                }
            }
            catch (const Standard_Failure&) {
                return;
            }
        }

        const bool duplicate = std::ranges::any_of(measuredEdges, [&](const TopoDS_Edge& existing) {
            return existing.IsSame(edge);
        });
        if (!duplicate) {
            measuredEdges.push_back(edge);
        }
    };

    if (measuredShape.ShapeType() == TopAbs_EDGE) {
        appendMeasuredEdge(TopoDS::Edge(measuredShape));
    }
    else {
        for (TopExp_Explorer edges(measuredShape, TopAbs_EDGE); edges.More(); edges.Next()) {
            appendMeasuredEdge(TopoDS::Edge(edges.Current()));
        }
    }

    if (measuredEdges.empty()) {
        return false;
    }

    std::set<std::size_t> remainingEdges;
    for (std::size_t index = 0; index < measuredEdges.size(); ++index) {
        remainingEdges.insert(index);
    }

    const auto replayMatchingEdges = [&](App::DocumentObject* displayedObject,
                                         const Part::TopoShape& displayedShape,
                                         const std::string& subNamePrefix) {
        if (!displayedObject || displayedShape.isNull()) {
            return false;
        }

        std::set<int> usedCandidateEdges;
        std::vector<std::size_t> matchedEdges;
        for (const std::size_t measuredIndex : remainingEdges) {
            for (TopExp_Explorer edges(displayedShape.getShape(), TopAbs_EDGE); edges.More();
                 edges.Next()) {
                const auto candidate = TopoDS::Edge(edges.Current());
                const int edgeIndex = displayedShape.findShape(candidate);
                if (edgeIndex <= 0 || usedCandidateEdges.contains(edgeIndex)
                    || !edgesHaveSameGeometry(measuredEdges[measuredIndex], candidate)) {
                    continue;
                }

                if (replayMeasuredGeometryHighlight(
                        selectionRoot,
                        displayedObject,
                        subNamePrefix + "Edge" + std::to_string(edgeIndex),
                        color,
                        true
                    )) {
                    usedCandidateEdges.insert(edgeIndex);
                    matchedEdges.push_back(measuredIndex);
                }
                break;
            }
        }

        for (const std::size_t matchedIndex : matchedEdges) {
            remainingEdges.erase(matchedIndex);
        }
        return !matchedEdges.empty();
    };

    // A Sketcher circle can be selected through its generated InternalFace, while the white
    // outline visible in the 3D view belongs to the corresponding EdgeN in the normal Shape.
    // First try to replay that native edge while the Sketch itself is still visible.
    App::SubObjectT subject(sourceObject, subName.c_str());
    const auto objectPath = subject.getSubObjectList();
    App::DocumentObject* owner = objectPath.empty() ? nullptr : objectPath.back();
    if (owner) {
        Part::TopoShape visibleShape
            = Part::Feature::getTopoShape(owner, Part::ShapeOption::ResolveLink);
        if (!visibleShape.isNull()) {
            const Base::Placement globalPlacement = App::GeoFeature::getGlobalPlacement(
                owner,
                subject.getObject(),
                subject.getSubName()
            );
            visibleShape.setPlacement(globalPlacement);
            replayMatchingEdges(sourceObject, visibleShape, subject.getSubNameNoElement());
            if (remainingEdges.empty()) {
                return true;
            }
        }
    }

    // Creating a Pad hides the Sketch and replaces its displayed outline with a new edge in the
    // current Body result. Follow the dependency chain and replay the matching edge of the first
    // visible successor. Its ViewProvider supplies the current cylinder tessellation, preventing
    // the old sketch polyline from alternating with the black model edge.
    if (owner) {
        std::vector<App::DocumentObject*> pending {owner};
        std::set<App::DocumentObject*> visited {owner};
        for (std::size_t index = 0; index < pending.size(); ++index) {
            for (auto* dependent : pending[index]->getInList()) {
                if (!dependent || dependent->isRemoving() || !visited.insert(dependent).second) {
                    continue;
                }
                pending.push_back(dependent);

                Part::TopoShape dependentShape
                    = Part::Feature::getTopoShape(dependent, Part::ShapeOption::ResolveLink);
                if (dependentShape.isNull()) {
                    continue;
                }

                dependentShape.setPlacement(App::GeoFeature::getGlobalPlacement(dependent));
                replayMatchingEdges(dependent, dependentShape, {});
                if (remainingEdges.empty()) {
                    return true;
                }
            }
        }
    }

    bool added = remainingEdges.empty();
    for (const std::size_t measuredIndex : remainingEdges) {
        added = addIndependentMeasuredEdgeHighlight(selectionRoot, measuredEdges[measuredIndex], color, true)
            || added;
    }
    return added;
}

bool addMeasuredCircularHighlight(
    Gui::SoFCSelectionRoot* selectionRoot,
    App::DocumentObject* sourceObject,
    const std::string& subName,
    const SbColor& color
)
{
    return addMeasuredEdgeHighlights(selectionRoot, sourceObject, subName, color, true);
}

bool replayMeasuredGeometryHighlight(
    Gui::SoFCSelectionRoot* selectionRoot,
    App::DocumentObject* sourceObject,
    const std::string& subName,
    const SbColor& color,
    bool selectOnlyThisElement = false
)
{
    if (!selectionRoot || !sourceObject) {
        return false;
    }

    SoTempPath sourcePath(32);
    sourcePath.ref();
    SoDetail* detail = nullptr;

    if (!buildMeasuredGeometryHighlightPath(sourceObject, subName, sourcePath, detail)) {
        delete detail;
        sourcePath.unrefNoDelete();
        return false;
    }

    auto* annotation = new Gui::SoFCPathAnnotation();
    annotation->setPath(&sourcePath);
    selectionRoot->addChild(annotation);

    SoTempPath actionPath(sourcePath.getLength() + 2);
    actionPath.ref();
    actionPath.append(selectionRoot);
    actionPath.append(annotation);
    actionPath.append(&sourcePath);

    // Restrict the replayed path to the measured sub-element when a detail is available.
    if (detail) {
        Gui::SoSelectionElementAction filterAction(Gui::SoSelectionElementAction::Append, true);
        filterAction.setElement(detail);
        // Set the color explicitly; the secondary selection context otherwise defaults to black.
        filterAction.setColor(color);
        filterAction.apply(&actionPath);
    }

    if (selectOnlyThisElement && detail) {
        // Radius/diameter colors only the edge detail. A root-wide highlight can also tint the
        // generated Sketcher face, which must remain unchanged.
        Gui::SoSelectionElementAction edgeAction(Gui::SoSelectionElementAction::Append);
        edgeAction.setElement(detail);
        edgeAction.setColor(color);
        edgeAction.apply(&actionPath);
    }

    actionPath.unrefNoDelete();
    annotation->setDetail(detail);
    sourcePath.unrefNoDelete();
    return true;
}

bool addMeasuredGeometryHighlight(
    Gui::SoFCSelectionRoot* selectionRoot,
    App::DocumentObject* sourceObject,
    const std::string& subName,
    const SbColor& color,
    bool selectOnlyThisElement = false
)
{
    if (
        replayMeasuredGeometryHighlight(selectionRoot, sourceObject, subName, color, selectOnlyThisElement)
    ) {
        return true;
    }

    // Hidden predecessor features have no renderable scene path. Preserve the old independent
    // fallback for non-circular measurements; circular measurements first search the current
    // visible dependent feature and therefore normally never reach it.
    return addIndependentMeasuredEdgeHighlight(selectionRoot, sourceObject, subName, color);
}

}  // namespace

ViewProviderMeasureLength::ViewProviderMeasureLength()
{
    sPixmap = "Measurement-Distance";
}

void ViewProviderMeasure::ensureMeasuredGeometryHighlight()
{
    if (pMeasuredGeometrySelectionRoot) {
        attachMeasuredGeometryHighlightToViews();
        return;
    }

    pMeasuredGeometryHighlight = new SoSeparator();
    pMeasuredGeometryHighlight->ref();

    // Reuse FreeCAD's selection renderer so theme color and line-thickening settings are respected.
    auto* pickStyle = new SoPickStyle();
    pickStyle->style = SoPickStyle::UNPICKABLE;
    pickStyle->setOverride(true);
    pMeasuredGeometryHighlight->addChild(pickStyle);

    // Keep selection state private so the overlay does not modify the source ViewProvider.
    pMeasuredGeometrySelectionRoot = new Gui::SoFCSelectionRoot();
    pMeasuredGeometrySelectionRoot->selectionStyle = Gui::SoFCSelectionRoot::PassThrough;
    pMeasuredGeometryHighlight->addChild(pMeasuredGeometrySelectionRoot);

    // Do not put the persistent geometry overlay below the measurement ViewProvider root.
    // Tree selection colors that root green and would therefore recolor every replayed source edge.
    // A separate top-level switch keeps the overlay outside that selection context while still
    // following the measurement's normal visibility/display-mode switch.
    pMeasuredGeometryVisibilitySwitch = new SoSwitch();
    pMeasuredGeometryVisibilitySwitch->ref();
    pMeasuredGeometryVisibilitySwitch->whichChild.connectFrom(&pcModeSwitch->whichChild);
    pMeasuredGeometryVisibilitySwitch->addChild(pMeasuredGeometryHighlight);

    attachMeasuredGeometryHighlightToViews();
}

void ViewProviderMeasure::attachMeasuredGeometryHighlightToViews()
{
    if (!pMeasuredGeometryVisibilitySwitch) {
        return;
    }

    Gui::Document* guiDocument = nullptr;
    try {
        guiDocument = getDocument();
    }
    catch (const Base::RuntimeError&) {
        return;
    }
    if (!guiDocument) {
        return;
    }

    for (Gui::MDIView* mdiView : guiDocument->getMDIViews(true)) {
        auto* view = dynamic_cast<Gui::View3DInventor*>(mdiView);
        if (!view || !view->getViewer()) {
            continue;
        }

        auto* sceneGraph = dynamic_cast<SoGroup*>(view->getViewer()->getSceneGraph());
        if (sceneGraph && sceneGraph->findChild(pMeasuredGeometryVisibilitySwitch) < 0) {
            sceneGraph->addChild(pMeasuredGeometryVisibilitySwitch);
        }
    }
}

void ViewProviderMeasure::detachMeasuredGeometryHighlightFromViews()
{
    if (!pMeasuredGeometryVisibilitySwitch) {
        return;
    }

    Gui::Document* guiDocument = nullptr;
    try {
        guiDocument = getDocument();
    }
    catch (const Base::RuntimeError&) {
        return;
    }
    if (!guiDocument) {
        return;
    }

    for (Gui::MDIView* mdiView : guiDocument->getMDIViews(true)) {
        auto* view = dynamic_cast<Gui::View3DInventor*>(mdiView);
        if (!view || !view->getViewer()) {
            continue;
        }

        auto* sceneGraph = dynamic_cast<SoGroup*>(view->getViewer()->getSceneGraph());
        if (!sceneGraph) {
            continue;
        }

        const int childIndex = sceneGraph->findChild(pMeasuredGeometryVisibilitySwitch);
        if (childIndex >= 0) {
            sceneGraph->removeChild(childIndex);
        }
    }
}

void ViewProviderMeasure::clearMeasuredGeometryHighlight()
{
    if (!pMeasuredGeometrySelectionRoot) {
        return;
    }

    // Clear private highlight/selection state before rebuilding the overlay.
    Gui::SoHighlightElementAction highlightAction;
    highlightAction.setHighlighted(false);
    highlightAction.apply(pMeasuredGeometrySelectionRoot);

    Gui::SoSelectionElementAction secondaryAction(Gui::SoSelectionElementAction::None, true);
    secondaryAction.apply(pMeasuredGeometrySelectionRoot);

    // Radius/diameter also uses the primary selection context for edge-only coloring.
    Gui::SoSelectionElementAction primaryAction(Gui::SoSelectionElementAction::None);
    primaryAction.apply(pMeasuredGeometrySelectionRoot);

    while (pMeasuredGeometrySelectionRoot->getNumChildren() > 0) {
        pMeasuredGeometrySelectionRoot->removeChild(0);
    }
}

void ViewProviderMeasure::updateMeasuredGeometryHighlight()
{
    clearMeasuredGeometryHighlight();

    std::vector<App::DocumentObject*> objects;
    std::vector<std::string> subElements;

    if (auto* measure = dynamic_cast<Measure::MeasureLength*>(pcObject)) {
        objects = measure->Elements.getValues();
        subElements = measure->Elements.getSubValues(true);
    }
    else if (auto* measure = dynamic_cast<Measure::MeasureArea*>(pcObject)) {
        objects = measure->Elements.getValues();
        subElements = measure->Elements.getSubValues(true);
    }
    else if (auto* measure = dynamic_cast<Measure::MeasureRadius*>(pcObject)) {
        objects.push_back(measure->Element.getValue());
        subElements = measure->Element.getSubValues(true);
    }
    else if (auto* measure = dynamic_cast<Measure::MeasureDiameter*>(pcObject)) {
        objects.push_back(measure->Element.getValue());
        subElements = measure->Element.getSubValues(true);
    }
    else if (dynamic_cast<ViewProviderMeasureCOM*>(this)) {
        // COM is MeasurePython; its dedicated ViewProvider identifies the "Element" input.
        auto* element = dynamic_cast<App::PropertyLinkSub*>(pcObject->getPropertyByName("Element"));
        if (!element || !element->getValue()) {
            return;
        }

        objects.push_back(element->getValue());
        const auto values = element->getSubValues(true);
        if (!values.empty()) {
            subElements.push_back(values.front());
        }
        else {
            // Empty subname means the whole measured object.
            subElements.emplace_back();
        }
    }
    else {
        return;
    }

    const auto count = std::min(objects.size(), subElements.size());
    if (count == 0) {
        return;
    }

    ensureMeasuredGeometryHighlight();

    const SbColor persistentColor = Gui::SelectionColors::defaultHighlightColor();

    const bool isRadiusOrDiameter = dynamic_cast<Measure::MeasureRadius*>(pcObject)
        || dynamic_cast<Measure::MeasureDiameter*>(pcObject);
    const bool isCenterOfMass = dynamic_cast<ViewProviderMeasureCOM*>(this) != nullptr;

    // The overlay is a top-level scene node, outside the measurement ViewProvider selection root.
    // Its own selection context therefore remains blue even when the saved measurement is selected
    // in the tree.
    Gui::SoSelectionElementAction persistentSelection(Gui::SoSelectionElementAction::All);
    persistentSelection.setColor(persistentColor);
    persistentSelection.apply(pMeasuredGeometrySelectionRoot);

    // Saved measurements keep the persistent highlight color even when the measurement object
    // itself is selected. Selection of a measurement must not recolor its measured geometry.
    const bool wholeObjectCenterOfMass = isCenterOfMass
        && std::ranges::any_of(subElements, [](const std::string& sub) { return sub.empty(); });
    if (wholeObjectCenterOfMass) {
        Gui::SoHighlightElementAction highlightAction;
        highlightAction.setHighlighted(true);
        highlightAction.setColor(persistentColor);
        highlightAction.apply(pMeasuredGeometrySelectionRoot);
    }

    std::set<std::pair<App::DocumentObject*, std::string>> addedHighlights;
    for (std::size_t i = 0; i < count; ++i) {
        auto* object = objects[i];
        if (!object || (subElements[i].empty() && !isCenterOfMass)) {
            continue;
        }

        if (isRadiusOrDiameter) {
            if (addedHighlights.emplace(object, subElements[i]).second) {
                addMeasuredCircularHighlight(
                    pMeasuredGeometrySelectionRoot,
                    object,
                    subElements[i],
                    persistentColor
                );
            }
            continue;
        }

        // Length, area and face-based center-of-mass measurements all use the same successor-edge
        // lookup as circles. This refreshes lines, polylines, arcs and splines against the current
        // visible Part Design result and reuses its exact ViewProvider tessellation.
        if (!isCenterOfMass || !subElements[i].empty()) {
            if (addedHighlights.emplace(object, subElements[i]).second) {
                addMeasuredEdgeHighlights(
                    pMeasuredGeometrySelectionRoot,
                    object,
                    subElements[i],
                    persistentColor
                );
            }
            continue;
        }

        // A whole-object center-of-mass measurement has no edge subname and intentionally keeps
        // the existing whole-object replay behavior.
        if (addedHighlights.emplace(object, std::string {}).second) {
            addMeasuredGeometryHighlight(pMeasuredGeometrySelectionRoot, object, {}, persistentColor, false);
        }
    }
}

PROPERTY_SOURCE(MeasureGui::ViewProviderMeasurePosition, MeasureGui::ViewProviderMeasure)
PROPERTY_SOURCE(MeasureGui::ViewProviderMeasureRadius, MeasureGui::ViewProviderMeasure)
PROPERTY_SOURCE(MeasureGui::ViewProviderMeasureDiameter, MeasureGui::ViewProviderMeasure)
PROPERTY_SOURCE(MeasureGui::ViewProviderMeasureCOM, MeasureGui::ViewProviderMeasure)
