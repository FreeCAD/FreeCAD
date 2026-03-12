// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Sayantan Deb <sayantandebin[at]gmail.com>           *
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

#include "Gizmo.h"

#include <cmath>

#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <algorithm>

#include <App/Application.h>
#include <Base/Converter.h>
#include <Base/Parameter.h>
#include <Base/Precision.h>
#include "Base/ServiceProvider.h"
#include <Base/Tools.h>
#include <Document.h>
#include <Gui/Inventor/Draggers/GizmoStyleParameters.h>
#include <Gui/Inventor/So3DAnnotation.h>
#include <Gui/Inventor/SoToggleSwitch.h>
#include <Gui/ViewProviderDragger.h>
#include <Gui/QuantitySpinBox.h>
#include <Gui/Utilities.h>
#include <Gui/View3DInventorViewer.h>

#include "SoLinearDragger.h"
#include "SoLinearDraggerGeometry.h"
#include "SoRotationDragger.h"
#include "SoRotationDraggerGeometry.h"

using namespace Gui;

void Gizmo::setDraggerPlacement(const Base::Vector3d& pos, const Base::Vector3d& dir)
{
    setDraggerPlacement(Base::convertTo<SbVec3f>(pos), Base::convertTo<SbVec3f>(dir));
}

bool Gizmo::isDelayedUpdateEnabled()
{
    static Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().GetGroup(
        "BaseApp/Preferences/Gui/Gizmos"
    );

    return hGrp->GetBool("DelayedGizmoUpdate", false);
}

double Gizmo::getMultFactor()
{
    return multFactor;
}

double Gizmo::getAddFactor()
{
    return addFactor;
}

bool Gizmo::getVisibility()
{
    return visible;
}

LinearGizmo::LinearGizmo(QuantitySpinBox* property)
{
    this->property = property;
}

SoInteractionKit* LinearGizmo::initDragger()
{
    draggerContainer = new SoLinearDraggerContainer;
    draggerContainer->color.setValue(1, 0, 0);
    dragger = draggerContainer->getDragger();

    dragger->addStartCallback(
        [](void* data, SoDragger*) { static_cast<LinearGizmo*>(data)->draggingStarted(); },
        this
    );
    dragger->addFinishCallback(
        [](void* data, SoDragger*) { static_cast<LinearGizmo*>(data)->draggingFinished(); },
        this
    );
    dragger->addMotionCallback(
        [](void* data, SoDragger*) { static_cast<LinearGizmo*>(data)->draggingContinued(); },
        this
    );

    dragger->labelVisible = false;

    dragger->instantiateBaseGeometry();

    // change the dragger dimensions
    auto arrow = SO_GET_PART(dragger, "arrow", SoArrowGeometry);
    arrow->cylinderHeight = 3.5;
    arrow->cylinderRadius = 0.2;

    updateColorTheme();

    setProperty(property);

    return draggerContainer;
}

void LinearGizmo::uninitDragger()
{
    dragger = nullptr;
    draggerContainer = nullptr;
}

void LinearGizmo::updateColorTheme()
{
    auto* styleParameterManager = Base::provideService<Gui::StyleParameters::ParameterManager>();
    Base::Color baseColor = styleParameterManager->resolve(StyleParameters::LinearGizmoBaseColor);
    Base::Color activeColor = styleParameterManager->resolve(StyleParameters::LinearGizmoActiveColor);

    dragger->color = baseColor.asValue<SbColor>();
    dragger->activeColor = activeColor.asValue<SbColor>();

    auto baseGeom = SO_GET_PART(dragger, "baseGeom", SoArrowBase);
    Base::Color baseGeomColor = styleParameterManager->resolve(
        StyleParameters::DimensionVisualizerColor
    );
    baseGeom->color = baseGeomColor.asValue<SbColor>();
}

GizmoPlacement LinearGizmo::getDraggerPlacement()
{
    assert(draggerContainer && "Forgot to call GizmoContainer::initGizmos?");
    return {draggerContainer->translation.getValue(), draggerContainer->getPointerDirection()};
}

void LinearGizmo::setDraggerPlacement(const SbVec3f& pos, const SbVec3f& dir)
{
    assert(draggerContainer && "Forgot to call GizmoContainer::initGizmos?");
    draggerContainer->translation = pos;
    draggerContainer->setPointerDirection(dir);
}

void LinearGizmo::reverseDir()
{
    auto dir = getDraggerContainer()->getPointerDirection();
    getDraggerContainer()->setPointerDirection(dir * -1);
}

double LinearGizmo::getDragLength()
{
    double dragLength = dragger->translationIncrementCount.getValue()
        * dragger->translationIncrement.getValue();

    return (dragLength - addFactor) / multFactor;
}

void LinearGizmo::setDragLength(double dragLength)
{
    dragLength = dragLength * multFactor + addFactor;
    dragger->translation = {0, static_cast<float>(dragLength), 0};
}

void LinearGizmo::setGeometryScale(float scale)
{
    dragger->geometryScale = SbVec3f(scale, scale, scale);
    // Scales the dragger increment in exponents of 10 based on the zoom level (scale)
    constexpr float base = 10.0F;
    dragger->translationIncrement = multFactor * std::pow(base, std::floor(std::log10(scale)));
}

SoLinearDraggerContainer* LinearGizmo::getDraggerContainer()
{
    assert(draggerContainer && "Forgot to call GizmoContainer::initGizmos?");
    return draggerContainer;
}

void LinearGizmo::setProperty(QuantitySpinBox* property)
{
    QuantitySpinBox::disconnect(quantityChangedConnection);
    QuantitySpinBox::disconnect(formulaDialogConnection);

    this->property = property;
    quantityChangedConnection = QuantitySpinBox::connect(
        property,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        [this](double value) { setDragLength(value); }
    );
    formulaDialogConnection
        = QuantitySpinBox::connect(property, &Gui::QuantitySpinBox::showFormulaDialog, [this](bool) {
              // This will set the visibility of the actual geometry to true or false
              // based on if an expression is bound and the externally set visibility
              setVisibility(visible);
          });

    // Updates the gizmo state based on the new property
    setDragLength(property->rawValue());
    setVisibility(visible);
}

void LinearGizmo::setMultFactor(const double val)
{
    multFactor = val;
    setDragLength(property->value().getValue());
}

void LinearGizmo::setAddFactor(const double val)
{
    addFactor = val;
    setDragLength(property->value().getValue());
}

void LinearGizmo::setVisibility(bool visible)
{
    this->visible = visible;
    getDraggerContainer()->visible = visible && !property->hasExpression();
}

void LinearGizmo::draggingStarted()
{
    initialValue = property->value().getValue();
    dragger->translationIncrementCount.setValue(0);

    if (isDelayedUpdateEnabled()) {
        property->blockSignals(true);
    }
}

void LinearGizmo::draggingFinished()
{
    if (isDelayedUpdateEnabled()) {
        property->blockSignals(false);
        property->valueChanged(property->value().getValue());
    }

    property->setFocus();
    property->selectAll();
}

void LinearGizmo::draggingContinued()
{
    double value = initialValue + getDragLength();
    // TODO: Need to change the lower limit to sudoThis->property->minimum() once the
    // two direction extrude work gets merged
    value = std::clamp(value, dragger->translationIncrement.getValue(), property->maximum());

    property->setValue(value);
    setDragLength(value);
}


RotationGizmo::RotationGizmo(QuantitySpinBox* property)
{
    this->property = property;
}

RotationGizmo::~RotationGizmo()
{
    translationSensor.detach();
    translationSensor.setData(nullptr);
    translationSensor.setFunction(nullptr);
}

SoInteractionKit* RotationGizmo::initDragger()
{
    multFactor = std::numbers::pi_v<float> / 180.0;
    draggerContainer = new SoRotationDraggerContainer;

    draggerContainer->color.setValue(1, 0, 0);
    dragger = draggerContainer->getDragger();
    dragger->rotationIncrement = std::numbers::pi / 90.0;

    auto rotator = new SoRotatorGeometry;
    rotator->arcAngle = std::numbers::pi_v<float> / 6.0f;
    rotator->arcRadius = 16.0f;
    dragger->setPart("rotator", rotator);

    dragger->addStartCallback(
        [](void* data, SoDragger*) { static_cast<RotationGizmo*>(data)->draggingStarted(); },
        this
    );
    dragger->addFinishCallback(
        [](void* data, SoDragger*) { static_cast<RotationGizmo*>(data)->draggingFinished(); },
        this
    );
    dragger->addMotionCallback(
        [](void* data, SoDragger*) { static_cast<RotationGizmo*>(data)->draggingContinued(); },
        this
    );

    setProperty(property);

    updateColorTheme();

    return draggerContainer;
}

void RotationGizmo::uninitDragger()
{
    dragger = nullptr;
    draggerContainer = nullptr;

    translationSensor.detach();
    translationSensor.setData(nullptr);
    translationSensor.setFunction(nullptr);
}

void RotationGizmo::updateColorTheme()
{
    auto* styleParameterManager = Base::provideService<Gui::StyleParameters::ParameterManager>();
    Base::Color baseColor = styleParameterManager->resolve(StyleParameters::RotationGizmoBaseColor);
    Base::Color activeColor = styleParameterManager->resolve(StyleParameters::RotationGizmoActiveColor);

    dragger->color = baseColor.asValue<SbColor>();
    dragger->activeColor = activeColor.asValue<SbColor>();
}

GizmoPlacement RotationGizmo::getDraggerPlacement()
{
    assert(draggerContainer && "Forgot to call GizmoContainer::initGizmos?");
    return {draggerContainer->translation.getValue(), draggerContainer->getPointerDirection()};
}

void RotationGizmo::setDraggerPlacement(const SbVec3f& pos, const SbVec3f& dir)
{
    assert(draggerContainer && "Forgot to call GizmoContainer::initGizmos?");
    draggerContainer->translation = pos;
    draggerContainer->setPointerDirection(dir);
}

void RotationGizmo::reverseDir()
{
    auto dir = getDraggerContainer()->getPointerDirection();
    getDraggerContainer()->setPointerDirection(dir * -1);
}

void RotationGizmo::placeOverLinearGizmo(LinearGizmo* gizmo)
{
    linearGizmo = gizmo;

    GizmoPlacement placement = gizmo->getDraggerPlacement();

    draggerContainer->translation = Base::convertTo<SbVec3f>(placement.pos);
    draggerContainer->setPointerDirection(placement.dir);

    translationSensor.setData(this);
    translationSensor.setFunction(translationSensorCB);
    translationSensor.setPriority(0);
    SoSFVec3f& translation = gizmo->getDraggerContainer()->getDragger()->translation;
    translationSensor.attach(&translation);
    translation.touch();

    automaticOrientation = true;
}

void RotationGizmo::translationSensorCB(void* data, SoSensor* sensor)
{
    auto sudoThis = static_cast<RotationGizmo*>(data);
    auto translationSensor = static_cast<SoFieldSensor*>(sensor);

    GizmoPlacement placement = sudoThis->linearGizmo->getDraggerPlacement();

    SbVec3f translation = static_cast<SoSFVec3f*>(translationSensor->getAttachedField())->getValue();
    float yComp = translation.getValue()[1];
    SbVec3f dir = placement.dir;
    dir.normalize();
    sudoThis->draggerContainer->translation = placement.pos + dir * (yComp + sudoThis->sepDistance);
}

void RotationGizmo::placeBelowLinearGizmo(LinearGizmo* gizmo)
{
    linearGizmo = gizmo;

    GizmoPlacement placement = gizmo->getDraggerPlacement();

    draggerContainer->translation = Base::convertTo<SbVec3f>(placement.pos);
    draggerContainer->setPointerDirection(-placement.dir);

    translationSensor.setData(this);
    translationSensor.setFunction(translationSensorCB);
    translationSensor.setPriority(0);
    SoSFVec3f& translation = gizmo->getDraggerContainer()->getDragger()->translation;
    translationSensor.attach(&translation);
    translation.touch();
}

double RotationGizmo::getRotAngle()
{
    double rotAngle = dragger->rotationIncrementCount.getValue()
        * dragger->rotationIncrement.getValue();

    return (rotAngle - addFactor) / multFactor;
}

void RotationGizmo::setRotAngle(double angle)
{
    angle = multFactor * angle + addFactor;
    dragger->rotation = SbRotation({0, 0, 1.0f}, static_cast<float>(angle));
}

void RotationGizmo::setGeometryScale(float scale)
{
    dragger->geometryScale = SbVec3f(scale, scale, scale);
}

SoRotationDraggerContainer* RotationGizmo::getDraggerContainer()
{
    assert(draggerContainer && "Forgot to call GizmoContainer::initGizmos?");
    return draggerContainer;
}

void RotationGizmo::draggingStarted()
{
    initialValue = property->value().getValue();
    dragger->rotationIncrementCount.setValue(0);

    if (isDelayedUpdateEnabled()) {
        property->blockSignals(true);
    }
}

void RotationGizmo::draggingFinished()
{
    if (isDelayedUpdateEnabled()) {
        property->blockSignals(false);
        property->valueChanged(property->value().getValue());
    }

    property->setFocus();
    property->selectAll();
}

void RotationGizmo::draggingContinued()
{
    double value = initialValue + getRotAngle();
    value = Base::clampAngle(
        value,
        property->minimum(),
        property->maximum(),
        Base::Precision::Confusion()
    );

    property->setValue(value);
    setRotAngle(value);
}

void RotationGizmo::orientAlongCamera(SoCamera* camera)
{
    if (!automaticOrientation) {
        return;
    }

    SbVec3f cameraDir {0, 0, 1};
    camera->orientation.getValue().multVec(cameraDir, cameraDir);
    SbVec3f pointerDir = getDraggerContainer()->getPointerDirection();

    pointerDir.normalize();
    auto proj = cameraDir - cameraDir.dot(pointerDir) * pointerDir;
    if (proj.equals(SbVec3f {0, 0, 0}, 0.001)) {
        return;
    }

    assert(draggerContainer && "Forgot to call GizmoContainer::initGizmos?");
    draggerContainer->setArcNormalDirection(proj);
}

void RotationGizmo::setProperty(QuantitySpinBox* property)
{
    QuantitySpinBox::disconnect(quantityChangedConnection);
    QuantitySpinBox::disconnect(formulaDialogConnection);

    this->property = property;
    quantityChangedConnection = QuantitySpinBox::connect(
        property,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        [this](double value) { setRotAngle(value); }
    );
    formulaDialogConnection
        = QuantitySpinBox::connect(property, &Gui::QuantitySpinBox::showFormulaDialog, [this](bool) {
              // This will set the visibility of the actual geometry to true or false
              // based on if an expression is bound and the externally set visibility
              setVisibility(visible);
          });

    // Updates the gizmo state based on the new property
    setRotAngle(property->rawValue());
    setVisibility(visible);
}

void RotationGizmo::setMultFactor(const double val)
{
    multFactor = val;
    setRotAngle(property->value().getValue());
}

void RotationGizmo::setAddFactor(const double val)
{
    addFactor = val;
    setRotAngle(property->value().getValue());
}

void RotationGizmo::setVisibility(bool visible)
{
    this->visible = visible;
    getDraggerContainer()->visible = visible && !property->hasExpression();
}

DirectedRotationGizmo::DirectedRotationGizmo(QuantitySpinBox* property)
    : RotationGizmo(property)
{}

SoInteractionKit* DirectedRotationGizmo::initDragger()
{
    SoInteractionKit* ret = inherited::initDragger();

    auto dragger = getDraggerContainer()->getDragger();
    auto rotator = new SoRotatorGeometry2;
    rotator->arcAngle = std::numbers::pi_v<float> / 6.0f;
    rotator->arcRadius = 16.0f;
    rotator->rightArrowVisible = false;
    dragger->setPart("rotator", rotator);

    updateColorTheme();

    return ret;
}

void DirectedRotationGizmo::flipArrow()
{
    auto dragger = getDraggerContainer()->getDragger();
    auto rotator = SO_GET_PART(dragger, "rotator", SoRotatorGeometry2);

    rotator->toggleArrowVisibility();
}


RadialGizmo::RadialGizmo(QuantitySpinBox* property)
    : RotationGizmo(property)
{}

SoInteractionKit* RadialGizmo::initDragger()
{
    SoInteractionKit* ret = inherited::initDragger();

    auto dragger = getDraggerContainer()->getDragger();
    auto rotator = new SoRotatorArrow;
    rotator->geometryScale.connectFrom(&dragger->geometryScale);
    dragger->setPart("rotator", rotator);

    dragger->instantiateBaseGeometry();

    updateColorTheme();

    return ret;
}

void RadialGizmo::setRadius(float radius)
{
    auto dragger = getDraggerContainer()->getDragger();
    auto rotator = SO_GET_PART(dragger, "rotator", SoRotatorArrow);
    auto baseGeom = SO_GET_PART(dragger, "baseGeom", SoRotatorBase);

    rotator->radius = baseGeom->arcRadius = radius;
}

void RadialGizmo::flipArrow()
{
    auto dragger = getDraggerContainer()->getDragger();
    auto rotator = SO_GET_PART(dragger, "rotator", SoRotatorArrow);

    rotator->flipArrow();
}

void RadialGizmo::updateColorTheme()
{
    auto dragger = getDraggerContainer()->getDragger();

    auto* styleParameterManager = Base::provideService<Gui::StyleParameters::ParameterManager>();
    Base::Color baseColor = styleParameterManager->resolve(StyleParameters::RotationGizmoBaseColor);
    Base::Color activeColor = styleParameterManager->resolve(StyleParameters::RotationGizmoActiveColor);

    dragger->color = baseColor.asValue<SbColor>();
    dragger->activeColor = activeColor.asValue<SbColor>();

    auto baseGeom = SO_GET_PART(dragger, "baseGeom", SoRotatorBase);
    Base::Color baseGeomColor = styleParameterManager->resolve(
        StyleParameters::DimensionVisualizerColor
    );
    baseGeom->color = baseGeomColor.asValue<SbColor>();
}

SO_KIT_SOURCE(GizmoContainer)

void GizmoContainer::initClass()
{
    SO_KIT_INIT_CLASS(GizmoContainer, SoBaseKit, "BaseKit");
}

GizmoContainer::GizmoContainer()
    : viewProvider(nullptr)
{
    SO_KIT_CONSTRUCTOR(GizmoContainer);

#if defined(Q_OS_MACOS) || defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)
    this->ref();
#endif

    FC_ADD_CATALOG_ENTRY(annotation, So3DAnnotation, this);
    FC_ADD_CATALOG_ENTRY(pickStyle, SoPickStyle, annotation);
    FC_ADD_CATALOG_ENTRY(toggleSwitch, SoToggleSwitch, annotation);
    FC_ADD_CATALOG_ENTRY(geometry, SoSeparator, toggleSwitch);

    SO_KIT_INIT_INSTANCE();

    SO_KIT_ADD_FIELD(visible, (1));

    auto pickStyle = SO_GET_ANY_PART(this, "pickStyle", SoPickStyle);
    pickStyle->style = SoPickStyle::SHAPE_ON_TOP;

    auto toggleSwitch = SO_GET_ANY_PART(this, "toggleSwitch", SoToggleSwitch);
    toggleSwitch->on.connectFrom(&visible);

    setPart("geometry", new SoSeparator);

    cameraSensor.setFunction(&GizmoContainer::cameraChangeCallback);
    cameraSensor.setData(this);

    cameraPositionSensor.setData(this);
    cameraPositionSensor.setFunction(cameraPositionChangeCallback);
}

GizmoContainer::~GizmoContainer()
{
    cameraSensor.setData(nullptr);
    cameraSensor.detach();

    cameraPositionSensor.setData(nullptr);
    cameraPositionSensor.detach();

    uninitGizmos();

    if (!viewProvider.expired()) {
        viewProvider->setGizmoContainer(nullptr);
    }
}

void GizmoContainer::initGizmos()
{
    auto geometry = SO_GET_ANY_PART(this, "geometry", SoSeparator);
    for (auto gizmo : gizmos) {
        geometry->addChild(gizmo->initDragger());
    }
}

void GizmoContainer::uninitGizmos()
{
    for (auto gizmo : gizmos) {
        gizmo->uninitDragger();
        delete gizmo;
    }
    gizmos.clear();
}

void GizmoContainer::addGizmos(std::initializer_list<Gui::Gizmo*> gizmos)
{
    assert(this->gizmos.size() == 0 && "Already called GizmoContainer::addGizmos?");

    for (auto gizmo : gizmos) {
        addGizmo(gizmo);
    }
    initGizmos();
}

void GizmoContainer::addGizmo(Gizmo* gizmo)
{
    assert(std::ranges::find(gizmos, gizmo) == gizmos.end() && "this gizmo is already added!");
    gizmos.push_back(gizmo);
}

void GizmoContainer::attachViewer(Gui::View3DInventorViewer* viewer, Base::Placement& origin)
{
    if (!viewer) {
        return;
    }

    setUpAutoScale(viewer->getSoRenderManager()->getCamera());

    auto mat = origin.toMatrix();

    viewer->getDocument()->setEditingTransform(mat);
    So3DAnnotation* annotation = SO_GET_ANY_PART(this, "annotation", So3DAnnotation);
    viewer->setupEditingRoot(annotation, &mat);
}

void GizmoContainer::setUpAutoScale(SoCamera* cameraIn)
{
    if (cameraIn->getTypeId() == SoOrthographicCamera::getClassTypeId()) {
        auto localCamera = dynamic_cast<SoOrthographicCamera*>(cameraIn);
        cameraSensor.attach(&localCamera->height);
        cameraPositionSensor.attach(&localCamera->orientation);
        calculateScaleAndOrientation();
    }
    else if (cameraIn->getTypeId() == SoPerspectiveCamera::getClassTypeId()) {
        auto localCamera = dynamic_cast<SoPerspectiveCamera*>(cameraIn);
        cameraSensor.attach(&localCamera->position);
        cameraPositionSensor.attach(&localCamera->orientation);
        calculateScaleAndOrientation();
    }
}

void GizmoContainer::calculateScaleAndOrientation()
{
    if (cameraSensor.getAttachedField()) {
        cameraChangeCallback(this, nullptr);
        cameraPositionChangeCallback(this, nullptr);
    }
}

void GizmoContainer::cameraChangeCallback(void* data, SoSensor*)
{
    auto sudoThis = static_cast<GizmoContainer*>(data);

    SoField* field = sudoThis->cameraSensor.getAttachedField();
    if (!field) {
        return;
    }

    auto camera = static_cast<SoCamera*>(field->getContainer());

    SbViewVolume viewVolume = camera->getViewVolume();
    for (auto gizmo : sudoThis->gizmos) {
        float localScale = viewVolume.getWorldToScreenScale(gizmo->getDraggerPlacement().pos, 0.015);
        gizmo->setGeometryScale(localScale);
    }
}

void GizmoContainer::cameraPositionChangeCallback(void* data, SoSensor*)
{
    auto sudoThis = static_cast<GizmoContainer*>(data);

    SoField* field = sudoThis->cameraSensor.getAttachedField();
    if (field) {
        auto camera = static_cast<SoCamera*>(field->getContainer());

        for (auto gizmo : sudoThis->gizmos) {
            gizmo->orientAlongCamera(camera);
        }
    }
}

bool GizmoContainer::isEnabled()
{
    static Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().GetGroup(
        "BaseApp/Preferences/Gui/Gizmos"
    );

    return hGrp->GetBool("EnableGizmos", true);
}

std::unique_ptr<GizmoContainer> GizmoContainer::create(
    std::initializer_list<Gui::Gizmo*> gizmos,
    ViewProviderDragger* vp
)
{
    auto gizmoContainer = std::make_unique<GizmoContainer>();
    gizmoContainer->addGizmos(gizmos);
    gizmoContainer->viewProvider = vp;

    vp->setGizmoContainer(gizmoContainer.get());

    return gizmoContainer;
}
