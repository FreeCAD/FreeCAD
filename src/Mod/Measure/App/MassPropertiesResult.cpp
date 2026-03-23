// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Morten Vajhøj
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/


#include "MassPropertiesResult.h"

#include <App/DocumentObject.h>
#include <App/Datums.h>

#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/Tools.h>
#include <Base/BaseClass.h>
#include <Base/Matrix.h>
#include <Base/Quantity.h>
#include <Base/Converter.h>

#include <TopoDS_Shape.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <gp_Trsf.hxx>
#include <GProp_GProps.hxx>
#include <BRepGProp.hxx>
#include <GProp_PrincipalProps.hxx>
#include <gp_Pnt.hxx>
#include <gp_Mat.hxx>
#include <gp_Ax3.hxx>
#include <math_Jacobi.hxx>
#include <math_Vector.hxx>

#include <map>
#include <unordered_set>
#include <string>
#include <vector>
#include <cmath>
#include <QString>

MassPropertiesData CalculateMassProperties(
    const std::vector<MassPropertiesInput>& objects,
    MassPropertiesMode mode,
    const App::DocumentObject* referenceDatum,
    const Base::Placement* referencePlacement
)
{
    MassPropertiesData data {};
    if (objects.empty()) {
        return data;
    }

    GProp_GProps globalMassProps;
    GProp_GProps globalSurfaceProps;
    GProp_GProps globalVolumeProps;

    double totalVolume = 0.0;
    bool hasShape = false;

    for (const auto& object : objects) {
        App::DocumentObject* obj = object.object;
        if (!obj && object.shape.IsNull()) {
            continue;
        }

        TopoDS_Shape shape = object.shape;
        if (shape.IsNull()) {
            auto* part = freecad_cast<Part::Feature*>(obj);
            if (!part) {
                continue;
            }

            shape = part->Shape.getShape().getShape();
        }

        if (shape.IsNull()) {
            continue;
        }

        if (!object.placement.isIdentity()) {
            gp_Trsf trsf = Part::Tools::fromPlacement(object.placement).Transformation();
            BRepBuilderAPI_Transform transformer(shape, trsf, true);
            shape = transformer.Shape();
        }


        auto materialFeature = [](App::DocumentObject* candidate) -> Part::Feature* {
            std::unordered_set<App::DocumentObject*> visited;

            while (candidate && visited.insert(candidate).second) {
                if (auto* feature = freecad_cast<Part::Feature*>(candidate)) {
                    return feature;
                }

                App::DocumentObject* linked = candidate->getLinkedObject(true);
                if (!linked || linked == candidate) {
                    break;
                }
                candidate = linked;
            }

            return nullptr;
        };

        Part::Feature* part = materialFeature(obj);

        Materials::Material mat;
        // Fallback density the units 1e-6 kg/mm^3 (1000 kg/m^3)
        double density = 1.0e-6;

        const QString densityMaterialProperty = QStringLiteral("Density");

        if (part) {
            mat = part->ShapeMaterial.getValue();
        }
        if (mat.hasPhysicalProperty(densityMaterialProperty)) {
            try {
                if (mat.getName() != QStringLiteral("Default")) {
                    density = mat.getPhysicalQuantity(densityMaterialProperty).getValue();
                }
            }
            catch (...) {
                Base::Console().message(
                    "Error retrieving density from material. Using default value.\n"
                );
            }
        }
        else {
            Base::Console().message("Density property not found for material. Using default value.\n");
        }

        GProp_GProps volProps;
        BRepGProp::VolumeProperties(shape, volProps);
        globalVolumeProps.Add(volProps);

        GProp_GProps surfProps;
        BRepGProp::SurfaceProperties(shape, surfProps);

        totalVolume += volProps.Mass();
        globalMassProps.Add(volProps, density);
        globalSurfaceProps.Add(surfProps);

        hasShape = true;
    }

    if (!hasShape) {
        return data;
    }

    data.volume = Base::Quantity(totalVolume, Base::Unit::Volume);
    data.mass = Base::Quantity(globalMassProps.Mass(), Base::Unit::Mass);
    data.surfaceArea = Base::Quantity(globalSurfaceProps.Mass(), Base::Unit::Area);

    if (data.volume.getValue() > 0.0) {
        data.density
            = Base::Quantity(data.mass.getValue() / data.volume.getValue(), Base::Unit::Density);
    }

    data.cog = Base::convertTo<Base::Vector3d>(globalMassProps.CentreOfMass());
    data.cov = Base::convertTo<Base::Vector3d>(globalVolumeProps.CentreOfMass());


    gp_Mat inertia = globalMassProps.MatrixOfInertia();

    GProp_PrincipalProps principal;

    if (mode == MassPropertiesMode::CenterOfGravity) {
        data.inertiaJo = Base::Vector3d(inertia(1, 1), inertia(2, 2), inertia(3, 3));
        data.inertiaJCross = Base::Vector3d(inertia(1, 2), inertia(1, 3), inertia(2, 3));

        principal = globalMassProps.PrincipalProperties();

        if (principal.HasSymmetryPoint()) {
            data.principalAxisX = Base::Vector3d::UnitX;
            data.principalAxisY = Base::Vector3d::UnitY;
            data.principalAxisZ = Base::Vector3d::UnitZ;
        }
        else if (principal.HasSymmetryAxis()) {
            gp_Vec zVec = principal.FirstAxisOfInertia();
            Base::Vector3d zAxis {Base::convertTo<Base::Vector3d>(zVec)};
            zAxis.Normalize();

            Base::Vector3d ref {Base::Vector3d::UnitZ};
            if (std::fabs(zAxis.z) > 0.9) {
                ref = Base::Vector3d::UnitY;
            }

            Base::Vector3d xAxis = ref.Cross(zAxis);
            xAxis.Normalize();

            Base::Vector3d yAxis = zAxis.Cross(xAxis);
            yAxis.Normalize();

            data.principalAxisZ = zAxis;
            data.principalAxisX = xAxis;
            data.principalAxisY = yAxis;
        }
        else {
            gp_Vec v1 = principal.FirstAxisOfInertia();
            gp_Vec v2 = principal.SecondAxisOfInertia();

            gp_Dir Z = Base::convertTo<gp_Dir>(v1);
            gp_Dir X = Base::convertTo<gp_Dir>(v2);
            gp_Dir Y = Z.Crossed(X);

            data.principalAxisZ = Base::convertTo<Base::Vector3d>(Z);
            data.principalAxisX = Base::convertTo<Base::Vector3d>(X);
            data.principalAxisY = Base::convertTo<Base::Vector3d>(Y);
        }

        auto axisMoment = [&](const Base::Vector3d& u) {
            return u.x * (inertia(1, 1) * u.x + inertia(1, 2) * u.y + inertia(1, 3) * u.z)
                + u.y * (inertia(2, 1) * u.x + inertia(2, 2) * u.y + inertia(2, 3) * u.z)
                + u.z * (inertia(3, 1) * u.x + inertia(3, 2) * u.y + inertia(3, 3) * u.z);
        };

        data.inertiaJ = Base::Vector3d(
            axisMoment(data.principalAxisX),
            axisMoment(data.principalAxisY),
            axisMoment(data.principalAxisZ)
        );
    }
    else if (mode == MassPropertiesMode::Custom) {
        if (!referenceDatum) {
            return data;
        }

        const App::LocalCoordinateSystem* lcs = nullptr;
        if (auto referenceDatumElement = freecad_cast<App::DatumElement const*>(referenceDatum)) {
            lcs = referenceDatumElement->getLCS();
        }


        if (auto const* line = freecad_cast<App::Line const*>(referenceDatum)) {

            Base::Vector3d axisOrigin = line->getBasePoint();
            Base::Vector3d axisDir = line->getDirection();
            axisDir.Normalize();

            if (referencePlacement) {
                Base::Vector3d transformedOrigin;
                referencePlacement->multVec(axisOrigin, transformedOrigin);
                axisOrigin = transformedOrigin;
                axisDir = referencePlacement->getRotation().multVec(axisDir);
                axisDir.Normalize();
            }

            Base::Vector3d r(
                data.cog.x - axisOrigin.x,
                data.cog.y - axisOrigin.y,
                data.cog.z - axisOrigin.z
            );

            double projection = r.Dot(axisDir);
            Base::Vector3d parallel = axisDir * projection;

            Base::Vector3d perpendicular = r - parallel;
            double d = perpendicular.Length();

            double I_cog_axis = inertia(1, 1) * axisDir.x * axisDir.x
                + inertia(2, 2) * axisDir.y * axisDir.y + inertia(3, 3) * axisDir.z * axisDir.z
                + 2.0 * inertia(1, 2) * axisDir.x * axisDir.y
                + 2.0 * inertia(1, 3) * axisDir.x * axisDir.z
                + 2.0 * inertia(2, 3) * axisDir.y * axisDir.z;

            data.axisInertia = I_cog_axis + data.mass.getValue() * d * d;

            return data;
        }

        if (!lcs) {
            lcs = freecad_cast<const App::LocalCoordinateSystem*>(referenceDatum);
            if (!lcs) {
                return data;
            }
        }


        Base::Placement placement = lcs->Placement.getValue();
        if (referencePlacement) {
            placement = *referencePlacement;
        }
        Base::Vector3d customOrigin = placement.getPosition();
        gp_Mat R = Part::Tools::fromPlacement(placement).Transformation().VectorialPart();

        double dx = data.cog.x - customOrigin.x;
        double dy = data.cog.y - customOrigin.y;
        double dz = data.cog.z - customOrigin.z;

        double m = globalMassProps.Mass();

        double r_dot_r = dx * dx + dy * dy + dz * dz;

        gp_Mat I_parallel;

        I_parallel.SetValue(1, 1, m * (r_dot_r - dx * dx));
        I_parallel.SetValue(2, 2, m * (r_dot_r - dy * dy));
        I_parallel.SetValue(3, 3, m * (r_dot_r - dz * dz));


        I_parallel.SetValue(1, 2, -m * dx * dy);
        I_parallel.SetValue(2, 1, -m * dx * dy);
        I_parallel.SetValue(1, 3, -m * dx * dz);
        I_parallel.SetValue(3, 1, -m * dx * dz);
        I_parallel.SetValue(2, 3, -m * dy * dz);
        I_parallel.SetValue(3, 2, -m * dy * dz);

        gp_Mat I_translated = inertia.Added(I_parallel);

        gp_Mat R_transpose = R.Transposed();
        gp_Mat I_temp = R_transpose.Multiplied(I_translated);
        gp_Mat I_custom = I_temp.Multiplied(R);

        data.inertiaJo = Base::Vector3d(I_custom(1, 1), I_custom(2, 2), I_custom(3, 3));
        data.inertiaJCross = Base::Vector3d(I_custom(1, 2), I_custom(1, 3), I_custom(2, 3));

        math_Matrix I_mat(1, 3, 1, 3);
        for (int r = 1; r <= 3; r++) {
            for (int c = 1; c <= 3; c++) {
                I_mat(r, c) = I_custom.Value(r, c);
            }
        }

        math_Jacobi jacobi(I_mat);

        data.inertiaJ = Base::Vector3d(jacobi.Value(1), jacobi.Value(2), jacobi.Value(3));

        math_Vector v1(1, 3);
        math_Vector v2(1, 3);
        math_Vector v3(1, 3);

        jacobi.Vector(1, v1);
        jacobi.Vector(2, v2);
        jacobi.Vector(3, v3);

        data.principalAxisX = Base::Vector3d(v1(1), v1(2), v1(3));
        data.principalAxisY = Base::Vector3d(v2(1), v2(2), v2(3));
        data.principalAxisZ = Base::Vector3d(v3(1), v3(2), v3(3));
    }

    return data;
}
