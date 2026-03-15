
/***************************************************************************
 *   Copyright (c) 2025 Stefan Tröger <stefantroeger@gmx.net>              *
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


#include <Gui/Application.h>
#include <Gui/Document.h>
#include <App/Document.h>
#include <Base/UnitsApi.h>

#include "ViewProviderShapeExtension.h"
#include "ViewProviderShapeExtensionPy.h"

#include <Mod/Fem/App/FemShapeExtension.h>

#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/draggers/SoDragPointDragger.h>
#include <Inventor/draggers/SoHandleBoxDragger.h>
#include <Inventor/draggers/SoJackDragger.h>
#include <Inventor/manips/SoCenterballManip.h>
#include <Inventor/manips/SoHandleBoxManip.h>
#include <Inventor/manips/SoJackManip.h>
#include <Inventor/manips/SoTransformManip.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTransform.h>

#include "FemSettings.h"

#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

#include "ui_BoxWidget.h"
#include "ui_CylinderWidget.h"
#include "ui_PlaneWidget.h"
#include "ui_SphereWidget.h"


using namespace FemGui;
namespace sp = std::placeholders;

EXTENSION_PROPERTY_SOURCE(FemGui::ViewProviderShapeExtension, Gui::ViewProviderExtension)
EXTENSION_PROPERTY_SOURCE(FemGui::ViewProviderBoxExtension, FemGui::ViewProviderShapeExtension)
EXTENSION_PROPERTY_SOURCE(FemGui::ViewProviderCylinderExtension, FemGui::ViewProviderShapeExtension)
EXTENSION_PROPERTY_SOURCE(FemGui::ViewProviderSphereExtension, FemGui::ViewProviderShapeExtension)
EXTENSION_PROPERTY_SOURCE(FemGui::ViewProviderPlaneExtension, FemGui::ViewProviderShapeExtension)

namespace Gui
{
EXTENSION_PROPERTY_SOURCE_TEMPLATE(FemGui::ViewProviderBoxExtensionPython, FemGui::ViewProviderBoxExtension)
EXTENSION_PROPERTY_SOURCE_TEMPLATE(
    FemGui::ViewProviderCylinderExtensionPython,
    FemGui::ViewProviderCylinderExtension
)
EXTENSION_PROPERTY_SOURCE_TEMPLATE(
    FemGui::ViewProviderSphereExtensionPython,
    FemGui::ViewProviderSphereExtension
)
EXTENSION_PROPERTY_SOURCE_TEMPLATE(
    FemGui::ViewProviderPlaneExtensionPython,
    FemGui::ViewProviderPlaneExtension
)


// explicit template instantiation
template class FemGuiExport ViewProviderExtensionPythonT<ViewProviderBoxExtension>;
template class FemGuiExport ViewProviderExtensionPythonT<ViewProviderCylinderExtension>;
template class FemGuiExport ViewProviderExtensionPythonT<ViewProviderSphereExtension>;
template class FemGuiExport ViewProviderExtensionPythonT<ViewProviderPlaneExtension>;

}  // namespace Gui


void ShapeWidget::setViewProvider(Gui::ViewProviderDocumentObject* view)
{
    // NOLINTBEGIN
    m_view = view;
    m_object = view->getObject();
    m_connection = m_object->getDocument()->signalChangedObject.connect(
        std::bind(&ShapeWidget::onObjectsChanged, this, sp::_1, sp::_2)
    );
    // NOLINTEND
}

void ShapeWidget::onObjectsChanged(const App::DocumentObject& obj, const App::Property& p)
{
    if (&obj == m_object) {
        onChange(p);
    }
}

ViewProviderShapeExtension::ViewProviderShapeExtension()
{
    initExtensionType(ViewProviderShapeExtension::getExtensionClassTypeId());

    m_geometrySeperator = new SoSeparator();
    m_geometrySeperator->ref();

    m_scale = new SoScale();
    m_scale->ref();
    m_scale->scaleFactor = SbVec3f(1, 1, 1);
}

ViewProviderShapeExtension::~ViewProviderShapeExtension()
{

    m_geometrySeperator->unref();
    m_manip->unref();
    m_scale->unref();
}


void ViewProviderShapeExtension::extensionAttach(App::DocumentObject* pcObj)
{
    ViewProviderExtension::extensionAttach(pcObj);

    // setup the graph for editing the function unit geometry
    SoMaterial* color = new SoMaterial();
    color->diffuseColor.setValue(0, 0, 1);
    color->transparency.setValue(0.5);

    SoTransform* transform = new SoTransform();

    m_manip = setupManipulator();
    m_manip->ref();

    SoSeparator* pcEditNode = new SoSeparator();
    pcEditNode->ref();

    pcEditNode->addChild(color);
    pcEditNode->addChild(transform);
    pcEditNode->addChild(m_geometrySeperator);

    m_geometrySeperator->insertChild(m_scale, 0);

    // Now we replace the SoTransform node by a manipulator
    // Note: Even SoCenterballManip inherits from SoTransform
    // we cannot use it directly (in above code) because the
    // translation and center fields are overridden.
    SoSearchAction sa;
    sa.setInterest(SoSearchAction::FIRST);
    sa.setSearchingAll(FALSE);
    sa.setNode(transform);
    sa.apply(pcEditNode);
    SoPath* path = sa.getPath();
    if (path) {
        m_manip->replaceNode(path);

        SoDragger* dragger = m_manip->getDragger();
        dragger->addStartCallback(dragStartCallback, this);
        dragger->addFinishCallback(dragFinishCallback, this);
        dragger->addMotionCallback(dragMotionCallback, this);
    }

    getExtendedViewProvider()->addDisplayMaskMode(pcEditNode, m_mask_mode.c_str());
    getExtendedViewProvider()->setDisplayMaskMode(m_mask_mode.c_str());
    pcEditNode->unref();
}


void ViewProviderShapeExtension::dragStartCallback(void* data, SoDragger*)
{
    // This is called when a manipulator is about to manipulating
    Gui::Application::Instance->activeDocument()->openCommand(
        QT_TRANSLATE_NOOP("Command", "Edit Shape")
    );

    ViewProviderShapeExtension* that = static_cast<ViewProviderShapeExtension*>(data);
    that->m_isDragging = true;
    that->m_autoRecompute = FemSettings().getPostAutoRecompute();
}

void ViewProviderShapeExtension::dragFinishCallback(void* data, SoDragger*)
{
    // This is called when a manipulator has done manipulating
    Gui::Application::Instance->activeDocument()->commitCommand();

    ViewProviderShapeExtension* that = static_cast<ViewProviderShapeExtension*>(data);
    if (that->m_autoRecompute) {
        that->getExtendedViewProvider()->getDocument()->getDocument()->recompute();
    }

    that->m_isDragging = false;
}

void ViewProviderShapeExtension::dragMotionCallback(void* data, SoDragger* drag)
{
    ViewProviderShapeExtension* that = static_cast<ViewProviderShapeExtension*>(data);
    that->draggerUpdate(drag);

    if (that->m_autoRecompute) {
        that->getExtendedViewProvider()->getDocument()->getDocument()->recompute();
    }
}


SbBox3f ViewProviderShapeExtension::getBoundingsOfView() const
{
    SbBox3f box;
    Gui::Document* doc = getExtendedViewProvider()->getDocument();
    Gui::View3DInventor* view = qobject_cast<Gui::View3DInventor*>(
        doc->getViewOfViewProvider(getExtendedViewProvider())
    );
    if (view) {
        Gui::View3DInventorViewer* viewer = view->getViewer();
        box = viewer->getBoundingBox();
    }

    return box;
}

bool ViewProviderShapeExtension::findScaleFactor(double& scale) const
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

PyObject* ViewProviderShapeExtension::getExtensionPyObject()
{
    if (ExtensionPythonObject.is(Py::_None())) {
        // ref counter is set to 1
        auto ext = new ViewProviderShapeExtensionPy(this);
        ExtensionPythonObject = Py::Object(ext, true);
    }
    return Py::new_reference_to(ExtensionPythonObject);
}


// Box Manipulation
// ****************

BoxWidget::BoxWidget()
    : ui(new Ui_BoxWidget)
{
    ui->setupUi(this);

    QSize size = ui->centerX->sizeForText(QStringLiteral("000000000000"));
    ui->centerX->setMinimumWidth(size.width());
    ui->centerY->setMinimumWidth(size.width());
    ui->centerZ->setMinimumWidth(size.width());
    ui->length->setMinimumWidth(size.width());
    ui->width->setMinimumWidth(size.width());
    ui->height->setMinimumWidth(size.width());

    int UserDecimals = Base::UnitsApi::getDecimals();
    ui->centerX->setDecimals(UserDecimals);
    ui->centerY->setDecimals(UserDecimals);
    ui->centerZ->setDecimals(UserDecimals);
    ui->length->setDecimals(UserDecimals);
    ui->width->setDecimals(UserDecimals);
    ui->height->setDecimals(UserDecimals);

    connect(
        ui->centerX,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        &BoxWidget::centerChanged
    );
    connect(
        ui->centerY,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        &BoxWidget::centerChanged
    );
    connect(
        ui->centerZ,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        &BoxWidget::centerChanged
    );
    connect(
        ui->length,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        &BoxWidget::lengthChanged
    );
    connect(
        ui->width,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        &BoxWidget::widthChanged
    );
    connect(
        ui->height,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        &BoxWidget::heightChanged
    );
}

BoxWidget::~BoxWidget() = default;


void BoxWidget::setViewProvider(Gui::ViewProviderDocumentObject* view)
{
    FemGui::ShapeWidget::setViewProvider(view);
    setBlockObjectUpdates(true);
    Fem::BoxExtension* box = getObjectExtension<Fem::BoxExtension>();
    Base::Unit unit = box->BoxCenter.getUnit();
    ui->centerX->setUnit(unit);
    ui->centerY->setUnit(unit);
    ui->centerZ->setUnit(unit);
    unit = box->BoxLength.getUnit();
    ui->length->setUnit(unit);
    unit = box->BoxWidth.getUnit();
    ui->width->setUnit(unit);
    unit = box->BoxHeight.getUnit();
    ui->height->setUnit(unit);
    setBlockObjectUpdates(false);
    onChange(box->BoxCenter);
    onChange(box->BoxLength);
    onChange(box->BoxWidth);
    onChange(box->BoxHeight);
}

void BoxWidget::onChange(const App::Property& p)
{
    setBlockObjectUpdates(true);
    Fem::BoxExtension* box = getObjectExtension<Fem::BoxExtension>();
    if (&p == &box->BoxCenter) {
        const Base::Vector3d& vec = static_cast<const App::PropertyVector*>(&p)->getValue();
        ui->centerX->setValue(vec.x);
        ui->centerY->setValue(vec.y);
        ui->centerZ->setValue(vec.z);
    }
    else if (&p == &box->BoxLength) {
        double l = static_cast<const App::PropertyDistance*>(&p)->getValue();
        ui->length->setValue(l);
    }
    else if (&p == &box->BoxWidth) {
        double w = static_cast<const App::PropertyDistance*>(&p)->getValue();
        ui->width->setValue(w);
    }
    else if (&p == &box->BoxHeight) {
        double h = static_cast<const App::PropertyDistance*>(&p)->getValue();
        ui->height->setValue(h);
    }
    setBlockObjectUpdates(false);
}

void BoxWidget::centerChanged(double)
{
    if (!blockObjectUpdates()) {
        Base::Vector3d vec(
            ui->centerX->value().getValue(),
            ui->centerY->value().getValue(),
            ui->centerZ->value().getValue()
        );
        getObjectExtension<Fem::BoxExtension>()->BoxCenter.setValue(vec);
    }
}

void BoxWidget::lengthChanged(double)
{
    if (!blockObjectUpdates()) {
        double l = ui->length->value().getValue();
        getObjectExtension<Fem::BoxExtension>()->BoxLength.setValue(l);
    }
}

void BoxWidget::widthChanged(double)
{
    if (!blockObjectUpdates()) {
        double w = ui->width->value().getValue();
        getObjectExtension<Fem::BoxExtension>()->BoxWidth.setValue(w);
    }
}

void BoxWidget::heightChanged(double)
{
    if (!blockObjectUpdates()) {
        double h = ui->height->value().getValue();
        getObjectExtension<Fem::BoxExtension>()->BoxHeight.setValue(h);
    }
}


ViewProviderBoxExtension::ViewProviderBoxExtension()
{
    initExtensionType(ViewProviderBoxExtension::getExtensionClassTypeId());

    // setup the visualisation geometry
    getGeometryNode()->addChild(ShapeNodes::postBox());

    m_mask_mode = "Box";
}

ViewProviderBoxExtension::~ViewProviderBoxExtension() = default;


void ViewProviderBoxExtension::draggerUpdate(SoDragger* m)
{
    Fem::BoxExtension* box = getObjectExtension<Fem::BoxExtension>();
    SoHandleBoxDragger* dragger = static_cast<SoHandleBoxDragger*>(m);

    const SbVec3f& center = dragger->translation.getValue();
    SbVec3f scale = dragger->scaleFactor.getValue();

    box->BoxCenter.setValue(center[0], center[1], center[2]);
    box->BoxLength.setValue(scale[0]);
    box->BoxWidth.setValue(scale[1]);
    box->BoxHeight.setValue(scale[2]);
}

void ViewProviderBoxExtension::extensionUpdateData(const App::Property* p)
{
    Fem::BoxExtension* box = getObjectExtension<Fem::BoxExtension>();
    if (!isDragging()
        && (p == &box->BoxCenter || p == &box->BoxLength || p == &box->BoxWidth
            || p == &box->BoxHeight)) {
        const Base::Vector3d& center = box->BoxCenter.getValue();
        float l = box->BoxLength.getValue();
        float w = box->BoxWidth.getValue();
        float h = box->BoxHeight.getValue();

        SbMatrix s, t;
        s.setScale(SbVec3f(l, w, h));
        t.setTranslate(SbVec3f(center.x, center.y, center.z));
        s.multRight(t);
        getManipulator()->setMatrix(s);
    }
    ViewProviderShapeExtension::extensionUpdateData(p);
}

SoTransformManip* ViewProviderBoxExtension::setupManipulator()
{
    return new SoHandleBoxManip;
}

ShapeWidget* ViewProviderBoxExtension::createShapeWidget()
{
    return new BoxWidget();
}


// Cylinder Manipulation
// *********************

CylinderWidget::CylinderWidget()
    : ui(new Ui_CylinderWidget)
{
    ui->setupUi(this);

    QSize size = ui->centerX->sizeForText(QStringLiteral("000000000000"));
    ui->centerX->setMinimumWidth(size.width());
    ui->centerY->setMinimumWidth(size.width());
    ui->centerZ->setMinimumWidth(size.width());
    ui->axisX->setMinimumWidth(size.width());
    ui->axisY->setMinimumWidth(size.width());
    ui->axisZ->setMinimumWidth(size.width());
    ui->radius->setMinimumWidth(size.width());

    int UserDecimals = Base::UnitsApi::getDecimals();
    ui->centerX->setDecimals(UserDecimals);
    ui->centerY->setDecimals(UserDecimals);
    ui->centerZ->setDecimals(UserDecimals);
    ui->axisX->setDecimals(UserDecimals);
    ui->axisY->setDecimals(UserDecimals);
    ui->axisZ->setDecimals(UserDecimals);

    connect(
        ui->centerX,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        &CylinderWidget::centerChanged
    );
    connect(
        ui->centerY,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        &CylinderWidget::centerChanged
    );
    connect(
        ui->centerZ,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        &CylinderWidget::centerChanged
    );
    connect(
        ui->axisX,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        &CylinderWidget::axisChanged
    );
    connect(
        ui->axisY,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        &CylinderWidget::axisChanged
    );
    connect(
        ui->axisZ,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        &CylinderWidget::axisChanged
    );
    connect(
        ui->radius,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        &CylinderWidget::radiusChanged
    );
}

CylinderWidget::~CylinderWidget() = default;

void CylinderWidget::setViewProvider(Gui::ViewProviderDocumentObject* view)
{
    FemGui::ShapeWidget::setViewProvider(view);
    setBlockObjectUpdates(true);
    Fem::CylinderExtension* cyl = getObjectExtension<Fem::CylinderExtension>();
    Base::Unit unit = cyl->CylinderCenter.getUnit();
    ui->centerX->setUnit(unit);
    ui->centerY->setUnit(unit);
    ui->centerZ->setUnit(unit);
    unit = cyl->CylinderRadius.getUnit();
    ui->radius->setUnit(unit);
    setBlockObjectUpdates(false);
    onChange(cyl->CylinderCenter);
    onChange(cyl->CylinderRadius);
    onChange(cyl->CylinderAxis);
}

void CylinderWidget::onChange(const App::Property& p)
{
    setBlockObjectUpdates(true);
    Fem::CylinderExtension* cyl = getObjectExtension<Fem::CylinderExtension>();
    if (&p == &cyl->CylinderAxis) {
        const Base::Vector3d& vec = static_cast<const App::PropertyVector*>(&p)->getValue();
        ui->axisX->setValue(vec.x);
        ui->axisY->setValue(vec.y);
        ui->axisZ->setValue(vec.z);
    }
    else if (&p == &cyl->CylinderCenter) {
        const Base::Vector3d& vec = static_cast<const App::PropertyVectorDistance*>(&p)->getValue();
        ui->centerX->setValue(vec.x);
        ui->centerY->setValue(vec.y);
        ui->centerZ->setValue(vec.z);
    }
    else if (&p == &cyl->CylinderRadius) {
        double val = static_cast<const App::PropertyDistance*>(&p)->getValue();
        ui->radius->setValue(val);
    }
    setBlockObjectUpdates(false);
}

void CylinderWidget::centerChanged(double)
{
    if (!blockObjectUpdates()) {
        Base::Vector3d vec(
            ui->centerX->value().getValue(),
            ui->centerY->value().getValue(),
            ui->centerZ->value().getValue()
        );
        getObjectExtension<Fem::CylinderExtension>()->CylinderCenter.setValue(vec);
    }
}

void CylinderWidget::axisChanged(double)
{
    if (!blockObjectUpdates()) {
        Base::Vector3d vec(
            ui->axisX->value().getValue(),
            ui->axisY->value().getValue(),
            ui->axisZ->value().getValue()
        );
        getObjectExtension<Fem::CylinderExtension>()->CylinderAxis.setValue(vec);
    }
}

void CylinderWidget::radiusChanged(double)
{
    if (!blockObjectUpdates()) {
        getObjectExtension<Fem::CylinderExtension>()->CylinderRadius.setValue(
            ui->radius->value().getValue()
        );
    }
}

ViewProviderCylinderExtension::ViewProviderCylinderExtension()
{
    initExtensionType(ViewProviderCylinderExtension::getExtensionClassTypeId());

    // setup the visualisation geometry
    getGeometryNode()->addChild(ShapeNodes::postCylinder());

    m_mask_mode = "Cylinder";
}

ViewProviderCylinderExtension::~ViewProviderCylinderExtension() = default;

void ViewProviderCylinderExtension::draggerUpdate(SoDragger* m)
{
    Fem::CylinderExtension* cyl = getObjectExtension<Fem::CylinderExtension>();
    SoJackDragger* dragger = static_cast<SoJackDragger*>(m);
    const SbVec3f& center = dragger->translation.getValue();
    SbVec3f norm(0, 0, 1);
    dragger->rotation.getValue().multVec(norm, norm);
    cyl->CylinderCenter.setValue(center[0], center[1], center[2]);
    cyl->CylinderRadius.setValue(dragger->scaleFactor.getValue()[0]);
    cyl->CylinderAxis.setValue(norm[0], norm[1], norm[2]);
}

void ViewProviderCylinderExtension::extensionUpdateData(const App::Property* p)
{
    Fem::CylinderExtension* cyl = getObjectExtension<Fem::CylinderExtension>();
    if (!isDragging()
        && (p == &cyl->CylinderCenter || p == &cyl->CylinderRadius || p == &cyl->CylinderAxis)) {
        Base::Vector3d trans = cyl->CylinderCenter.getValue();
        Base::Vector3d axis = cyl->CylinderAxis.getValue();
        double radius = cyl->CylinderRadius.getValue();

        SbMatrix translate;
        SbRotation rot(SbVec3f(0.0, 0.0, 1.0), SbVec3f(axis.x, axis.y, axis.z));
        translate.setTransform(SbVec3f(trans.x, trans.y, trans.z), rot, SbVec3f(radius, radius, radius));
        getManipulator()->setMatrix(translate);
    }

    ViewProviderShapeExtension::extensionUpdateData(p);
}

SoTransformManip* ViewProviderCylinderExtension::setupManipulator()
{
    return new SoJackManip;
}

ShapeWidget* ViewProviderCylinderExtension::createShapeWidget()
{
    return new CylinderWidget();
}


// Sphere Manipulation
// *********************

SphereWidget::SphereWidget()
    : ui(new Ui_SphereWidget)
{
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

    connect(
        ui->centerX,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        &SphereWidget::centerChanged
    );
    connect(
        ui->centerY,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        &SphereWidget::centerChanged
    );
    connect(
        ui->centerZ,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        &SphereWidget::centerChanged
    );
    connect(
        ui->radius,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        &SphereWidget::radiusChanged
    );
}

SphereWidget::~SphereWidget() = default;

void SphereWidget::setViewProvider(Gui::ViewProviderDocumentObject* view)
{
    FemGui::ShapeWidget::setViewProvider(view);
    setBlockObjectUpdates(true);
    Fem::SphereExtension* sph = getObjectExtension<Fem::SphereExtension>();
    Base::Unit unit = sph->SphereCenter.getUnit();
    ui->centerX->setUnit(unit);
    ui->centerY->setUnit(unit);
    ui->centerZ->setUnit(unit);
    unit = sph->SphereRadius.getUnit();
    ui->radius->setUnit(unit);
    setBlockObjectUpdates(false);
    onChange(sph->SphereCenter);
    onChange(sph->SphereRadius);
}

void SphereWidget::onChange(const App::Property& p)
{
    setBlockObjectUpdates(true);
    Fem::SphereExtension* sph = getObjectExtension<Fem::SphereExtension>();
    if (&p == &sph->SphereRadius) {
        double val = static_cast<const App::PropertyDistance*>(&p)->getValue();
        ui->radius->setValue(val);
    }
    else if (&p == &sph->SphereCenter) {
        const Base::Vector3d& vec = static_cast<const App::PropertyVectorDistance*>(&p)->getValue();
        ui->centerX->setValue(vec.x);
        ui->centerY->setValue(vec.y);
        ui->centerZ->setValue(vec.z);
    }
    setBlockObjectUpdates(false);
}

void SphereWidget::centerChanged(double)
{
    if (!blockObjectUpdates()) {
        Base::Vector3d vec(
            ui->centerX->value().getValue(),
            ui->centerY->value().getValue(),
            ui->centerZ->value().getValue()
        );
        getObjectExtension<Fem::SphereExtension>()->SphereCenter.setValue(vec);
    }
}

void SphereWidget::radiusChanged(double)
{
    if (!blockObjectUpdates()) {
        getObjectExtension<Fem::SphereExtension>()->SphereRadius.setValue(
            ui->radius->value().getValue()
        );
    }
}


ViewProviderSphereExtension::ViewProviderSphereExtension()
{
    initExtensionType(ViewProviderSphereExtension::getExtensionClassTypeId());

    // setup the visualisation geometry
    getGeometryNode()->addChild(ShapeNodes::postSphere());

    m_mask_mode = "Sphere";
}

ViewProviderSphereExtension::~ViewProviderSphereExtension() = default;

SoTransformManip* ViewProviderSphereExtension::setupManipulator()
{
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

void ViewProviderSphereExtension::draggerUpdate(SoDragger* m)
{
    Fem::SphereExtension* sph = getObjectExtension<Fem::SphereExtension>();
    SoHandleBoxDragger* dragger = static_cast<SoHandleBoxDragger*>(m);

    // the new axis of the plane
    SbRotation rot, scaleDir;
    const SbVec3f& center = dragger->translation.getValue();

    SbVec3f norm(0, 0, 1);
    sph->SphereCenter.setValue(center[0], center[1], center[2]);
    sph->SphereRadius.setValue(dragger->scaleFactor.getValue()[0]);
}

void ViewProviderSphereExtension::extensionUpdateData(const App::Property* p)
{
    Fem::SphereExtension* sph = getObjectExtension<Fem::SphereExtension>();

    if (!isDragging() && (p == &sph->SphereCenter || p == &sph->SphereRadius)) {

        Base::Vector3d trans = sph->SphereCenter.getValue();
        double radius = sph->SphereRadius.getValue();

        SbMatrix t, translate;
        t.setScale(radius);
        translate.setTranslate(SbVec3f(trans.x, trans.y, trans.z));
        t.multRight(translate);
        getManipulator()->setMatrix(t);
    }
    ViewProviderShapeExtension::extensionUpdateData(p);
}

ShapeWidget* ViewProviderSphereExtension::createShapeWidget()
{
    return new SphereWidget();
}


// Plane Manipulation
// *********************

PlaneWidget::PlaneWidget()
    : ui(new Ui_PlaneWidget)
{
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

    connect(
        ui->originX,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        &PlaneWidget::originChanged
    );
    connect(
        ui->originY,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        &PlaneWidget::originChanged
    );
    connect(
        ui->originZ,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        &PlaneWidget::originChanged
    );
    connect(
        ui->normalX,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        &PlaneWidget::normalChanged
    );
    connect(
        ui->normalY,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        &PlaneWidget::normalChanged
    );
    connect(
        ui->normalZ,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        &PlaneWidget::normalChanged
    );
}

PlaneWidget::~PlaneWidget() = default;

void PlaneWidget::setViewProvider(Gui::ViewProviderDocumentObject* view)
{
    FemGui::ShapeWidget::setViewProvider(view);
    Fem::PlaneExtension* pln = getObjectExtension<Fem::PlaneExtension>();
    const Base::Unit unit = pln->PlaneOrigin.getUnit();
    setBlockObjectUpdates(true);
    ui->originX->setUnit(unit);
    ui->originY->setUnit(unit);
    ui->originZ->setUnit(unit);
    setBlockObjectUpdates(false);
    // The normal vector is unitless. It uses nevertheless Gui::PrefQuantitySpinBox to keep dialog
    // uniform.
    onChange(pln->PlaneNormal);
    onChange(pln->PlaneOrigin);
}

void PlaneWidget::onChange(const App::Property& p)
{
    setBlockObjectUpdates(true);
    Fem::PlaneExtension* pln = getObjectExtension<Fem::PlaneExtension>();
    if (&p == &pln->PlaneNormal) {
        const Base::Vector3d& vec = static_cast<const App::PropertyVector*>(&p)->getValue();
        ui->normalX->setValue(vec.x);
        ui->normalY->setValue(vec.y);
        ui->normalZ->setValue(vec.z);
    }
    else if (&p == &pln->PlaneOrigin) {
        const Base::Vector3d& vec = static_cast<const App::PropertyVectorDistance*>(&p)->getValue();
        ui->originX->setValue(vec.x);
        ui->originY->setValue(vec.y);
        ui->originZ->setValue(vec.z);
    }
    setBlockObjectUpdates(false);
}

void PlaneWidget::normalChanged(double)
{
    if (!blockObjectUpdates()) {
        Base::Vector3d vec(
            ui->normalX->value().getValue(),
            ui->normalY->value().getValue(),
            ui->normalZ->value().getValue()
        );
        getObjectExtension<Fem::PlaneExtension>()->PlaneNormal.setValue(vec);
    }
}

void PlaneWidget::originChanged(double)
{
    if (!blockObjectUpdates()) {
        Base::Vector3d vec(
            ui->originX->value().getValue(),
            ui->originY->value().getValue(),
            ui->originZ->value().getValue()
        );
        getObjectExtension<Fem::PlaneExtension>()->PlaneOrigin.setValue(vec);
    }
}


// NOTE: The technical lower limit is at 1e-4 that the Coin3D manipulator can handle
static const App::PropertyFloatConstraint::Constraints scaleConstraint
    = {1e-4, std::numeric_limits<double>::max(), 1.0};

ViewProviderPlaneExtension::ViewProviderPlaneExtension()
{
    EXTENSION_ADD_PROPERTY_TYPE(
        Scale,
        (10),
        "Manipulator",
        App::Prop_None,
        "Scaling factor for the manipulator"
    );

    Scale.setConstraints(&scaleConstraint);

    initExtensionType(ViewProviderPlaneExtension::getExtensionClassTypeId());

    // setup the visualisation geometry
    getGeometryNode()->addChild(ShapeNodes::postPlane());

    m_mask_mode = "Plane";
}

ViewProviderPlaneExtension::~ViewProviderPlaneExtension() = default;

void ViewProviderPlaneExtension::draggerUpdate(SoDragger* m)
{
    Fem::PlaneExtension* pln = getObjectExtension<Fem::PlaneExtension>();
    SoJackDragger* dragger = static_cast<SoJackDragger*>(m);

    // the new axis of the plane
    const SbVec3f& base = dragger->translation.getValue();
    const SbVec3f& scale = dragger->scaleFactor.getValue();

    SbVec3f norm(0.0, 0.0, 1.0);
    dragger->rotation.getValue().multVec(norm, norm);
    pln->PlaneOrigin.setValue(base[0], base[1], base[2]);
    pln->PlaneNormal.setValue(norm[0], norm[1], norm[2]);
    this->Scale.setValue(scale[0]);
}

void ViewProviderPlaneExtension::extensionOnChanged(const App::Property* prop)
{
    if (prop == &Scale && m_detectscale) {
        // Scale property detected at restore
        m_detectscale = false;
    }

    if (prop == &Scale && !isDragging()) {
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

    ViewProviderShapeExtension::extensionOnChanged(prop);
}

void ViewProviderPlaneExtension::extensionUpdateData(const App::Property* p)
{
    Fem::PlaneExtension* pln = getObjectExtension<Fem::PlaneExtension>();

    if (!isDragging() && (p == &pln->PlaneOrigin || p == &pln->PlaneNormal)) {
        // Auto-scale from geometry size at restore
        if (m_detectscale) {
            double s;
            bool find = findScaleFactor(s);
            if (find) {
                Scale.setValue(s);
                m_detectscale = false;
            }
        }
        Base::Vector3d trans = pln->PlaneOrigin.getValue();
        Base::Vector3d norm = pln->PlaneNormal.getValue();

        norm.Normalize();
        SbRotation rot(SbVec3f(0.0, 0.0, 1.0), SbVec3f(norm.x, norm.y, norm.z));
        float scale = static_cast<float>(Scale.getValue());

        SbMatrix mat;
        mat.setTransform(SbVec3f(trans.x, trans.y, trans.z), rot, SbVec3f(scale, scale, scale));
        getManipulator()->setMatrix(mat);
    }

    ViewProviderShapeExtension::extensionUpdateData(p);
}

SoTransformManip* ViewProviderPlaneExtension::setupManipulator()
{
    auto manip = new SoJackManip;

    // Set axis translator to Z-axis
    SoNode* trans = manip->getDragger()->getPart("translator", false);
    static_cast<SoDragPointDragger*>(trans)->showNextDraggerSet();

    return manip;
}


ShapeWidget* ViewProviderPlaneExtension::createShapeWidget()
{
    return new PlaneWidget();
}


namespace FemGui
{

namespace ShapeNodes
{

SoGroup* postBox()
{
    SoCoordinate3* points = new SoCoordinate3();
    points->point.setNum(18);
    points->point.set1Value(0, -0.5, -0.5, -0.5);
    points->point.set1Value(1, 0.5, -0.5, -0.5);
    points->point.set1Value(2, 0.5, 0.5, -0.5);
    points->point.set1Value(3, -0.5, 0.5, -0.5);
    points->point.set1Value(4, -0.5, -0.5, -0.5);
    points->point.set1Value(5, -0.5, -0.5, 0.5);
    points->point.set1Value(6, 0.5, -0.5, 0.5);
    points->point.set1Value(7, 0.5, 0.5, 0.5);
    points->point.set1Value(8, -0.5, 0.5, 0.5);
    points->point.set1Value(9, -0.5, -0.5, 0.5);
    points->point.set1Value(10, -0.5, -0.5, -0.5);
    points->point.set1Value(11, -0.5, -0.5, 0.5);
    points->point.set1Value(12, 0.5, -0.5, -0.5);
    points->point.set1Value(13, 0.5, -0.5, 0.5);
    points->point.set1Value(14, 0.5, 0.5, -0.5);
    points->point.set1Value(15, 0.5, 0.5, 0.5);
    points->point.set1Value(16, -0.5, 0.5, -0.5);
    points->point.set1Value(17, -0.5, 0.5, 0.5);

    int vert[6];
    vert[0] = 5;
    vert[1] = 5;
    vert[2] = 2;
    vert[3] = 2;
    vert[4] = 2;
    vert[5] = 2;

    SoGroup* group = new SoGroup();
    SoLineSet* line = new SoLineSet();

    line->numVertices.setValues(0, 6, vert);

    group->addChild(points);
    group->addChild(line);

    return group;
}


SoGroup* postCylinder()
{
    using std::numbers::pi;

    SoCoordinate3* points = new SoCoordinate3();
    int nCirc = 20;
    const int nSide = 8;
    float h = 3.0;
    points->point.setNum(2 * (nCirc + 1 + nSide));

    int idx = 0;
    // top and bottom
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < nCirc + 1; ++j) {
            points->point.set1Value(
                idx,
                SbVec3f(std::cos(2 * pi / nCirc * j), std::sin(2 * pi / nCirc * j), -h / 2. + h * i)
            );
            ++idx;
        }
    }
    // sides
    for (int i = 0; i < nSide; ++i) {
        for (int j = 0; j < 2; ++j) {
            points->point.set1Value(
                idx,
                SbVec3f(std::cos(2 * pi / nSide * i), std::sin(2 * pi / nSide * i), -h / 2. + h * j)
            );
            ++idx;
        }
    }
    // numVertices
    int vert[nSide + 2] {};
    vert[0] = nCirc + 1;
    vert[1] = nCirc + 1;
    for (int i = 0; i < nSide; ++i) {
        vert[i + 2] = 2;
    }

    SoLineSet* line = new SoLineSet();
    SoGroup* group = new SoGroup();
    line->numVertices.setValues(0, nSide + 2, vert);

    group->addChild(points);
    group->addChild(line);

    return group;
}


SoGroup* postPlane()
{
    SoCoordinate3* points = new SoCoordinate3();
    points->point.setNum(4);
    points->point.set1Value(0, -0.5, -0.5, 0.0);
    points->point.set1Value(1, -0.5, 0.5, 0.0);
    points->point.set1Value(2, 0.5, 0.5, 0.0);
    points->point.set1Value(3, 0.5, -0.5, 0.0);
    points->point.set1Value(4, -0.5, -0.5, 0.0);
    SoGroup* group = new SoGroup();
    SoLineSet* line = new SoLineSet();

    group->addChild(points);
    group->addChild(line);

    return group;
}

SoGroup* postSphere()
{
    using std::numbers::pi;

    SoCoordinate3* points = new SoCoordinate3();
    points->point.setNum(2 * 84);
    int idx = 0;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 21; j++) {
            points->point.set1Value(
                idx,
                SbVec3f(
                    std::sin(2 * pi / 20 * j) * std::cos(pi / 4 * i),
                    std::sin(2 * pi / 20 * j) * std::sin(pi / 4 * i),
                    std::cos(2 * pi / 20 * j)
                )
            );
            ++idx;
        }
    }
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 21; j++) {
            points->point.set1Value(
                idx,
                SbVec3f(
                    std::sin(pi / 4 * i) * std::cos(2 * pi / 20 * j),
                    std::sin(pi / 4 * i) * std::sin(2 * pi / 20 * j),
                    std::cos(pi / 4 * i)
                )
            );
            ++idx;
        }
    }

    SoGroup* group = new SoGroup();
    SoLineSet* line = new SoLineSet();

    group->addChild(points);
    group->addChild(line);

    return group;
}


}  // namespace ShapeNodes
}  // namespace FemGui
