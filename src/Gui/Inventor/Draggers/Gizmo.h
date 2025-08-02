#ifndef GUI_GIZMO_H
#define GUI_GIZMO_H

#include <Inventor/SbMatrix.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/sensors/SoFieldSensor.h>
#include <Inventor/nodekits/SoBaseKit.h>
#include <Inventor/SbVec3f.h>

#include <Base/Placement.h>
#include <Gui/ViewProvider.h>
#include <Gui/Inventor/So3DAnnotation.h>

class SoDragger;
class SoCamera;
class SoInteractionKit;

namespace Gui
{
class SoLinearDragger;
class SoLinearDraggerContainer;
class SoRotationDragger;
class SoRotationDraggerContainer;
class QuantitySpinBox;

struct GizmoPlacement
{
    SbVec3f pos;
    SbVec3f dir;
};

class Gizmo
{
public:
    double multFactor = 1.0f;
    double addFactor = 0.0f;

    virtual ~Gizmo() = default;
    virtual SoInteractionKit* initDragger() = 0;
    virtual void uninitDragger() = 0;

    virtual GizmoPlacement getDraggerPlacement() = 0;
    virtual void setDraggerPlacement(const SbVec3f& pos, const SbVec3f& dir) = 0;
    void setDraggerPlacement(const Base::Vector3d& pos, const Base::Vector3d& dir);

    virtual void setGeometryScale(float scale) = 0;
    virtual void orientAlongCamera([[maybe_unused]] SoCamera* camera) {};
    void setProperty(QuantitySpinBox* property) {this->property = property;};

protected:
    QuantitySpinBox* property = nullptr;
    double initialValue;
};

class LinearGizmo: public Gizmo
{
public:
    LinearGizmo() = default;
    ~LinearGizmo() override = default;

    SoInteractionKit* initDragger() override;
    void uninitDragger() override;

    // Returns the position and rotation of the base of the dragger
    GizmoPlacement getDraggerPlacement() override;
    void setDraggerPlacement(const SbVec3f& pos, const SbVec3f& dir) override;
    void setDraggerPlacement(Base::Placement placement);
    // Returns the drag distance from the base of the feature
    double getDragLength();
    void setDragLength(double dragLength);
    void setGeometryScale(float scale) override;
    SoLinearDraggerContainer* getDraggerContainer();

private:
    SoLinearDragger* dragger = nullptr;
    SoLinearDraggerContainer* draggerContainer = nullptr;

    static void dragStartCallback(void *data, SoDragger *d);
    static void dragFinishCallback(void *data, SoDragger *d);
    static void dragMotionCallback(void *data, SoDragger *d);
};

class RotationGizmo: public Gizmo
{
public:
    RotationGizmo() = default;
    ~RotationGizmo() override;

    SoInteractionKit* initDragger() override;
    void uninitDragger() override;

    // Distance between the linear gizmo base and rotation gizmo
    double sepDistance = 5;

    // Returns the position and rotation of the base of the dragger
    GizmoPlacement getDraggerPlacement() override;
    void setDraggerPlacement(const SbVec3f& pos, const SbVec3f& dir) override;
    // The two gizmos are separated by sepDistance units
    void placeOverLinearGizmo(LinearGizmo* gizmo);
    // Returns the rotation angle wrt the normal axis
    double getRotAngle();
    void setRotAngle(double angle);
    void setGeometryScale(float scale) override;
    SoRotationDraggerContainer* getDraggerContainer();
    void orientAlongCamera(SoCamera* camera) override;

private:
    SoRotationDragger* dragger = nullptr;
    SoRotationDraggerContainer* draggerContainer = nullptr;
    SoFieldSensor translationSensor;
    LinearGizmo* linearGizmo = nullptr;

    static void dragStartCallback(void *data, SoDragger *d);
    static void dragFinishCallback(void *data, SoDragger *d);
    static void dragMotionCallback(void *data, SoDragger *d);
    static void translationSensorCB(void* data, SoSensor* sensor);
};

enum class GizmoType: uint8_t
{
    Linear,
    Rotational
};

class Gizmos: public SoBaseKit
{
    SO_KIT_HEADER(Gizmos);
    SO_KIT_CATALOG_ENTRY_HEADER(annotation);
    SO_KIT_CATALOG_ENTRY_HEADER(pickStyle);
    SO_KIT_CATALOG_ENTRY_HEADER(toggleSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(geometry);

public:
    static void initClass();
    Gizmos();
    ~Gizmos() override;

    SoSFBool visible;

    void initGizmos();
    void uninitGizmos();

    template<typename T = Gizmo>
    T* getGizmo(int index)
    {
        assert(index >= 0 && index < static_cast<int>(gizmos.size()) && "index out of range!");
        T* ptr  = dynamic_cast<T*>(gizmos[index]);
        assert(ptr);

        return ptr;
    }

    void addGizmo(Gizmo* gizmo);
    void attachViewer(Gui::View3DInventorViewer* viewer, Base::Placement &origin);
    void setUpAutoScale(SoCamera* cameraIn);

    // Checks if the gizmos are enabled in the preferences
    static bool isEnabled();

private:
    std::vector<Gizmo*> gizmos;
    SoFieldSensor cameraSensor;
    SoFieldSensor cameraPositionSensor;

    static void cameraChangeCallback(void* data, SoSensor*);
    static void cameraPositionChangeCallback(void* data, SoSensor*);
};

}

#endif /* GUI_GIZMO_H */
