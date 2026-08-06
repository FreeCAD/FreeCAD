// SPDX-License-Identifier: LGPL-2.1-or-later

#include <BRepExtrema_DistShapeShape.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <Precision.hxx>
#include <TopoDS.hxx>

#include <Base/Exception.h>
#include <Base/Rotation.h>
#include <Mod/Part/App/Part2DObject.h>

#include "FeatureProjectOnSurface.h"

using namespace PartDesign;

PROPERTY_SOURCE(PartDesign::ProjectOnSurface, Part::ProjectOnSurface)

ProjectOnSurface::ProjectOnSurface()
{
    // Helper geometry may reference sketches or faces outside its Body.
    Projection.setScope(App::LinkScope::Global);
    SupportFace.setScope(App::LinkScope::Global);

    // Direction is derived during recompute rather than supplied by the camera.
    Direction.setStatus(App::Property::Hidden, true);
    SupportFace.setStatus(App::Property::Hidden, false);

    Mode.setValue(Part::ProjectOnSurface::AllMode);
}

App::DocumentObjectExecReturn* ProjectOnSurface::execute()
{
    // Incomplete input is normal while the task panel is open. Clear any stale
    // preview and wait until both source geometry and a target face are present.
    if (Projection.getSize() == 0 || !SupportFace.getValue()) {
        Shape.setValue(TopoDS_Shape());
        return App::DocumentObject::StdReturn;
    }

    Direction.setValue(calculateDirection());
    return Part::ProjectOnSurface::execute();
}

Base::Vector3d ProjectOnSurface::calculateDirection() const
{
    const auto objects = Projection.getValues();
    const auto subNames = Projection.getSubValues();
    if (objects.size() != subNames.size() || objects.empty()) {
        throw Base::ValueError("Projection sources are incomplete");
    }

    gp_Dir commonDirection;
    TopoDS_Shape referenceShape;
    bool haveDirection = false;

    for (std::size_t index = 0; index < objects.size(); ++index) {
        Base::Matrix4D transform;
        App::DocumentObject* owner = nullptr;
        auto source = Part::Feature::getTopoShape(
            objects[index],
            Part::ShapeOption::NeedSubElement | Part::ShapeOption::ResolveLink
                | Part::ShapeOption::Transform,
            subNames[index].c_str(),
            &transform,
            &owner
        );
        if (source.isNull()) {
            throw Part::NullShapeException("Projection source has an empty shape");
        }

        gp_Dir sourceDirection;
        if (owner && owner->isDerivedFrom<Part::Part2DObject>()) {
            // Sketches define their plane in local XY, so transform local +Z into
            // document coordinates through attachment and container placements.
            Base::Vector3d normal;
            Base::Rotation(transform).multVec(Base::Vector3d(0, 0, 1), normal);
            sourceDirection = gp_Dir(normal.x, normal.y, normal.z);
        }
        else {
            gp_Pln plane;
            if (!source.findPlane(plane)) {
                throw Base::ValueError("Projection source geometry must be planar");
            }
            sourceDirection = plane.Axis().Direction();
        }

        if (!haveDirection) {
            commonDirection = sourceDirection;
            referenceShape = source.getShape();
            haveDirection = true;
        }
        else if (!commonDirection.IsParallel(sourceDirection, Precision::Angular())) {
            throw Base::ValueError("All projection sources must have parallel plane normals");
        }
    }

    // A normal has two possible signs. Point it toward the selected target face.
    const auto targetSubNames = SupportFace.getSubValues();
    auto target = Part::Feature::getTopoShape(
        SupportFace.getValue(),
        Part::ShapeOption::NeedSubElement | Part::ShapeOption::ResolveLink
            | Part::ShapeOption::Transform,
        targetSubNames.empty() ? nullptr : targetSubNames.front().c_str()
    );
    if (target.isNull() || target.getShape().ShapeType() != TopAbs_FACE) {
        throw Base::TypeError("Projection target must be a face");
    }

    BRepExtrema_DistShapeShape distance(referenceShape, target.getShape());
    distance.Perform();
    if (distance.IsDone() && distance.NbSolution() > 0) {
        const gp_Vec towardTarget(distance.PointOnShape1(1), distance.PointOnShape2(1));
        if (towardTarget.SquareMagnitude() > Precision::SquareConfusion()
            && towardTarget.Dot(commonDirection) < 0) {
            commonDirection.Reverse();
        }
    }

    return Base::Vector3d(commonDirection.X(), commonDirection.Y(), commonDirection.Z());
}

const char* ProjectOnSurface::getViewProviderName() const
{
    return "PartDesignGui::ViewProviderProjectOnSurface";
}
