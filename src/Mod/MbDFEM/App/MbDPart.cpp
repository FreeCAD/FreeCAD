// SPDX-License-Identifier: LGPL-2.1-or-later

#include "MbDPart.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

#include <BRepGProp.hxx>
#include <App/Document.h>
#include <App/GeoFeatureGroupExtension.h>
#include <Base/Exception.h>
#include <Base/Matrix.h>
#include <Base/Placement.h>
#include <GProp_GProps.hxx>
#include <GProp_PrincipalProps.hxx>
#include <Mod/Part/App/TopoShape.h>
#include <TopoDS_Shape.hxx>
#include <gp_Dir.hxx>

#include "MbDAssembly.h"
#include "MbDFolders.h"
#include "MbDGroupUtils.h"
#include "MbDMassMarker.h"
#include "MbDMarker.h"
#include "MbDPartPy.h"

PROPERTY_SOURCE_WITH_EXTENSIONS(MbDFEM::MbDPart, Part::Feature)

namespace
{

constexpr double mm2ToM2 = 1.0e-6;

struct PrincipalAxis
{
    double moment {};
    Base::Vector3d direction;
};

App::DocumentObjectGroup* matchingFolder(const char* subname,
                                         const char*& rest,
                                         App::DocumentObjectGroup* folder)
{
    rest = nullptr;
    const char* dot = subname ? std::strchr(subname, '.') : nullptr;
    if (!dot || !folder) {
        return nullptr;
    }

    if (std::string(subname, dot) == folder->getNameInDocument()) {
        rest = dot + 1;
        return folder;
    }
    return nullptr;
}

App::DocumentObject* findDirectChildByInternalName(const char* element,
                                                   const std::vector<App::DocumentObject*>& children)
{
    if (!element || !*element) {
        return nullptr;
    }

    std::string name(element);
    if (!name.empty() && name.back() == '.') {
        name.pop_back();
    }

    for (auto* child : children) {
        if (!child) {
            continue;
        }
        if (name == child->getNameInDocument()) {
            return child;
        }
    }
    return nullptr;
}

bool parentAssemblyVisible(const App::DocumentObject* object)
{
    auto* group = object ? App::GeoFeatureGroupExtension::getGroupOfObject(object) : nullptr;
    auto* assembly = freecad_cast<MbDFEM::MbDAssembly*>(group);
    return !assembly || assembly->Visibility.getValue();
}

Base::Vector3d toVector(const gp_Dir& direction)
{
    return Base::Vector3d(direction.X(), direction.Y(), direction.Z());
}

bool hasUsableMass(const GProp_GProps& props)
{
    return std::abs(props.Mass()) > 1.0e-12;
}

bool shapeMassProperties(const TopoDS_Shape& shape, GProp_GProps& props)
{
    if (shape.IsNull()) {
        return false;
    }

    BRepGProp::VolumeProperties(shape, props);
    if (hasUsableMass(props)) {
        return true;
    }

    props = GProp_GProps();
    BRepGProp::SurfaceProperties(shape, props);
    if (hasUsableMass(props)) {
        return true;
    }

    props = GProp_GProps();
    BRepGProp::LinearProperties(shape, props);
    return hasUsableMass(props);
}

bool hasDefaultMassProperties(const MbDFEM::MbDMassMarker* marker)
{
    if (!marker) {
        return false;
    }

    const Base::Vector3d inertias = marker->principalInertias.getValue();
    return std::abs(marker->mass.getValue() - 1.0) <= 1.0e-12
        && std::abs(inertias.x - 1.0) <= 1.0e-12
        && std::abs(inertias.y - 1.0) <= 1.0e-12
        && std::abs(inertias.z - 1.0) <= 1.0e-12;
}

std::array<PrincipalAxis, 3> sortedPrincipalAxes(const GProp_PrincipalProps& props)
{
    double firstMoment {};
    double secondMoment {};
    double thirdMoment {};
    props.Moments(firstMoment, secondMoment, thirdMoment);

    std::array<PrincipalAxis, 3> axes {{
        {firstMoment, toVector(props.FirstAxisOfInertia())},
        {secondMoment, toVector(props.SecondAxisOfInertia())},
        {thirdMoment, toVector(props.ThirdAxisOfInertia())},
    }};
    std::stable_sort(axes.begin(), axes.end(), [](const auto& left, const auto& right) {
        return left.moment < right.moment;
    });

    if (axes[0].direction.Cross(axes[1].direction).Dot(axes[2].direction) < 0.0) {
        axes[2].direction = -axes[2].direction;
    }
    return axes;
}

Base::Rotation principalAxesRotation(const std::array<PrincipalAxis, 3>& axes)
{
    Base::Matrix4D matrix;
    matrix[0][0] = axes[0].direction.x;
    matrix[1][0] = axes[0].direction.y;
    matrix[2][0] = axes[0].direction.z;
    matrix[0][1] = axes[1].direction.x;
    matrix[1][1] = axes[1].direction.y;
    matrix[2][1] = axes[1].direction.z;
    matrix[0][2] = axes[2].direction.x;
    matrix[1][2] = axes[2].direction.y;
    matrix[2][2] = axes[2].direction.z;

    return Base::Rotation(matrix);
}

Base::Placement shapePlacement(const TopoDS_Shape& shape)
{
    return Base::Placement(Part::TopoShape::convert(shape.Location().Transformation()));
}

Base::Placement massMarkerPlacement(const MbDFEM::MbDPart* part,
                                    const TopoDS_Shape& shape,
                                    const Base::Vector3d& center,
                                    const Base::Rotation& rotation)
{
    Base::Placement placement(center, rotation);
    if (!part) {
        return placement;
    }

    const Base::Placement partPlacement = part->Placement.getValue();
    if (shapePlacement(shape).isSame(partPlacement, 1.0e-7)) {
        placement = partPlacement.inverse() * placement;
    }
    return placement;
}

}  // namespace

MbDFEM::MbDPart::MbDPart()
{
    ADD_PROPERTY_TYPE(markers,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_None,
                      "Markers belonging to this part");
    markers.setScope(App::LinkScope::Child);
    ADD_PROPERTY_TYPE(massMarker,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_None,
                      "Marker representing the center of mass position and principal axes");
    massMarker.setScope(App::LinkScope::Child);
    ADD_PROPERTY_TYPE(_markersFolder,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_Hidden,
                      "Tree folder containing this part's markers");
    _markersFolder.setScope(App::LinkScope::Hidden);
    ADD_PROPERTY_TYPE(xs, (), "MbDFEM Results", App::Prop_None, "Solved X position values");
    ADD_PROPERTY_TYPE(ys, (), "MbDFEM Results", App::Prop_None, "Solved Y position values");
    ADD_PROPERTY_TYPE(zs, (), "MbDFEM Results", App::Prop_None, "Solved Z position values");
    ADD_PROPERTY_TYPE(bryxs, (), "MbDFEM Results", App::Prop_None, "Solved Bryant X angle values");
    ADD_PROPERTY_TYPE(bryys, (), "MbDFEM Results", App::Prop_None, "Solved Bryant Y angle values");
    ADD_PROPERTY_TYPE(bryzs, (), "MbDFEM Results", App::Prop_None, "Solved Bryant Z angle values");
    ADD_PROPERTY_TYPE(vxs, (), "MbDFEM Results", App::Prop_None, "Solved X velocity values");
    ADD_PROPERTY_TYPE(vys, (), "MbDFEM Results", App::Prop_None, "Solved Y velocity values");
    ADD_PROPERTY_TYPE(vzs, (), "MbDFEM Results", App::Prop_None, "Solved Z velocity values");
    ADD_PROPERTY_TYPE(omexs, (), "MbDFEM Results", App::Prop_None, "Solved X angular velocity values");
    ADD_PROPERTY_TYPE(omeys, (), "MbDFEM Results", App::Prop_None, "Solved Y angular velocity values");
    ADD_PROPERTY_TYPE(omezs, (), "MbDFEM Results", App::Prop_None, "Solved Z angular velocity values");
    ADD_PROPERTY_TYPE(axs, (), "MbDFEM Results", App::Prop_None, "Solved X acceleration values");
    ADD_PROPERTY_TYPE(ays, (), "MbDFEM Results", App::Prop_None, "Solved Y acceleration values");
    ADD_PROPERTY_TYPE(azs, (), "MbDFEM Results", App::Prop_None, "Solved Z acceleration values");
    ADD_PROPERTY_TYPE(alpxs, (), "MbDFEM Results", App::Prop_None, "Solved X angular acceleration values");
    ADD_PROPERTY_TYPE(alpys, (), "MbDFEM Results", App::Prop_None, "Solved Y angular acceleration values");
    ADD_PROPERTY_TYPE(alpzs, (), "MbDFEM Results", App::Prop_None, "Solved Z angular acceleration values");

    App::OriginGroupExtension::initExtension(this);
}

const App::PropertyComplexGeoData* MbDFEM::MbDPart::getPropertyOfGeometry() const
{
    return nullptr;
}

void MbDFEM::MbDPart::addMarker(MbDMarker* marker)
{
    if (!marker) {
        return;
    }

    addChildToListFolderAndGeoGroup(this, markers, ensureMarkersFolder(), marker);
}

void MbDFEM::MbDPart::removeMarker(MbDMarker* marker)
{
    removeChildFromListFolderAndGeoGroup(this, markers, getMarkersFolder(), marker);
}

void MbDFEM::MbDPart::setMassMarker(MbDMassMarker* marker)
{
    if (!marker) {
        massMarker.setValue(nullptr);
        return;
    }

    removeChildFromMbDFEMSemanticOwners(marker, this);
    removeAll(markers, marker);
    massMarker.setValue(marker);
    marker->massMarkerFromShape.setValue(false);

    if (auto* folder = ensureMarkersFolder()) {
        if (folder->hasObject(marker)) {
            folder->removeObject(marker);
        }
    }
    if (auto* group = getExtensionByType<App::GroupExtension>()) {
        appendUnique(group->Group, marker);
    }
}

MbDFEM::MbDMassMarker* MbDFEM::MbDPart::populateMassMarkerFromShape()
{
    GProp_GProps props;
    if (!shapeMassProperties(Shape.getValue(), props)) {
        throw Base::ValueError("MbDPart shape has no usable mass properties");
    }

    auto* marker = ensureMassMarker();
    if (!marker) {
        throw Base::ValueError("populateMassMarkerFromShape requires an attached document");
    }

    const gp_Pnt center = props.CentreOfMass();
    const auto axes = sortedPrincipalAxes(props.PrincipalProperties());
    const Base::Placement massPlacement = massMarkerPlacement(
        this,
        Shape.getValue(),
        Base::Vector3d(center.X(), center.Y(), center.Z()),
        principalAxesRotation(axes)
    );
    const double density = marker->densityInKgPerMm3();
    marker->Placement.setValue(massPlacement);
    marker->mass.setValue(props.Mass() * density);
    marker->principalInertias.setValue(
        Base::Vector3d(axes[0].moment * density * mm2ToM2,
                       axes[1].moment * density * mm2ToM2,
                       axes[2].moment * density * mm2ToM2));
    marker->massMarkerFromShape.setValue(true);
    marker->purgeTouched();
    return marker;
}

int MbDFEM::MbDPart::setElementVisible(const char* element, bool visible)
{
    auto* massMarkerObject = getMassMarker();
    if (massMarkerObject && findDirectChildByInternalName(element, {massMarkerObject})) {
        massMarkerObject->Visibility.setValue(visible);
        return visible ? 1 : 0;
    }

    auto* child = findDirectChildByInternalName(element, markers.getValues());
    if (!child) {
        return Part::Feature::setElementVisible(element, visible);
    }

    child->Visibility.setValue(visible);
    return visible ? 1 : 0;
}

int MbDFEM::MbDPart::isElementVisible(const char* element) const
{
    if (!Visibility.getValue() || !parentAssemblyVisible(this)) {
        return 0;
    }

    auto* child = findDirectChildByInternalName(element, markers.getValues());
    if (!child) {
        auto* massMarkerObject = getMassMarker();
        child = massMarkerObject ? findDirectChildByInternalName(element, {massMarkerObject}) : nullptr;
    }
    if (!child) {
        return Part::Feature::isElementVisible(element);
    }

    return child->Visibility.getValue() ? 1 : 0;
}

App::DocumentObject* MbDFEM::MbDPart::getSubObject(const char* subname,
                                                   PyObject** pyObj,
                                                   Base::Matrix4D* mat,
                                                   bool transform,
                                                   int depth) const
{
    const char* rest = nullptr;
    auto* folder = matchingFolder(subname, rest, getMarkersFolder());
    if (folder) {
        if (!rest || *rest == '\0') {
            return folder;
        }
        return Part::Feature::getSubObject(rest, pyObj, mat, transform, depth);
    }

    return Part::Feature::getSubObject(subname, pyObj, mat, transform, depth);
}

void MbDFEM::MbDPart::onChanged(const App::Property* prop)
{
    App::GeoFeature::onChanged(prop);

    auto* marker = getMassMarker();
    if (prop == &Shape && (marker == nullptr || marker->massMarkerFromShape.getValue())) {
        try {
            populateMassMarkerFromShape();
        }
        catch (const Base::Exception&) {
        }
    }
}

void MbDFEM::MbDPart::onDocumentRestored()
{
    Part::Feature::onDocumentRestored();

    auto* marker = getMassMarker();
    if (marker) {
        if (auto* folder = getMarkersFolder()) {
            if (folder->hasObject(marker)) {
                folder->removeObject(marker);
            }
        }
    }
    if (!marker || marker->massMarkerFromShape.getValue() || hasDefaultMassProperties(marker)) {
        try {
            populateMassMarkerFromShape();
        }
        catch (const Base::Exception&) {
        }
    }
}

void MbDFEM::MbDPart::unsetupObject()
{
    auto* document = getDocument();
    if (document) {
        const auto markerValues = markers.getValues();
        for (auto* marker : markerValues) {
            if (marker && marker->isAttachedToDocument() && !marker->isRemoving()) {
                document->removeObject(marker->getNameInDocument());
            }
        }
        if (auto* marker = getMassMarker()) {
            if (marker->isAttachedToDocument() && !marker->isRemoving()) {
                document->removeObject(marker->getNameInDocument());
            }
        }

        if (auto* folder = getMarkersFolder()) {
            if (folder->isAttachedToDocument() && !folder->isRemoving()) {
                document->removeObject(folder->getNameInDocument());
            }
        }
    }

    Part::Feature::unsetupObject();
}

App::DocumentObjectGroup* MbDFEM::MbDPart::getMarkersFolder() const
{
    return dynamic_cast<App::DocumentObjectGroup*>(_markersFolder.getValue());
}

MbDFEM::MbDMassMarker* MbDFEM::MbDPart::getMassMarker() const
{
    return dynamic_cast<MbDFEM::MbDMassMarker*>(massMarker.getValue());
}

MbDFEM::MbDMassMarker* MbDFEM::MbDPart::ensureMassMarker()
{
    if (auto* marker = getMassMarker()) {
        return marker;
    }
    if (!getDocument()) {
        return nullptr;
    }

    const std::string name = std::string(getNameInDocument()) + "_MassMarker";
    auto* marker = static_cast<MbDFEM::MbDMassMarker*>(
        getDocument()->addObject("MbDFEM::MbDMassMarker", name.c_str()));
    marker->Label.setValue("MassMarker");
    setMassMarker(marker);
    return marker;
}

App::DocumentObjectGroup* MbDFEM::MbDPart::ensureMarkersFolder()
{
    if (auto* folder = getMarkersFolder()) {
        return folder;
    }
    if (!getDocument()) {
        return nullptr;
    }

    const std::string name = std::string(getNameInDocument()) + "_Markers";
    auto* folder = static_cast<App::DocumentObjectGroup*>(
        getDocument()->addObject("MbDFEM::MbDMarkersFolder", name.c_str()));
    folder->Label.setValue("Markers");
    _markersFolder.setValue(folder);
    return folder;
}

PyObject* MbDFEM::MbDPart::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        PythonObject = Py::Object(new MbDPartPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}
