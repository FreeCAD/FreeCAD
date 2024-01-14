/***************************************************************************
 *   Copyright (c) 2023 David Friedli <david[at]friedli-be.ch>             *
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

#include "Gui/Application.h"
#include "Gui/MDIView.h"
#include "PreCompiled.h"

#ifndef _PreComp_
# include <boost_signals2.hpp>
# include <boost/signals2/connection.hpp>

# include <Inventor/actions/SoGetMatrixAction.h>
# include <Inventor/nodes/SoAnnotation.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoCamera.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoIndexedLineSet.h>
# include <Inventor/nodes/SoMarkerSet.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <Inventor/draggers/SoTranslate2Dragger.h>
# include <Inventor/engines/SoComposeMatrix.h>
# include <Inventor/engines/SoTransformVec3f.h>
# include <Inventor/engines/SoConcatenate.h>
#endif

#include <App/DocumentObject.h>
#include <Base/Console.h>
#include <Gui/Document.h>
#include <Gui/ViewParams.h>
#include <Gui/Inventor/MarkerBitmaps.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

#include <Mod/Measure/App/Preferences.h>
#include "ViewProviderMeasureBase.h"


using namespace MeasureGui;
using namespace Measure;
namespace bp = boost::placeholders;

//NOLINTBEGIN
PROPERTY_SOURCE(MeasureGui::ViewProviderMeasureBase, Gui::ViewProviderDocumentObject)
//NOLINTEND

ViewProviderMeasureBase::ViewProviderMeasureBase()
{
    static const char *agroup = "Appearance";
//NOLINTBEGIN
    ADD_PROPERTY_TYPE(TextColor, (Preferences::defaultTextColor()), agroup, App::Prop_None, "Color for the measurement text");
    ADD_PROPERTY_TYPE(TextBackgroundColor, (Preferences::defaultTextBackgroundColor()), agroup, App::Prop_None, "Color for the measurement text background");
    ADD_PROPERTY_TYPE(LineColor, (Preferences::defaultLineColor()), agroup, App::Prop_None, "Color for the measurement lines");
    ADD_PROPERTY_TYPE(FontSize, (Preferences::defaultFontSize()), agroup, App::Prop_None, "Size of measurement text");
//NOLINTEND

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
    pDraggerOrientation = new SoTransform();
    pDraggerOrientation->ref();
    dragSeparator->addChild(pDraggerOrientation);
    dragSeparator->addChild(pDragger);

    // Transform drag location by dragger local orientation and connect to labelTranslation
    auto matrixEngine = new SoComposeMatrix();
    matrixEngine->rotation.connectFrom(&pDraggerOrientation->rotation);
    auto transformEngine = new SoTransformVec3f();
    transformEngine->vector.connectFrom(&pDragger->translation);
    transformEngine->matrix.connectFrom(&matrixEngine->matrix);
    pLabelTranslation->translation.connectFrom(&transformEngine->point);

    pTextSeparator = new SoSeparator();
    pTextSeparator->ref();
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

    TextColor.touch();
    TextBackgroundColor.touch();
    FontSize.touch();
    LineColor.touch();
    pRootSeparator->touch();
    pTextSeparator->touch();
    pLineSeparator->touch();

    // Register dragger callback
    auto dragger = pDragger;

    dragger->addValueChangedCallback(draggerChangedCallback, this);


    // Use the label node as the transform handle
    SoSearchAction sa;
    sa.setInterest(SoSearchAction::FIRST);
    sa.setSearchingAll(true);
    sa.setNode(pLabel);
    sa.apply(pcRoot);
    SoPath * labelPath = sa.getPath();
    assert(labelPath);
    dragger->setPartAsPath("translator", labelPath);

    // Hide the dragger feedback during translation
    dragger->setPart("translatorActive", NULL);
    dragger->setPart("xAxisFeedback", NULL);
    dragger->setPart("yAxisFeedback", NULL);
}

ViewProviderMeasureBase::~ViewProviderMeasureBase()
{
    pLabel->unref();
    pColor->unref();
    pDragger->unref();
    pDraggerOrientation->unref();
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


void ViewProviderMeasureBase::finishRestoring() {

    // Force measurement visibility when loading a document
    show();
}


void ViewProviderMeasureBase::onChanged(const App::Property* prop)
{
    if (prop == &TextColor) {
        const App::Color& color = TextColor.getValue();
        pLabel->textColor.setValue(color.r, color.g, color.b);
    }
    else if (prop == &TextBackgroundColor) {
        const App::Color& color = TextBackgroundColor.getValue();
        pLabel->backgroundColor.setValue(color.r, color.g, color.b);
    }
    else if (prop == &LineColor) {
        const App::Color& color = LineColor.getValue();
        pColor->rgb.setValue(color.r, color.g, color.b);
    }
    else if (prop == &FontSize) {
        pLabel->size = FontSize.getValue();
    }
}

void ViewProviderMeasureBase::draggerChangedCallback(void *data, SoDragger *) {
    auto me = static_cast<ViewProviderMeasureBase*>(data);
    me->onLabelMoved();
}

void ViewProviderMeasureBase::setLabelValue(const Base::Quantity& value) {
    pLabel->string.setValue(value.getUserString().toUtf8().constData());
}

void ViewProviderMeasureBase::setLabelValue(const QString& value) {

    auto lines = value.split(QString::fromLatin1("\n"));

    int i = 0;
    for (auto it : lines) {
        pLabel->string.set1Value(i, it.toUtf8().constData());
        i++;
    }
}

void ViewProviderMeasureBase::setLabelTranslation(const SbVec3f& position) {
    // Set the dragger translation to keep it in sync with pLabelTranslation
    pDragger->translation.setValue(position);
}


SoPickStyle* ViewProviderMeasureBase::getSoPickStyle() {
    auto ps = new SoPickStyle();
    ps->style = SoPickStyle::UNPICKABLE;
    return ps;
}

SoDrawStyle* ViewProviderMeasureBase::getSoLineStylePrimary() {
    auto style = new SoDrawStyle();
    style->lineWidth = 2.0f;
    return style;
}

SoDrawStyle* ViewProviderMeasureBase::getSoLineStyleSecondary() {
    auto style = new SoDrawStyle();
    style->lineWidth = 1.0f;
    return style;
}

SoSeparator* ViewProviderMeasureBase::getSoSeparatorText() {
    return pTextSeparator;
}


void ViewProviderMeasureBase::onGuiInit(const Measure::MeasureBase* measureObject) {
    (void)measureObject;
}

//! the App side has requested a redraw
void ViewProviderMeasureBase::onGuiUpdate(const Measure::MeasureBase* measureObject) {
    (void) measureObject;
    updateView();
}


void ViewProviderMeasureBase::attach(App::DocumentObject *pcObj)
{
    ViewProviderDocumentObject::attach(pcObj);

//NOLINTBEGIN
    auto bndInit = boost::bind(&ViewProviderMeasureBase::onGuiInit, this, bp::_1);
    auto bnd = boost::bind(&ViewProviderMeasureBase::onGuiUpdate, this, bp::_1);
//NOLINTEND

    auto feature = dynamic_cast<Measure::MeasureBase*>(pcObject);
    if (feature) {
        feature->signalGuiInit.connect(bndInit);
        feature->signalGuiUpdate.connect(bnd);
    }
}


//! handle changes to the feature's properties
void ViewProviderMeasureBase::updateData(const App::Property* prop)
{
    bool doUpdate = false;

    auto obj = getMeasureObject();
    if (!obj) {
        return;
    }

    // Check if one of the input properties has been changed
    auto inputProps = obj->getInputProps();
    if (std::find(inputProps.begin(), inputProps.end(), std::string(prop->getName())) != inputProps.end()) {
        doUpdate = true;

        // Add connections to be notified when the measured objects are changed
        connectToSubject(obj->getSubject());
    }

    // Check if the result prop has been changed
    auto resultProp = obj->getResultProp();
    if (resultProp && prop == resultProp){
        doUpdate = true;
    }

    if (doUpdate) {
        redrawAnnotation();
    }

    ViewProviderDocumentObject::updateData(prop);
}


// TODO: should this be pure virtual in vpMeasureBase?
void ViewProviderMeasureBase::redrawAnnotation()
{
//    Base::Console().Message("VPMB::redrawAnnotation()\n");
}

//! connect to the subject to receive visibility updates
void ViewProviderMeasureBase::connectToSubject(App::DocumentObject* subject)
{
    if (!subject) {
        return;
    }

    //NOLINTBEGIN
    auto bndVisibility = boost::bind(&ViewProviderMeasureBase::onSubjectVisibilityChanged, this, bp::_1, bp::_2);
    //NOLINTEND
    subject->signalChanged.connect(bndVisibility);
}

//! connect to the subject to receive visibility updates
void ViewProviderMeasureBase::connectToSubject(std::vector<App::DocumentObject*> subject)
{
    if (subject.empty()) {
        return;
    }

    //NOLINTBEGIN
    auto bndVisibility = boost::bind(&ViewProviderMeasureBase::onSubjectVisibilityChanged, this, bp::_1, bp::_2);
    //NOLINTEND

    // TODO: should we connect to all the subject objects when there is >1?
    auto proxy = subject.front();
    if (!proxy) {
        return;
    }
    proxy->signalChanged.connect(bndVisibility);
}


//! retrieve the feature
Measure::MeasureBase* ViewProviderMeasureBase::getMeasureObject()
{
    // Note: Cast to MeasurePropertyBase once we use it to provide the needed values e.g. basePosition textPosition etc.
    auto feature = dynamic_cast<Measure::MeasureBase*>(pcObject);
    if (!feature) {
        throw Base::RuntimeError("Feature not found for ViewProviderMeasureBase");
    }
    return feature;
}


//! calculate a good direction from the elements being measured to the annotation text based on the layout
//! of the elements and relationship with the cardinal axes and the view direction.  elementDirection
//! is expected to be a normalized vector.
//! an example of an elementDirection would be the vector from the start of a line to the end.
Base::Vector3d ViewProviderMeasureBase::getTextDirection(Base::Vector3d elementDirection, double tolerance)
{
    // TODO: this can fail if the active view is not a 3d view (spreadsheet, techdraw page) and something causes a measure to try to update
    // we need to search through the mdi views for a 3d view and take the direction from it (or decide that if the active view is not 3d,
    // assume we are looking from the front).
    Base::Vector3d viewDirection;
    Base::Vector3d upDirection;
    auto view = dynamic_cast<Gui::View3DInventor*>(Gui::Application::Instance->activeDocument()->getActiveView());
    if (view) {
        Gui::View3DInventorViewer* viewer = view->getViewer();
        viewDirection = toVector3d(viewer->getViewDirection()).Normalize();
        upDirection = toVector3d(viewer->getUpDirection()).Normalize();
        // Measure doesn't work with this kind of active view.  Might be dependency graph, might be TechDraw, or ????
        //throw Base::RuntimeError("Measure doesn't work with this kind of active view.");
    } else {
        viewDirection = Base::Vector3d(0.0, 1.0, 0.0);
        upDirection = Base::Vector3d(0.0, 0.0, 1.0);
    }

    Base::Vector3d textDirection = elementDirection.Cross(viewDirection);
    if (textDirection.Length() < tolerance)  {
        // either elementDirection and viewDirection are parallel or one of them is null.
        textDirection = elementDirection.Cross(upDirection);
    }

    return textDirection.Normalize();
}


//! true if the subject of this measurement is visible.  For Measures that have multiple object subject,
//! all of the subjects must be visible.
bool ViewProviderMeasureBase::isSubjectVisible()
{
    // we need these things to proceed
    if (!getMeasureObject() ||
        getMeasureObject()->getSubject().empty() ||
        !Gui::Application::Instance->getDocument(getMeasureObject()->getDocument()) ) {
        return false;
    }

    auto guiDoc = Gui::Application::Instance->getDocument(getMeasureObject()->getDocument());
    for (auto & obj : getMeasureObject()->getSubject()) {
        Gui::ViewProvider* vp = guiDoc->getViewProvider(obj);
        if (!vp || !vp->isVisible()) {
            return false;
        }
    }

    // all of the subject objects are visible
    return true;
}


//! gets called when the subject object issues a signalChanged (ie a property change).  We are only interested in the subject's
//! Visibility property
void ViewProviderMeasureBase::onSubjectVisibilityChanged(const App::DocumentObject& docObj, const App::Property& prop)
{
    std::string propName = prop.getName();
    if (propName == "Visibility") {
        if (!docObj.Visibility.getValue()) {
            // show ourselves only if subject is visible
            setVisible(false);
        } else {
            // here, we don't know if we should be visible or not, so we have to check the whole subject
            setVisible(isSubjectVisible());
        }
    }
}


//NOLINTBEGIN
PROPERTY_SOURCE(MeasureGui::ViewProviderMeasure, MeasureGui::ViewProviderMeasureBase)
//NOLINTEND


ViewProviderMeasure::ViewProviderMeasure()
{
    const size_t lineCount(3);

    // indexes used to create the edges
    // this makes a line from verts[0] to verts[1] above
    static const int32_t lines[lineCount] =
    {
        0,1,-1
    };

    pCoords = new SoCoordinate3();
    pCoords->ref();

    // Combine coordinates from baseTranslation and labelTranslation
    auto engineCat = new SoConcatenate(SoMFVec3f::getClassTypeId());
    auto origin = new SoSFVec3f();
    origin->setValue(0,0,0);
    engineCat->input[0]->connectFrom(origin);
    engineCat->input[1]->connectFrom(&pLabelTranslation->translation);
    pCoords->point.setNum(engineCat->output->getNumConnections());
    pCoords->point.connectFrom(engineCat->output);

    pLines  = new SoIndexedLineSet();
    pLines->ref();
    pLines->coordIndex.setNum(lineCount);
    pLines->coordIndex.setValues(0, lineCount, lines);

    auto lineSep = pLineSeparator;
    lineSep->addChild(pCoords);
    lineSep->addChild(pLines);
    auto points = new SoMarkerSet();
    points->markerIndex = Gui::Inventor::MarkerBitmaps::getMarkerIndex("CROSS",
            Gui::ViewParams::instance()->getMarkerSize());
    points->numPoints=1;
    lineSep->addChild(points);

    // Connect dragger local orientation to view orientation
    auto vp = dynamic_cast<Gui::View3DInventor*>(Gui::Application::Instance->activeDocument()->getActiveView());
    if (vp) {
        Gui::View3DInventorViewer* viewer = vp->getViewer();
        auto renderManager = viewer->getSoRenderManager();
        auto cam = renderManager->getCamera();
        pDraggerOrientation->rotation.connectFrom(&cam->orientation);
    }

    sPixmap = "umf-measurement";
}

ViewProviderMeasure::~ViewProviderMeasure()
{
    pCoords->unref();
    pLines->unref();
}


void ViewProviderMeasure::onGuiInit(const Measure::MeasureBase* measureObject) {
    (void)measureObject;

    // Initialize the text position
    Base::Vector3d textPos = getTextPosition();
    auto srcVec = SbVec3f(textPos.x, textPos.y, textPos.z);

    // Translate the position by the local dragger matrix (pDraggerOrientation) 
    auto view = dynamic_cast<Gui::View3DInventor*>(Gui::Application::Instance->activeDocument()->getActiveView());
    Gui::View3DInventorViewer* viewer = view->getViewer();
    auto gma = new SoGetMatrixAction(viewer->getSoRenderManager()->getViewportRegion());
    gma->apply(pDraggerOrientation);
    auto mat = gma->getMatrix();
    SbVec3f destVec(0, 0, 0);
    mat.multVecMatrix(srcVec, destVec);

    setLabelTranslation(destVec);
    updateView();
}

void ViewProviderMeasure::onChanged(const App::Property* prop)
{
    if (pcObject == nullptr) {
        return;
    }

    ViewProviderMeasureBase::onChanged(prop);
}



void ViewProviderMeasure::attach(App::DocumentObject* pcObject)
{
    ViewProviderMeasureBase::attach(pcObject);

}


//! repaint the annotation
void ViewProviderMeasure::redrawAnnotation()
{
    // point on element
    Base::Vector3d basePos = getBasePosition();
    pcTransform->translation.setValue(SbVec3f(basePos.x, basePos.y, basePos.z));

    setLabelValue(getMeasureObject()->getResultString());

    ViewProviderMeasureBase::redrawAnnotation();
    ViewProviderDocumentObject::updateView();
}


Base::Vector3d ViewProviderMeasure::getBasePosition(){
    auto measureObject = getMeasureObject();
    Base::Placement placement = measureObject->getPlacement();
    return placement.getPosition();
}


Base::Vector3d ViewProviderMeasure::getTextPosition(){
    auto basePoint = getBasePosition();
    Base::Vector3d textDirection(1.0, 1.0, 1.0);
    textDirection.Normalize();
    double length = 10.0;

    return basePoint + textDirection * length;
}

//! called by the system when it is time to display this measure
void ViewProviderMeasureBase::show()
{
    if (isSubjectVisible()) {
        // only show the annotation if the subject is visible.
        // this avoids disconnected annotations floating in space.
        ViewProviderDocumentObject::show();
    }
}
