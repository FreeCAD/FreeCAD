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
# include <Inventor/actions/SoSearchAction.h>
# include <Inventor/draggers/SoCenterballDragger.h>
# include <Inventor/draggers/SoHandleBoxDragger.h>
# include <Inventor/draggers/SoJackDragger.h>
# include <Inventor/manips/SoCenterballManip.h>
# include <Inventor/manips/SoHandleBoxManip.h>
# include <Inventor/manips/SoJackManip.h>
# include <Inventor/manips/SoTransformManip.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoLineSet.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoTransform.h>

# include <QApplication>
# include <QMessageBox>
# include <QTextStream>

# include <cmath>
#endif

#include <App/Document.h>
#include <App/PropertyUnits.h>
#include <Base/UnitsApi.h>
#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Mod/Fem/App/FemAnalysis.h>
#include <Mod/Fem/App/FemPostPipeline.h>

#include "ViewProviderFemPostFunction.h"
#include "ActiveAnalysisObserver.h"
#include "FemSettings.h"
#include "TaskPostBoxes.h"

#include "ui_PlaneWidget.h"
#include "ui_SphereWidget.h"


using namespace FemGui;
namespace bp = boost::placeholders;

void FunctionWidget::setViewProvider(ViewProviderFemPostFunction* view) {

    m_view = view;
    m_object = static_cast<Fem::FemPostFunction*>(view->getObject());
    m_connection = m_object->getDocument()->signalChangedObject.connect(boost::bind(&FunctionWidget::onObjectsChanged, this, bp::_1, bp::_2));
}

void FunctionWidget::onObjectsChanged(const App::DocumentObject& obj, const App::Property& p) {

    if (&obj == m_object)
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

    if (strcmp(prop->getName(), "Functions") == 0) {
        updateSize();
    }
}

void ViewProviderFemPostFunctionProvider::updateSize() {

    std::vector< App::DocumentObject* > vec = claimChildren();
    for (std::vector< App::DocumentObject* >::iterator it = vec.begin(); it != vec.end(); ++it) {

        if (!(*it)->isDerivedFrom(Fem::FemPostFunction::getClassTypeId()))
            continue;

        ViewProviderFemPostFunction* vp = static_cast<FemGui::ViewProviderFemPostFunction*>(Gui::Application::Instance->getViewProvider(*it));
        vp->AutoScaleFactorX.setValue(SizeX.getValue());
        vp->AutoScaleFactorY.setValue(SizeY.getValue());
        vp->AutoScaleFactorZ.setValue(SizeZ.getValue());
    }
}

bool ViewProviderFemPostFunctionProvider::onDelete(const std::vector<std::string>&)
{
    // warn the user if the object has childs

    auto objs = claimChildren();
    if (!objs.empty())
    {
        // generate dialog
        QString bodyMessage;
        QTextStream bodyMessageStream(&bodyMessage);
        bodyMessageStream << qApp->translate("Std_Delete",
            "The functions list is not empty, therefore the\nfollowing referencing objects might be lost:");
        bodyMessageStream << '\n';
        for (auto ObjIterator : objs)
            bodyMessageStream << '\n' << QString::fromUtf8(ObjIterator->Label.getValue());
        bodyMessageStream << "\n\n" << QObject::tr("Are you sure you want to continue?");
        // show and evaluate the dialog
        int DialogResult = QMessageBox::warning(Gui::getMainWindow(),
            qApp->translate("Std_Delete", "Object dependencies"), bodyMessage,
            QMessageBox::Yes, QMessageBox::No);
        if (DialogResult == QMessageBox::Yes)
            return true;
        else
            return false;
    }
    else {
        return true;
    }
}

bool ViewProviderFemPostFunctionProvider::canDelete(App::DocumentObject* obj) const
{
    // deletions of objects from a FemFunction don't necessarily destroy anything
    // thus we can pass this action
    // we can warn the user if necessary in the object's ViewProvider in the onDelete() function
    Q_UNUSED(obj)
        return true;
}


// ***************************************************************************

PROPERTY_SOURCE(FemGui::ViewProviderFemPostFunction, Gui::ViewProviderDocumentObject)

ViewProviderFemPostFunction::ViewProviderFemPostFunction()
    : m_manip(nullptr), m_autoscale(false), m_isDragging(false), m_autoRecompute(false)
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
    m_scale->scaleFactor = SbVec3f(1, 1, 1);
}

ViewProviderFemPostFunction::~ViewProviderFemPostFunction()
{
    m_geometrySeperator->unref();
    m_manip->unref();
    m_scale->unref();
    //transform is unref'd when it is replaced by the dragger
}

void ViewProviderFemPostFunction::attach(App::DocumentObject* pcObj)
{
    ViewProviderDocumentObject::attach(pcObj);

    // setup the graph for editing the function unit geometry
    SoMaterial* color = new SoMaterial();
    color->diffuseColor.setValue(0, 0, 1);
    color->transparency.setValue(0.5);

    m_transform = new SoTransform;

    m_manip = setupManipulator();
    m_manip->ref();

    SoSeparator* pcEditNode = new SoSeparator();
    pcEditNode->ref();

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
    SoPath* path = sa.getPath();
    if (path) {
        m_manip->replaceNode(path);

        SoDragger* dragger = m_manip->getDragger();
        dragger->addStartCallback(dragStartCallback, this);
        dragger->addFinishCallback(dragFinishCallback, this);
        dragger->addMotionCallback(dragMotionCallback, this);
    }

    addDisplayMaskMode(pcEditNode, "Default");
    setDisplayMaskMode("Default");
    pcEditNode->unref();
}

SbBox3f ViewProviderFemPostFunction::getBoundingsOfView() const
{
    SbBox3f box;
    Gui::Document* doc = this->getDocument();
    Gui::View3DInventor* view = qobject_cast<Gui::View3DInventor*>(doc->getViewOfViewProvider(this));
    if (view) {
        Gui::View3DInventorViewer* viewer = view->getViewer();
        box = viewer->getBoundingBox();
    }

    return box;
}

bool ViewProviderFemPostFunction::findScaleFactor(double& scale) const
{
    SbBox3f bbox = getBoundingsOfView();
    if (bbox.hasVolume()) {
        float dx, dy, dz;
        bbox.getSize(dx, dy, dz);
        // we want the manipulator to have 20 % of the max size of the object
        scale = 0.2 * std::max(std::max(dx, dy), dz);
        return true;
    }

    return false;
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

void ViewProviderFemPostFunction::dragStartCallback(void* data, SoDragger*)
{
    // This is called when a manipulator is about to manipulating
    Gui::Application::Instance->activeDocument()->openCommand(QT_TRANSLATE_NOOP("Command", "Edit Mirror"));
    static_cast<ViewProviderFemPostFunction*>(data)->m_isDragging = true;

    ViewProviderFemPostFunction* that = static_cast<ViewProviderFemPostFunction*>(data);
    that->m_autoRecompute = FemSettings().getPostAutoRecompute();
}

void ViewProviderFemPostFunction::dragFinishCallback(void* data, SoDragger*)
{
    // This is called when a manipulator has done manipulating
    Gui::Application::Instance->activeDocument()->commitCommand();

    ViewProviderFemPostFunction* that = static_cast<ViewProviderFemPostFunction*>(data);
    if (that->m_autoRecompute)
        that->getObject()->getDocument()->recompute();

    static_cast<ViewProviderFemPostFunction*>(data)->m_isDragging = false;
}

void ViewProviderFemPostFunction::dragMotionCallback(void* data, SoDragger* drag)
{
    ViewProviderFemPostFunction* that = static_cast<ViewProviderFemPostFunction*>(data);
    that->draggerUpdate(drag);

    if (that->m_autoRecompute)
        that->getObject()->getDocument()->recompute();
}


bool ViewProviderFemPostFunction::setEdit(int ModNum) {


    if (ModNum == ViewProvider::Default || ModNum == 1) {

        Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
        TaskDlgPost* postDlg = qobject_cast<TaskDlgPost*>(dlg);
        if (postDlg && postDlg->getView() != this)
            postDlg = nullptr; // another pad left open its task panel
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

    if (m_autoscale)
        m_scale->scaleFactor = SbVec3f(AutoScaleFactorX.getValue(), AutoScaleFactorY.getValue(), AutoScaleFactorZ.getValue());
}


// ***************************************************************************

PROPERTY_SOURCE(FemGui::ViewProviderFemPostPlaneFunction, FemGui::ViewProviderFemPostFunction)
//NOTE: The technical lower limit is at 1e-4 that the Coin3D manipulator can handle
static const App::PropertyFloatConstraint::Constraints scaleConstraint = {1e-4, DBL_MAX, 1.0};

ViewProviderFemPostPlaneFunction::ViewProviderFemPostPlaneFunction()
    : m_detectscale(false)
{

    ADD_PROPERTY_TYPE(Scale, (1000.0), "Manipulator", App::Prop_None, "Scaling factor for the manipulator");
    Scale.setConstraints(&scaleConstraint);
    sPixmap = "fem-post-geo-plane";

    setAutoScale(true);

    //setup the visualisation geometry
    SoCoordinate3* points = new SoCoordinate3();
    points->point.setNum(4);
    points->point.set1Value(0, -0.5, -0.5, 0);
    points->point.set1Value(1, -0.5, 0.5, 0);
    points->point.set1Value(2, 0.5, 0.5, 0);
    points->point.set1Value(3, 0.5, -0.5, 0);
    points->point.set1Value(4, -0.5, -0.5, 0);
    SoLineSet* line = new SoLineSet();
    getGeometryNode()->addChild(points);
    getGeometryNode()->addChild(line);
}

ViewProviderFemPostPlaneFunction::~ViewProviderFemPostPlaneFunction() {

}

void ViewProviderFemPostPlaneFunction::draggerUpdate(SoDragger* m) {

    Fem::FemPostPlaneFunction* func = static_cast<Fem::FemPostPlaneFunction*>(getObject());
    SoJackDragger* dragger = static_cast<SoJackDragger*>(m);

    // the new axis of the plane
    const SbVec3f& base = dragger->translation.getValue();
    const SbVec3f& scale = dragger->scaleFactor.getValue();

    SbVec3f norm(0., 1., 0.);
    dragger->rotation.getValue().multVec(norm, norm);
    func->Origin.setValue(base[0], base[1], base[2]);
    func->Normal.setValue(norm[0], norm[1], norm[2]);
    this->Scale.setValue(scale[0]);
}

void ViewProviderFemPostPlaneFunction::onChanged(const App::Property* prop)
{
    if (prop == &Scale) {
        // When loading the Scale property from a project then keep that
        if (Scale.getConstraints())
            m_detectscale = true;
        if (!isDragging()) {
            // get current matrix
            SbVec3f t, s;
            SbRotation r, so;
            SbMatrix matrix = getManipulator()->getDragger()->getMotionMatrix();
            matrix.getTransform(t, r, s, so);

            float scale = static_cast<float>(Scale.getValue());
            s.setValue(scale, scale, scale);

            matrix.setTransform(t, r, s, so);
            getManipulator()->setMatrix(matrix);
        }
    }
    ViewProviderFemPostFunction::onChanged(prop);
}

void ViewProviderFemPostPlaneFunction::updateData(const App::Property* p) {

    Fem::FemPostPlaneFunction* func = static_cast<Fem::FemPostPlaneFunction*>(getObject());

    if (!isDragging() && (p == &func->Origin || p == &func->Normal)) {
        if (!m_detectscale) {
            double s;
            if (findScaleFactor(s)) {
                m_detectscale = true;
                this->Scale.setValue(s);
            }
        }

        Base::Vector3d trans = func->Origin.getValue();
        Base::Vector3d norm = func->Normal.getValue();

        norm.Normalize();
        SbRotation rot(SbVec3f(0., 1., 0.), SbVec3f(norm.x, norm.y, norm.z));
        float scale = static_cast<float>(Scale.getValue());

        SbMatrix mat;
        mat.setTransform(SbVec3f(trans.x, trans.y, trans.z), rot, SbVec3f(scale, scale, scale));
        getManipulator()->setMatrix(mat);
    }
    Gui::ViewProviderDocumentObject::updateData(p);
}


SoTransformManip* ViewProviderFemPostPlaneFunction::setupManipulator()
{
    return new SoJackManip;
}


FunctionWidget* ViewProviderFemPostPlaneFunction::createControlWidget() {
    return new PlaneWidget();
}


PlaneWidget::PlaneWidget() {

    ui = new Ui_PlaneWidget();
    ui->setupUi(this);

    QSize size = ui->originX->sizeForText(QStringLiteral("000000000000"));
    ui->originX->setMinimumWidth(size.width());
    ui->originY->setMinimumWidth(size.width());
    ui->originZ->setMinimumWidth(size.width());
    ui->normalX->setMinimumWidth(size.width());
    ui->originY->setMinimumWidth(size.width());
    ui->originZ->setMinimumWidth(size.width());

    int UserDecimals = Base::UnitsApi::getDecimals();
    ui->originX->setDecimals(UserDecimals);
    ui->originY->setDecimals(UserDecimals);
    ui->originZ->setDecimals(UserDecimals);
    ui->normalX->setDecimals(UserDecimals);
    ui->normalY->setDecimals(UserDecimals);
    ui->normalZ->setDecimals(UserDecimals);

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
    const Base::Unit unit = static_cast<Fem::FemPostPlaneFunction*>(getObject())->Origin.getUnit();
    setBlockObjectUpdates(true);
    ui->originX->setUnit(unit);
    ui->originY->setUnit(unit);
    ui->originZ->setUnit(unit);
    setBlockObjectUpdates(false);
    // The normal vector is unitless. It uses nevertheless Gui::PrefQuantitySpinBox to keep dialog uniform.
    onChange(static_cast<Fem::FemPostPlaneFunction*>(getObject())->Normal);
    onChange(static_cast<Fem::FemPostPlaneFunction*>(getObject())->Origin);
}

void PlaneWidget::onChange(const App::Property& p) {

    setBlockObjectUpdates(true);
    if (strcmp(p.getName(), "Normal") == 0) {
        const Base::Vector3d& vec = static_cast<const App::PropertyVector*>(&p)->getValue();
        ui->normalX->setValue(vec.x);
        ui->normalY->setValue(vec.y);
        ui->normalZ->setValue(vec.z);
    }
    else if (strcmp(p.getName(), "Origin") == 0) {
        const Base::Vector3d& vec = static_cast<const App::PropertyVectorDistance*>(&p)->getValue();
        ui->originX->setValue(vec.x);
        ui->originY->setValue(vec.y);
        ui->originZ->setValue(vec.z);
    }
    setBlockObjectUpdates(false);
}

void PlaneWidget::normalChanged(double) {

    if (!blockObjectUpdates()) {
        Base::Vector3d vec(ui->normalX->value().getValue(), ui->normalY->value().getValue(),
            ui->normalZ->value().getValue());
        static_cast<Fem::FemPostPlaneFunction*>(getObject())->Normal.setValue(vec);
    }
}

void PlaneWidget::originChanged(double) {

    if (!blockObjectUpdates()) {
        Base::Vector3d vec(ui->originX->value().getValue(), ui->originY->value().getValue(),
            ui->originZ->value().getValue());
        static_cast<Fem::FemPostPlaneFunction*>(getObject())->Origin.setValue(vec);
    }
}


// ***************************************************************************

PROPERTY_SOURCE(FemGui::ViewProviderFemPostSphereFunction, FemGui::ViewProviderFemPostFunction)

ViewProviderFemPostSphereFunction::ViewProviderFemPostSphereFunction() {

    sPixmap = "fem-post-geo-sphere";

    setAutoScale(false);

    //setup the visualisation geometry
    SoCoordinate3* points = new SoCoordinate3();
    points->point.setNum(2 * 84);
    int idx = 0;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 21; j++) {
            points->point.set1Value(idx, SbVec3f(std::sin(2 * M_PI / 20 * j) * std::cos(M_PI / 4 * i),
                std::sin(2 * M_PI / 20 * j) * std::sin(M_PI / 4 * i),
                std::cos(2 * M_PI / 20 * j)));
            ++idx;
        }
    }
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 21; j++) {
            points->point.set1Value(idx, SbVec3f(std::sin(M_PI / 4 * i) * std::cos(2 * M_PI / 20 * j),
                std::sin(M_PI / 4 * i) * std::sin(2 * M_PI / 20 * j),
                std::cos(M_PI / 4 * i)));
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

    SbVec3f norm(0, 0, 1);
    func->Center.setValue(center[0], center[1], center[2]);
    func->Radius.setValue(dragger->scaleFactor.getValue()[0]);
}

void ViewProviderFemPostSphereFunction::updateData(const App::Property* p) {

    Fem::FemPostSphereFunction* func = static_cast<Fem::FemPostSphereFunction*>(getObject());

    if (!isDragging() && (p == &func->Center || p == &func->Radius)) {

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

    QSize size = ui->centerX->sizeForText(QStringLiteral("000000000000"));
    ui->centerX->setMinimumWidth(size.width());
    ui->centerY->setMinimumWidth(size.width());
    ui->centerZ->setMinimumWidth(size.width());
    ui->radius->setMinimumWidth(size.width());

    int UserDecimals = Base::UnitsApi::getDecimals();
    ui->centerX->setDecimals(UserDecimals);
    ui->centerY->setDecimals(UserDecimals);
    ui->centerZ->setDecimals(UserDecimals);

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
    setBlockObjectUpdates(true);
    Base::Unit unit = static_cast<Fem::FemPostSphereFunction*>(getObject())->Center.getUnit();
    ui->centerX->setUnit(unit);
    ui->centerY->setUnit(unit);
    ui->centerZ->setUnit(unit);
    unit = static_cast<Fem::FemPostSphereFunction*>(getObject())->Radius.getUnit();
    ui->radius->setUnit(unit);
    setBlockObjectUpdates(false);
    onChange(static_cast<Fem::FemPostSphereFunction*>(getObject())->Center);
    onChange(static_cast<Fem::FemPostSphereFunction*>(getObject())->Radius);
}

void SphereWidget::onChange(const App::Property& p) {

    setBlockObjectUpdates(true);
    if (strcmp(p.getName(), "Radius") == 0) {
        double val = static_cast<const App::PropertyDistance*>(&p)->getValue();
        ui->radius->setValue(val);
    }
    else if (strcmp(p.getName(), "Center") == 0) {
        const Base::Vector3d& vec = static_cast<const App::PropertyVectorDistance*>(&p)->getValue();
        ui->centerX->setValue(vec.x);
        ui->centerY->setValue(vec.y);
        ui->centerZ->setValue(vec.z);
    }
    setBlockObjectUpdates(false);
}

void SphereWidget::centerChanged(double) {

    if (!blockObjectUpdates()) {
        Base::Vector3d vec(ui->centerX->value().getValue(), ui->centerY->value().getValue(),
            ui->centerZ->value().getValue());
        static_cast<Fem::FemPostSphereFunction*>(getObject())->Center.setValue(vec);
    }
}

void SphereWidget::radiusChanged(double) {

    if (!blockObjectUpdates()) {
        static_cast<Fem::FemPostSphereFunction*>(getObject())->Radius.setValue(ui->radius->value().getValue());
    }
}

#include "moc_ViewProviderFemPostFunction.cpp"
