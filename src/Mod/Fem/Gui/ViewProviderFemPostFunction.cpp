/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoSurroundScale.h>
# include <Inventor/nodes/SoLineSet.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoTransform.h>
# include <Inventor/nodes/SoMatrixTransform.h>
# include <Inventor/nodes/SoSphere.h>
# include <Inventor/manips/SoTransformManip.h>
# include <Inventor/manips/SoCenterballManip.h>
# include <Inventor/manips/SoTransformerManip.h>
# include <Inventor/manips/SoTransformBoxManip.h>
# include <Inventor/manips/SoHandleBoxManip.h>
# include <Inventor/manips/SoTabBoxManip.h>
# include <Inventor/actions/SoSearchAction.h>
# include <Inventor/engines/SoDecomposeMatrix.h>
# include <Inventor/draggers/SoCenterballDragger.h>
# include <Inventor/draggers/SoTransformerDragger.h>
# include <Inventor/draggers/SoTransformBoxDragger.h>
# include <Inventor/draggers/SoHandleBoxDragger.h>

# include <QMessageBox>

# include <boost/bind.hpp>

# include <math.h>
#endif

#include "ViewProviderFemPostFunction.h"
#include "TaskPostBoxes.h"
#include <Mod/Fem/App/FemPostFunction.h>
#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/SoNavigationDragger.h>
#include <Gui/Macro.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/Control.h>
#include <App/PropertyUnits.h>

#include "ui_PlaneWidget.h"
#include "ui_SphereWidget.h"

using namespace FemGui;

void FunctionWidget::setViewProvider(ViewProviderFemPostFunction* view) {

    m_view = view;
    m_object = static_cast<Fem::FemPostFunction*>(view->getObject());
    m_connection = m_object->getDocument()->signalChangedObject.connect(boost::bind(&FunctionWidget::onObjectsChanged, this, _1, _2));
}

void FunctionWidget::onObjectsChanged(const App::DocumentObject& obj, const App::Property& p) {

    if(&obj == m_object)
        onChange(p);
}


PROPERTY_SOURCE(FemGui::ViewProviderFemPostFunctionProvider, Gui::ViewProviderDocumentObject)

ViewProviderFemPostFunctionProvider::ViewProviderFemPostFunctionProvider() {

}

ViewProviderFemPostFunctionProvider::~ViewProviderFemPostFunctionProvider() {

}

std::vector< App::DocumentObject* > ViewProviderFemPostFunctionProvider::claimChildren(void) const {

    return static_cast<Fem::FemPostFunctionProvider*>(getObject())->Functions.getValues();
}

std::vector< App::DocumentObject* > ViewProviderFemPostFunctionProvider::claimChildren3D(void) const {
    return claimChildren();
}

void ViewProviderFemPostFunctionProvider::onChanged(const App::Property* prop) {
    Gui::ViewProviderDocumentObject::onChanged(prop);

    updateSize();
}

void ViewProviderFemPostFunctionProvider::updateData(const App::Property* prop) {
    Gui::ViewProviderDocumentObject::updateData(prop);

    if(strcmp(prop->getName(), "Functions") == 0) {
        updateSize();
    }
}

void ViewProviderFemPostFunctionProvider::updateSize() {

    std::vector< App::DocumentObject* > vec = claimChildren();
    for(std::vector< App::DocumentObject* >::iterator it = vec.begin(); it != vec.end(); ++it) {

        if(!(*it)->isDerivedFrom(Fem::FemPostFunction::getClassTypeId()))
            continue;

        ViewProviderFemPostFunction* vp = static_cast<FemGui::ViewProviderFemPostFunction*>(Gui::Application::Instance->getViewProvider(*it));
        vp->AutoScaleFactorX.setValue(SizeX.getValue());
        vp->AutoScaleFactorY.setValue(SizeY.getValue());
        vp->AutoScaleFactorZ.setValue(SizeZ.getValue());
    }
}



PROPERTY_SOURCE(FemGui::ViewProviderFemPostFunction, Gui::ViewProviderDocumentObject)

ViewProviderFemPostFunction::ViewProviderFemPostFunction() : m_autoscale(false), m_isDragging(false)
{

    ADD_PROPERTY_TYPE(AutoScaleFactorX, (1), "AutoScale", App::Prop_Hidden, "Automatic scaling factor");
    ADD_PROPERTY_TYPE(AutoScaleFactorY, (1), "AutoScale", App::Prop_Hidden, "Automatic scaling factor");
    ADD_PROPERTY_TYPE(AutoScaleFactorZ, (1), "AutoScale", App::Prop_Hidden, "Automatic scaling factor");

    m_geometrySeperator = new SoSeparator();
    m_geometrySeperator->ref();

    m_transform = new SoTransform();
    m_transform->ref();

    m_scale = new SoScale();
    m_scale->ref();
    m_scale->scaleFactor = SbVec3f(1,1,1);
}

ViewProviderFemPostFunction::~ViewProviderFemPostFunction()
{
    m_geometrySeperator->unref();
    m_manip->unref();
    m_scale->unref();
    //transform is unref'd when it is replaced by the dragger
}

void ViewProviderFemPostFunction::attach(App::DocumentObject *pcObj)
{
    ViewProviderDocumentObject::attach(pcObj);

    // setup the graph for editing the function unit geometry
    SoMaterial* color = new SoMaterial();
    color->diffuseColor.setValue(0,0,1);
    color->transparency.setValue(0.5);

    m_transform = new SoTransform;

    m_manip = setupManipulator();
    m_manip->ref();

    SoSeparator* pcEditNode = new SoSeparator();

    pcEditNode->addChild(color);
    pcEditNode->addChild(m_transform);
    pcEditNode->addChild(m_geometrySeperator);

    m_geometrySeperator->insertChild(m_scale, 0);

    // Now we replace the SoTransform node by a manipulator
    // Note: Even SoCenterballManip inherits from SoTransform
    // we cannot use it directly (in above code) because the
    // translation and center fields are overridden.
    SoSearchAction sa;
    sa.setInterest(SoSearchAction::FIRST);
    sa.setSearchingAll(FALSE);
    sa.setNode(m_transform);
    sa.apply(pcEditNode);
    SoPath * path = sa.getPath();
    if (path) {
        m_manip->replaceNode(path);

        SoDragger* dragger = m_manip->getDragger();
        dragger->addStartCallback(dragStartCallback, this);
        dragger->addFinishCallback(dragFinishCallback, this);
        dragger->addMotionCallback(dragMotionCallback, this);
    }

    addDisplayMaskMode(pcEditNode, "Default");
    setDisplayMaskMode("Default");
}

bool ViewProviderFemPostFunction::doubleClicked(void) {
    Gui::Application::Instance->activeDocument()->setEdit(this, (int)ViewProvider::Default);
    return true;
}


SoTransformManip* ViewProviderFemPostFunction::setupManipulator() {

    return new SoCenterballManip;
}


std::vector<std::string> ViewProviderFemPostFunction::getDisplayModes(void) const
{
    std::vector<std::string> StrList;
    StrList.push_back("Default");
    return StrList;
}

void ViewProviderFemPostFunction::dragStartCallback(void *data, SoDragger *)
{
    // This is called when a manipulator is about to manipulating
    Gui::Application::Instance->activeDocument()->openCommand("Edit Mirror");
    reinterpret_cast<ViewProviderFemPostFunction*>(data)->m_isDragging = true;

    ViewProviderFemPostFunction* that = reinterpret_cast<ViewProviderFemPostFunction*>(data);
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Fem");
    that->m_autoRecompute = hGrp->GetBool("PostAutoRecompute", false);
}

void ViewProviderFemPostFunction::dragFinishCallback(void *data, SoDragger *)
{
    // This is called when a manipulator has done manipulating
    Gui::Application::Instance->activeDocument()->commitCommand();

    ViewProviderFemPostFunction* that = reinterpret_cast<ViewProviderFemPostFunction*>(data);
    if(that->m_autoRecompute)
        that->getObject()->getDocument()->recompute();

    reinterpret_cast<ViewProviderFemPostFunction*>(data)->m_isDragging = false;
}

void ViewProviderFemPostFunction::dragMotionCallback(void *data, SoDragger *drag)
{
    ViewProviderFemPostFunction* that = reinterpret_cast<ViewProviderFemPostFunction*>(data);
    that->draggerUpdate(drag);

    if(that->m_autoRecompute)
        that->getObject()->getDocument()->recompute();
}


bool ViewProviderFemPostFunction::setEdit(int ModNum) {


     if (ModNum == ViewProvider::Default || ModNum == 1 ) {

        Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
        TaskDlgPost *postDlg = qobject_cast<TaskDlgPost*>(dlg);
        if (postDlg && postDlg->getView() != this)
            postDlg = 0; // another pad left open its task panel
        if (dlg && !postDlg) {
            QMessageBox msgBox;
            msgBox.setText(QObject::tr("A dialog is already open in the task panel"));
            msgBox.setInformativeText(QObject::tr("Do you want to close this dialog?"));
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::Yes);
            int ret = msgBox.exec();
            if (ret == QMessageBox::Yes)
                Gui::Control().reject();
            else
                return false;
        }

        // start the edit dialog
        if (postDlg)
            Gui::Control().showDialog(postDlg);
        else {
            postDlg = new TaskDlgPost(this);
            postDlg->appendBox(new TaskPostFunction(this));
            Gui::Control().showDialog(postDlg);
        }

        return true;
    }
    else {
        return ViewProviderDocumentObject::setEdit(ModNum);
    }
}

void ViewProviderFemPostFunction::unsetEdit(int ModNum) {

    if (ModNum == ViewProvider::Default) {
        // when pressing ESC make sure to close the dialog
        Gui::Control().closeDialog();
    }
    else {
        ViewProviderDocumentObject::unsetEdit(ModNum);
    }
}

void ViewProviderFemPostFunction::onChanged(const App::Property* prop) {

    Gui::ViewProviderDocumentObject::onChanged(prop);

    if(m_autoscale)
        m_scale->scaleFactor = SbVec3f(AutoScaleFactorX.getValue(), AutoScaleFactorY.getValue(), AutoScaleFactorZ.getValue());
}



//#################################################################################################

PROPERTY_SOURCE(FemGui::ViewProviderFemPostPlaneFunction, FemGui::ViewProviderFemPostFunction)

ViewProviderFemPostPlaneFunction::ViewProviderFemPostPlaneFunction() {

    sPixmap = "fem-post-geo-plane";

    setAutoScale(true);

    //setup the visualisation geometry
    SoCoordinate3* points = new SoCoordinate3();
    points->point.setNum(4);
    points->point.set1Value(0, -0.5, -0.5, 0);
    points->point.set1Value(1, -0.5,  0.5, 0);
    points->point.set1Value(2,  0.5,  0.5, 0);
    points->point.set1Value(3,  0.5, -0.5, 0);
    points->point.set1Value(4, -0.5, -0.5, 0);
    SoLineSet* line = new SoLineSet();
    getGeometryNode()->addChild(points);
    getGeometryNode()->addChild(line);
}

ViewProviderFemPostPlaneFunction::~ViewProviderFemPostPlaneFunction() {

}

void ViewProviderFemPostPlaneFunction::draggerUpdate(SoDragger* m) {

    Fem::FemPostPlaneFunction* func = static_cast<Fem::FemPostPlaneFunction*>(getObject());
    SoCenterballDragger* dragger = static_cast<SoCenterballDragger*>(m);

    // the new axis of the plane
    SbRotation rot, scaleDir;
    const SbVec3f& center = dragger->center.getValue();

    SbVec3f norm(0,0,1);
    dragger->rotation.getValue().multVec(norm,norm);
    func->Origin.setValue(center[0], center[1], center[2]);
    func->Normal.setValue(norm[0],norm[1],norm[2]);

    SbVec3f t = static_cast<SoCenterballManip*>(getManipulator())->translation.getValue();
    SbVec3f rt, irt;
    dragger->rotation.getValue().multVec(t,rt);
    dragger->rotation.getValue().inverse().multVec(t,irt);
}

void ViewProviderFemPostPlaneFunction::updateData(const App::Property* p) {

    Fem::FemPostPlaneFunction* func = static_cast<Fem::FemPostPlaneFunction*>(getObject());

    if(!isDragging() && (p == &func->Origin || p == &func->Normal)) {

        Base::Vector3d trans = func->Origin.getValue();
        Base::Vector3d norm = func->Normal.getValue();

        norm = norm / norm.Length();
        SbRotation rot(SbVec3f(0.,0.,1.), SbVec3f(norm.x, norm.y, norm.z));

        SbMatrix t, translate;
        t.setRotate(rot);
        translate.setTranslate(SbVec3f(trans.x, trans.y, trans.z));
        t.multRight(translate);
        getManipulator()->setMatrix(t);
    }
    Gui::ViewProviderDocumentObject::updateData(p);
}


FunctionWidget* ViewProviderFemPostPlaneFunction::createControlWidget() {
    return new PlaneWidget();
}


PlaneWidget::PlaneWidget() {

    ui = new Ui_PlaneWidget();
    ui->setupUi(this);

    connect(ui->originX, SIGNAL(valueChanged(double)), this, SLOT(originChanged(double)));
    connect(ui->originY, SIGNAL(valueChanged(double)), this, SLOT(originChanged(double)));
    connect(ui->originZ, SIGNAL(valueChanged(double)), this, SLOT(originChanged(double)));
    connect(ui->normalX, SIGNAL(valueChanged(double)), this, SLOT(normalChanged(double)));
    connect(ui->normalY, SIGNAL(valueChanged(double)), this, SLOT(normalChanged(double)));
    connect(ui->normalZ, SIGNAL(valueChanged(double)), this, SLOT(normalChanged(double)));

}

PlaneWidget::~PlaneWidget() {

}

void PlaneWidget::applyPythonCode() {

}

void PlaneWidget::setViewProvider(ViewProviderFemPostFunction* view) {

    FemGui::FunctionWidget::setViewProvider(view);
    onChange(static_cast<Fem::FemPostPlaneFunction*>(getObject())->Normal);
    onChange(static_cast<Fem::FemPostPlaneFunction*>(getObject())->Origin);
}

void PlaneWidget::onChange(const App::Property& p) {

    setBlockObjectUpdates(true);
    if(strcmp(p.getName(), "Normal") == 0) {
        const Base::Vector3d& vec = static_cast<const App::PropertyVector*>(&p)->getValue();
        ui->normalX->setValue(vec.x);
        ui->normalY->setValue(vec.y);
        ui->normalZ->setValue(vec.z);
    }
    else if(strcmp(p.getName(), "Origin") == 0) {
        const Base::Vector3d& vec = static_cast<const App::PropertyVectorDistance*>(&p)->getValue();
        ui->originX->setValue(vec.x);
        ui->originY->setValue(vec.y);
        ui->originZ->setValue(vec.z);
    }
    setBlockObjectUpdates(false);
}

void PlaneWidget::normalChanged(double) {

     if(!blockObjectUpdates()) {
        Base::Vector3d vec(ui->normalX->value(), ui->normalY->value(), ui->normalZ->value());
        static_cast<Fem::FemPostPlaneFunction*>(getObject())->Normal.setValue(vec);
     }
}

void PlaneWidget::originChanged(double) {

    if(!blockObjectUpdates()) {
        Base::Vector3d vec(ui->originX->value(), ui->originY->value(), ui->originZ->value());
        static_cast<Fem::FemPostPlaneFunction*>(getObject())->Origin.setValue(vec);
    }
}



//#################################################################################################

PROPERTY_SOURCE(FemGui::ViewProviderFemPostSphereFunction, FemGui::ViewProviderFemPostFunction)

ViewProviderFemPostSphereFunction::ViewProviderFemPostSphereFunction() {

    sPixmap = "fem-post-geo-sphere";

    setAutoScale(false);

    //setup the visualisation geometry
    SoCoordinate3* points = new SoCoordinate3();
    points->point.setNum(2*84);
    int idx = 0;
    for(int i=0; i<4; i++) {
        for(int j=0; j<21; j++) {
            points->point.set1Value(idx, SbVec3f(std::sin(2*M_PI/20*j) * std::cos(M_PI/4*i),
                                                 std::sin(2*M_PI/20*j) * std::sin(M_PI/4*i),
                                                 std::cos(2*M_PI/20*j) ));
            ++idx;
        }
    }
    for(int i=0; i<4; i++) {
        for(int j=0; j<21; j++) {
            points->point.set1Value(idx, SbVec3f(std::sin(M_PI/4*i) * std::cos(2*M_PI/20*j),
                                                 std::sin(M_PI/4*i) * std::sin(2*M_PI/20*j),
                                                 std::cos(M_PI/4*i) ));
            ++idx;
        }
    }

    SoLineSet* line = new SoLineSet();
    getGeometryNode()->addChild(points);
    getGeometryNode()->addChild(line);
}

ViewProviderFemPostSphereFunction::~ViewProviderFemPostSphereFunction() {
}

SoTransformManip* ViewProviderFemPostSphereFunction::setupManipulator() {
    SoHandleBoxManip* manip = new SoHandleBoxManip();
    manip->getDragger()->setPart("extruder1", new SoSeparator);
    manip->getDragger()->setPart("extruder2", new SoSeparator);
    manip->getDragger()->setPart("extruder3", new SoSeparator);
    manip->getDragger()->setPart("extruder4", new SoSeparator);
    manip->getDragger()->setPart("extruder5", new SoSeparator);
    manip->getDragger()->setPart("extruder6", new SoSeparator);
    manip->getDragger()->setPart("extruder1Active", new SoSeparator);
    manip->getDragger()->setPart("extruder2Active", new SoSeparator);
    manip->getDragger()->setPart("extruder3Active", new SoSeparator);
    manip->getDragger()->setPart("extruder4Active", new SoSeparator);
    manip->getDragger()->setPart("extruder5Active", new SoSeparator);
    manip->getDragger()->setPart("extruder6Active", new SoSeparator);

    return manip;
}


void ViewProviderFemPostSphereFunction::draggerUpdate(SoDragger* m) {

    Fem::FemPostSphereFunction* func = static_cast<Fem::FemPostSphereFunction*>(getObject());
    SoHandleBoxDragger* dragger = static_cast<SoHandleBoxDragger*>(m);

    // the new axis of the plane
    SbRotation rot, scaleDir;
    const SbVec3f& center = dragger->translation.getValue();

    SbVec3f norm(0,0,1);
    func->Center.setValue(center[0], center[1], center[2]);
    func->Radius.setValue(dragger->scaleFactor.getValue()[0]);
}

void ViewProviderFemPostSphereFunction::updateData(const App::Property* p) {

    Fem::FemPostSphereFunction* func = static_cast<Fem::FemPostSphereFunction*>(getObject());

    if(!isDragging() && (p == &func->Center || p == &func->Radius)) {

        Base::Vector3d trans = func->Center.getValue();
        double radius = func->Radius.getValue();

        SbMatrix t, translate;
        t.setScale(radius);
        translate.setTranslate(SbVec3f(trans.x, trans.y, trans.z));
        t.multRight(translate);
        getManipulator()->setMatrix(t);

    }
    Gui::ViewProviderDocumentObject::updateData(p);
}


FunctionWidget* ViewProviderFemPostSphereFunction::createControlWidget() {
    return new SphereWidget();
}


SphereWidget::SphereWidget() {

    ui = new Ui_SphereWidget();
    ui->setupUi(this);

    connect(ui->centerX, SIGNAL(valueChanged(double)), this, SLOT(centerChanged(double)));
    connect(ui->centerY, SIGNAL(valueChanged(double)), this, SLOT(centerChanged(double)));
    connect(ui->centerZ, SIGNAL(valueChanged(double)), this, SLOT(centerChanged(double)));
    connect(ui->radius, SIGNAL(valueChanged(double)), this, SLOT(radiusChanged(double)));
}

SphereWidget::~SphereWidget() {

}

void SphereWidget::applyPythonCode() {

}

void SphereWidget::setViewProvider(ViewProviderFemPostFunction* view) {

    FemGui::FunctionWidget::setViewProvider(view);
    onChange(static_cast<Fem::FemPostSphereFunction*>(getObject())->Center);
    onChange(static_cast<Fem::FemPostSphereFunction*>(getObject())->Radius);
}

void SphereWidget::onChange(const App::Property& p) {

    setBlockObjectUpdates(true);
    if(strcmp(p.getName(), "Radius") == 0) {
        double val = static_cast<const App::PropertyDistance*>(&p)->getValue();
        ui->radius->setValue(val);
    }
    else if(strcmp(p.getName(), "Center") == 0) {
        const Base::Vector3d& vec = static_cast<const App::PropertyVectorDistance*>(&p)->getValue();
        ui->centerX->setValue(vec.x);
        ui->centerY->setValue(vec.y);
        ui->centerZ->setValue(vec.z);
    }
    setBlockObjectUpdates(false);
}

void SphereWidget::centerChanged(double) {

     if(!blockObjectUpdates()) {
        Base::Vector3d vec(ui->centerX->value(), ui->centerY->value(), ui->centerZ->value());
        static_cast<Fem::FemPostSphereFunction*>(getObject())->Center.setValue(vec);
     }
}

void SphereWidget::radiusChanged(double) {

    if(!blockObjectUpdates()) {
        static_cast<Fem::FemPostSphereFunction*>(getObject())->Radius.setValue(ui->radius->value());
    }
}

#include "moc_ViewProviderFemPostFunction.cpp"
