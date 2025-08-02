#include "Gizmo.h"

#include <Inventor/draggers/SoDragger.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoScale.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/So3DAnnotation.h>
#include <Inventor/SoToggleSwitch.h>
#include <Base/Converter.h>
#include <Base/Console.h>

#include <algorithm>
#include "App/Application.h"
#include "Base/Parameter.h"
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

SoInteractionKit* LinearGizmo::initDragger()
{
    draggerContainer = new SoLinearDraggerContainer;
    draggerContainer->color.setValue(1, 0, 0);
    dragger = draggerContainer->getDragger();

    dragger->addStartCallback(dragStartCallback, this);
    dragger->addFinishCallback(dragFinishCallback, this);
    dragger->addMotionCallback(dragMotionCallback, this);
    dragger->labelVisible = false;

    setDragLength(property->value().getValue());

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

void LinearGizmo::dragStartCallback(void *data, [[maybe_unused]] SoDragger *d)
{
    Base::Console().message("Started dragging\n");

    auto sudoThis = static_cast<LinearGizmo*>(data);
    sudoThis->initialValue = sudoThis->property->value().getValue();
    sudoThis->dragger->translationIncrementCount.setValue(0);
}

void LinearGizmo::dragFinishCallback(void *data, [[maybe_unused]] SoDragger *d)
{
    Base::Console().message("Finished dragging\n");
}

void LinearGizmo::dragMotionCallback(void *data, [[maybe_unused]] SoDragger *d)
{
    auto sudoThis = static_cast<LinearGizmo*>(data);

    double value = sudoThis->initialValue + sudoThis->getDragLength();
    // TODO: Need to change the lower limit to sudoThis->property->minimum() once the
    // two direction extrude work gets merged
    value = std::clamp(value, sudoThis->dragger->translationIncrement.getValue(), sudoThis->property->maximum());

    sudoThis->property->setValue(value);
    sudoThis->setDragLength(value);

    Base::Console().message("Continuing dragging, value: %lf\n", value);
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

    dragger->addStartCallback(dragStartCallback, this);
    dragger->addFinishCallback(dragFinishCallback, this);
    dragger->addMotionCallback(dragMotionCallback, this);

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

void RotationGizmo::dragStartCallback(void *data, [[maybe_unused]] SoDragger *d)
{
    Base::Console().message("Started rotating\n");

    auto sudoThis = static_cast<RotationGizmo*>(data);
    sudoThis->initialValue = sudoThis->property->value().getValue();
    sudoThis->dragger->rotationIncrementCount.setValue(0);
}

void RotationGizmo::dragFinishCallback(void *data, [[maybe_unused]] SoDragger *d)
{
    Base::Console().message("Finished rotating\n");
}

void RotationGizmo::dragMotionCallback(void *data, [[maybe_unused]] SoDragger *d)
{
    auto sudoThis = static_cast<RotationGizmo*>(data);

    double value = sudoThis->initialValue + sudoThis->getRotAngle();
    value = fmod(value, 360);
    value = std::clamp(value, sudoThis->property->minimum(), sudoThis->property->maximum());

    sudoThis->property->setValue(value);
    sudoThis->setRotAngle(value);

    Base::Console().message("Continuing rotating, value: %lf, max: %lf, min: %lf\n", value, sudoThis->property->minimum(), sudoThis->property->maximum());
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

void RotationGizmo::orientAlongCamera(SoCamera* camera)
{
    if (linearGizmo == nullptr) {
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
        cameraChangeCallback(this, nullptr);
        cameraPositionSensor.attach(&localCamera->orientation);
        cameraPositionChangeCallback(this, nullptr);
    }
    else if (cameraIn->getTypeId() == SoPerspectiveCamera::getClassTypeId()) {
        auto localCamera = dynamic_cast<SoPerspectiveCamera*>(cameraIn);
        assert(localCamera);
        cameraSensor.attach(&localCamera->position);
        cameraChangeCallback(this, nullptr);
        cameraPositionSensor.attach(&localCamera->orientation);
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

    return hGrp->GetBool("EnableGizmos", false);
}
