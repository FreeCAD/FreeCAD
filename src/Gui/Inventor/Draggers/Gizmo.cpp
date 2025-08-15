#include "PreCompiled.h"

#include "Gizmo.h"

#ifndef _PreComp_
#include <Inventor/draggers/SoDragger.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoScale.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <algorithm>
#endif

#include "Gui/Inventor/So3DAnnotation.h"
#include "Gui/Inventor/SoToggleSwitch.h"
#include <Base/Converter.h>
#include <Base/Console.h>

#include "App/Application.h"
#include "Base/Parameter.h"
#include "Base/Precision.h"
#include "Document.h"
#include "Gui/Utilities.h"

#include "QuantitySpinBox.h"
#include "SoLinearDragger.h"
#include "SoRotationDragger.h"
#include "View3DInventorViewer.h"

using namespace Gui;

void Gizmo::setDraggerPlacement(const Base::Vector3d& pos, const Base::Vector3d& dir)
{
    setDraggerPlacement(
        Base::convertTo<SbVec3f>(pos),
        Base::convertTo<SbVec3f>(dir)
    );
}

bool Gizmo::delayedUpdateEnabled()
{
    static Base::Reference<ParameterGrp> hGrp = App::GetApplication()
        .GetUserParameter()
        .GetGroup("BaseApp/Preferences/Mod/PartDesign");

    return hGrp->GetBool("DelayedGizmoUpdate", false);
}

LinearGizmo::LinearGizmo(QuantitySpinBox* property)
{
    setProperty(property);
}

SoInteractionKit* LinearGizmo::initDragger()
{
    draggerContainer = new SoLinearDraggerContainer;
    draggerContainer->color.setValue(1, 0, 0);
    dragger = draggerContainer->getDragger();

    dragger->addStartCallback(
        [] (void* data, SoDragger*) {
            assert(data);
            static_cast<LinearGizmo*>(data)->draggingStarted();
        },
        this
    );
    dragger->addFinishCallback(
        [] (void* data, SoDragger*) {
            assert(data);
            static_cast<LinearGizmo*>(data)->draggingFinished();
        },
        this
    );
    dragger->addMotionCallback(
        [] (void* data, SoDragger*) {
            assert(data);
            static_cast<LinearGizmo*>(data)->draggingContinued();
        },
        this
    );

    dragger->labelVisible = false;

    setDragLength(property->value().getValue());

    dragger->instantiateBaseGeometry();

    // change the dragger dimensions
    auto arrow = SO_GET_PART(dragger, "arrow", SoArrowGeometry);
    arrow->cylinderHeight = 3.5;
    arrow->cylinderRadius = 0.2;

    return draggerContainer;
}

void LinearGizmo::uninitDragger()
{
    dragger = nullptr;
    draggerContainer = nullptr;
}

GizmoPlacement LinearGizmo::getDraggerPlacement()
{
    assert(draggerContainer);
    return {draggerContainer->translation.getValue(), draggerContainer->getPointerDirection()};
}

void LinearGizmo::setDraggerPlacement(const SbVec3f& pos, const SbVec3f& dir)
{
    assert(draggerContainer);
    draggerContainer->translation = pos;
    draggerContainer->setPointerDirection(dir);
}

void LinearGizmo::setDraggerPlacement(Base::Placement placement)
{
    assert(draggerContainer);
    draggerContainer->translation = Base::convertTo<SbVec3f>(placement.getPosition());
    draggerContainer->rotation = Base::convertTo<SbRotation>(placement.getRotation());
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
}

SoLinearDraggerContainer* LinearGizmo::getDraggerContainer()
{
    assert(draggerContainer);
    return draggerContainer;
}

void LinearGizmo::setProperty(QuantitySpinBox* property)
{
    if (connection) {
        QuantitySpinBox::disconnect(connection);
    }

    this->property = property;
    connection = QuantitySpinBox::connect(
        property, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        [this] (double value) {
            setDragLength(value);
        }
    );
}

void LinearGizmo::draggingStarted()
{
    initialValue = property->value().getValue();
    dragger->translationIncrementCount.setValue(0);

    if (delayedUpdateEnabled()) {
        property->blockSignals(true);
    }
}

void LinearGizmo::draggingFinished()
{
    if (delayedUpdateEnabled()) {
        property->blockSignals(false);
        property->valueChanged(property->value().getValue());
    }
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
    setProperty(property);
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
        [] (void* data, SoDragger*) {
            assert(data);
            static_cast<RotationGizmo*>(data)->draggingStarted();
        },
        this
    );
    dragger->addFinishCallback(
        [] (void* data, SoDragger*) {
            assert(data);
            static_cast<RotationGizmo*>(data)->draggingFinished();
        },
        this
    );
    dragger->addMotionCallback(
        [] (void* data, SoDragger*) {
            assert(data);
            static_cast<RotationGizmo*>(data)->draggingContinued();
        },
        this
    );

    setRotAngle(property->value().getValue());

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

GizmoPlacement RotationGizmo::getDraggerPlacement()
{
    assert(draggerContainer);
    return {draggerContainer->translation.getValue(), draggerContainer->getPointerDirection()};
}

void RotationGizmo::setDraggerPlacement(const SbVec3f& pos, const SbVec3f& dir)
{
    assert(draggerContainer);
    draggerContainer->translation = pos;
    draggerContainer->setPointerDirection(dir);
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
    assert(data);
    auto sudoThis = static_cast<RotationGizmo*>(data);
    assert(sensor);
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
    assert(draggerContainer);
    return draggerContainer;
}

void RotationGizmo::draggingStarted()
{
    initialValue = property->value().getValue();
    dragger->rotationIncrementCount.setValue(0);

    if (delayedUpdateEnabled()) {
        property->blockSignals(true);
    }
}

void RotationGizmo::draggingFinished()
{
    if (delayedUpdateEnabled()) {
        property->blockSignals(false);
        property->valueChanged(property->value().getValue());
    }
}

double normalizeIn360(double value)
{
    value = std::fmod(value, 360.0);
    if (value < 0.0) {
        value += 360.0;
    }

    return value;
}

double angularDist(double v1, double v2)
{
    return std::min(std::fabs(v1 - v2), 360 - std::fabs(v1 - v2));
}

// Returns a value between [0, 360) or (-180, 180] depending on if the
// minimum value was positive or negetive. This is done because the taper angle
// values in FreeCAD usually treat values like -10 and 350 differently
double clampAngle(double value, double min, double max)
{
    value = normalizeIn360(value);
    double nMin = normalizeIn360(min);
    double nMax = normalizeIn360(max);

    if (std::abs(nMax - nMin) > Base::Precision::Confusion()) {
        if (nMax > nMin) {
            if (value < nMin || value > nMax) {
                value = angularDist(value, nMin) > angularDist(value, nMax)? nMax : nMin;
            }
        } else {
            if (value < nMin && value > nMax) {
                value = angularDist(value, nMin) > angularDist(value, nMax)? nMax : nMin;
            }
        }
    }

    if (min >= 0.0) {
        // Return in [0, 360)
        return value;
    }

    // Map to (-180, 180]
    if (value > 180.0) {
        value = value - 360;
    }
    return value;
}

void RotationGizmo::draggingContinued()
{
    double value = initialValue + getRotAngle();
    value = clampAngle(value, property->minimum(), property->maximum());

    property->setValue(value);
    setRotAngle(value);
}

void RotationGizmo::orientAlongCamera(SoCamera* camera)
{
    if (linearGizmo == nullptr || automaticOrientation == false) {
        return;
    }

    assert(camera);
    SbVec3f cameraDir{0, 0, 1};
    camera->orientation.getValue().multVec(cameraDir, cameraDir);
    SbVec3f pointerDir = linearGizmo->getDraggerContainer()->getPointerDirection();

    pointerDir.normalize();
    auto proj = cameraDir - cameraDir.dot(pointerDir) * pointerDir;
    if (proj.equals(SbVec3f{0, 0, 0}, 0.001)) {
        return;
    }

    assert(draggerContainer);
    draggerContainer->setArcNormalDirection(proj);
}

void RotationGizmo::setProperty(QuantitySpinBox* property)
{
    if (connection) {
        QuantitySpinBox::disconnect(connection);
    }

    this->property = property;
    connection = QuantitySpinBox::connect(
        property, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        [this] (double value) {
            setRotAngle(value);
        }
    );
}


DirectedRotationGizmo::DirectedRotationGizmo(QuantitySpinBox* property): RotationGizmo(property)
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

    return ret;
}

void DirectedRotationGizmo::flipArrow()
{
    auto dragger = getDraggerContainer()->getDragger();
    auto rotator = SO_GET_PART(dragger, "rotator", SoRotatorGeometry2);
    assert(rotator);

    rotator->toggleArrowVisibility();
}


RadialGizmo::RadialGizmo(QuantitySpinBox* property): RotationGizmo(property)
{}

SoInteractionKit* RadialGizmo::initDragger()
{
    SoInteractionKit* ret = inherited::initDragger();

    auto dragger = getDraggerContainer()->getDragger();
    auto rotator = new SoRotatorArrow;
    rotator->geometryScale.connectFrom(&dragger->geometryScale);
    dragger->setPart("rotator", rotator);

    dragger->instantiateBaseGeometry();

    return ret;
}

void RadialGizmo::setRadius(float radius)
{
    auto dragger = getDraggerContainer()->getDragger();
    auto rotator = SO_GET_PART(dragger, "rotator", SoRotatorArrow);
    auto baseGeom = SO_GET_PART(dragger, "baseGeom", SoRotatorBase);
    assert(rotator && baseGeom);

    rotator->radius = baseGeom->arcRadius = radius;
}

void RadialGizmo::flipArrow()
{
    auto dragger = getDraggerContainer()->getDragger();
    auto rotator = SO_GET_PART(dragger, "rotator", SoRotatorArrow);
    assert(rotator);

    rotator->flipArrow();
}

SO_KIT_SOURCE(Gizmos)

void Gizmos::initClass()
{
    SO_KIT_INIT_CLASS(Gizmos, SoBaseKit, "BaseKit");
}

Gizmos::Gizmos()
{
    SO_KIT_CONSTRUCTOR(Gizmos);

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

    cameraSensor.setFunction(&Gizmos::cameraChangeCallback);
    cameraSensor.setData(this);

    cameraPositionSensor.setData(this);
    cameraPositionSensor.setFunction(cameraPositionChangeCallback);
}

Gizmos::~Gizmos()
{
    cameraSensor.setData(nullptr);
    cameraSensor.detach();

    cameraPositionSensor.setData(nullptr);
    cameraPositionSensor.detach();

    uninitGizmos();
}

void Gizmos::initGizmos()
{
    auto geometry = SO_GET_ANY_PART(this, "geometry", SoSeparator);
    for (auto gizmo: gizmos) {
        geometry->addChild(gizmo->initDragger());
    }
}

void Gizmos::uninitGizmos()
{
    for (auto gizmo: gizmos) {
        gizmo->uninitDragger();
        delete gizmo;
    }
    gizmos.clear();
}

void Gizmos::addGizmo(Gizmo* gizmo)
{
    assert(std::ranges::find(gizmos, gizmo) == gizmos.end() && "this gizmo is already added!");
    gizmos.push_back(gizmo);
}

void Gizmos::attachViewer(Gui::View3DInventorViewer* viewer, Base::Placement &origin)
{
    if (viewer) {
        auto mat = origin.toMatrix();

        viewer->getDocument()->setEditingTransform(mat);
        So3DAnnotation* annotation = SO_GET_ANY_PART(this, "annotation", So3DAnnotation);
        viewer->setupEditingRoot(annotation, &mat);
    }
}

void Gizmos::setUpAutoScale(SoCamera* cameraIn)
{
    if (cameraIn->getTypeId() == SoOrthographicCamera::getClassTypeId()) {
        auto localCamera = dynamic_cast<SoOrthographicCamera*>(cameraIn);
        assert(localCamera);
        cameraSensor.attach(&localCamera->height);
        cameraPositionSensor.attach(&localCamera->orientation);
        calculateScaleAndOrientation();

    }
    else if (cameraIn->getTypeId() == SoPerspectiveCamera::getClassTypeId()) {
        auto localCamera = dynamic_cast<SoPerspectiveCamera*>(cameraIn);
        assert(localCamera);
        cameraSensor.attach(&localCamera->position);
        cameraPositionSensor.attach(&localCamera->orientation);
        calculateScaleAndOrientation();
    }
}

void Gizmos::calculateScaleAndOrientation()
{
    if (cameraSensor.getAttachedField()) {
        cameraChangeCallback(this, nullptr);
        cameraPositionChangeCallback(this, nullptr);
    }
}

void Gizmos::cameraChangeCallback(void* data, SoSensor*)
{
    assert(data);
    auto sudoThis = static_cast<Gizmos*>(data);

    SoField* field = sudoThis->cameraSensor.getAttachedField();
    if (field) {
        auto camera = static_cast<SoCamera*>(field->getContainer());

        SbViewVolume viewVolume = camera->getViewVolume();
        for (auto gizmo: sudoThis->gizmos) {
            float localScale = viewVolume.getWorldToScreenScale(gizmo->getDraggerPlacement().pos, 0.015);
            gizmo->setGeometryScale(localScale);
        }
    }
}

void Gizmos::cameraPositionChangeCallback(void* data, SoSensor*)
{
    assert(data);
    auto sudoThis = static_cast<Gizmos*>(data);

    SoField* field = sudoThis->cameraSensor.getAttachedField();
    if (field) {
        auto camera = static_cast<SoCamera*>(field->getContainer());

        for (auto gizmo: sudoThis->gizmos) {
            gizmo->orientAlongCamera(camera);
        }
    }
}

bool Gizmos::isEnabled()
{
    static Base::Reference<ParameterGrp> hGrp = App::GetApplication()
        .GetUserParameter()
        .GetGroup("BaseApp/Preferences/Mod/PartDesign");

    return hGrp->GetBool("EnableGizmos", true);
}
