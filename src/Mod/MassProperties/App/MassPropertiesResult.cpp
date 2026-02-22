// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   © 2026 Morten Vajhøj                                                     *
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
#include <Base/Matrix.h>
#include <Base/Quantity.h>

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
    std::string& mode,
    const App::DocumentObject* referenceDatum,
    const Base::Placement* referencePlacement
)
{
    MassPropertiesData data{};
    if (objects.empty())
        return data;

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
            if (!obj || !obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
                continue;
            }
            Part::Feature* part = static_cast<Part::Feature*>(obj);
            shape = part->Shape.getShape().getShape();
        }

        if (shape.IsNull()) {
            continue;
        }

        if (!object.placement.isIdentity()) {
            Base::Matrix4D matrix = object.placement.toMatrix();
            gp_Trsf trsf;
            trsf.SetValues(
                matrix[0][0], matrix[0][1], matrix[0][2], matrix[0][3],
                matrix[1][0], matrix[1][1], matrix[1][2], matrix[1][3],
                matrix[2][0], matrix[2][1], matrix[2][2], matrix[2][3]
            );
            BRepBuilderAPI_Transform transformer(shape, trsf, true);
            shape = transformer.Shape();
        }

        double density = 1.0e-6;

        auto materialFeature = [](App::DocumentObject* candidate) -> Part::Feature* {
            std::unordered_set<App::DocumentObject*> visited;

            while (candidate && visited.insert(candidate).second) {
                if (candidate->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
                    return static_cast<Part::Feature*>(candidate);
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
        if (part) {
            mat = part->ShapeMaterial.getValue();
        }
        if (mat.hasPhysicalProperty(QStringLiteral("Density"))) {
            try {
                if (mat.getName() == QStringLiteral("Default")) {
                    density = 1.0e-6;
                }
                else {
                    density = mat.getPhysicalQuantity(QStringLiteral("Density")).getValue();
                }
            } catch (...) {}
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

    if (!hasShape)
        return data;

    data.volume = totalVolume;
    data.mass = globalMassProps.Mass();
    data.surfaceArea = globalSurfaceProps.Mass();

    if (data.volume > 0.0)
        data.density = data.mass / data.volume;

    gp_Pnt cog = globalMassProps.CentreOfMass();
    data.cogX = cog.X();
    data.cogY = cog.Y();
    data.cogZ = cog.Z();

    gp_Pnt cov = globalVolumeProps.CentreOfMass();
    data.covX = cov.X();
    data.covY = cov.Y();
    data.covZ = cov.Z();


    gp_Mat inertia = globalMassProps.MatrixOfInertia();

    GProp_PrincipalProps principal;
    
    if (mode == "Center of gravity") {
        data.inertiaJox = inertia(1, 1);
        data.inertiaJoy = inertia(2, 2);
        data.inertiaJoz = inertia(3, 3);
        data.inertiaJxy = inertia(1, 2);
        data.inertiaJzx = inertia(1, 3);
        data.inertiaJzy = inertia(2, 3);

        principal = globalMassProps.PrincipalProperties();

        if (principal.HasSymmetryPoint()) {
            data.principalAxisX = Base::Vector3d::UnitX;
            data.principalAxisY = Base::Vector3d::UnitY;
            data.principalAxisZ = Base::Vector3d::UnitZ;
        }
        else if (principal.HasSymmetryAxis()) {
            gp_Vec zVec = principal.FirstAxisOfInertia();
            Base::Vector3d zAxis(zVec.X(), zVec.Y(), zVec.Z());
            zAxis.Normalize();

            Base::Vector3d ref(0.0, 0.0, 1.0);
            if (std::fabs(zAxis.z) > 0.9) {
                ref = Base::Vector3d(0.0, 1.0, 0.0);
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

            gp_Dir Z(v1);
            gp_Dir X(v2);
            gp_Dir Y = Z.Crossed(X);

            data.principalAxisZ = Base::Vector3d(Z.X(), Z.Y(), Z.Z());
            data.principalAxisX = Base::Vector3d(X.X(), X.Y(), X.Z());
            data.principalAxisY = Base::Vector3d(Y.X(), Y.Y(), Y.Z());
        }

        auto axisMoment = [&](const Base::Vector3d& u) {
            return u.x * (inertia(1, 1) * u.x + inertia(1, 2) * u.y + inertia(1, 3) * u.z)
                 + u.y * (inertia(2, 1) * u.x + inertia(2, 2) * u.y + inertia(2, 3) * u.z)
                 + u.z * (inertia(3, 1) * u.x + inertia(3, 2) * u.y + inertia(3, 3) * u.z);
        };

        data.inertiaJx = axisMoment(data.principalAxisX);
        data.inertiaJy = axisMoment(data.principalAxisY);
        data.inertiaJz = axisMoment(data.principalAxisZ);
    }
    else if (mode == "Custom") {
        if (!referenceDatum) {
            return data;
        }

        const App::LocalCoordinateSystem* lcs = nullptr;
        if (auto referenceDatumElement = dynamic_cast<const App::DatumElement*>(referenceDatum)) {
            lcs = referenceDatumElement->getLCS();
        }
        
        
        if (referenceDatum->isDerivedFrom<App::Line>()) {
            App::Line const* line = static_cast<App::Line const*>(referenceDatum);
    
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
            
            Base::Vector3d r(cog.X() - axisOrigin.x, cog.Y() - axisOrigin.y, cog.Z() - axisOrigin.z);
            
            double projection = r.Dot(axisDir);
            Base::Vector3d parallel = axisDir * projection;
            
            Base::Vector3d perpendicular = r - parallel;
            double d = perpendicular.Length();
            
            double I_cog_axis = inertia(1,1) * axisDir.x * axisDir.x +
                                inertia(2,2) * axisDir.y * axisDir.y +
                                inertia(3,3) * axisDir.z * axisDir.z +
                                2.0 * inertia(1,2) * axisDir.x * axisDir.y +
                                2.0 * inertia(1,3) * axisDir.x * axisDir.z +
                                2.0 * inertia(2,3) * axisDir.y * axisDir.z;
            
            data.axisInertia = I_cog_axis + data.mass * d * d;
            
            return data;
        }
        
        if (!lcs) {
            lcs = dynamic_cast<const App::LocalCoordinateSystem*>(referenceDatum);
            if (!lcs) {
                return data;
            }
        }
        

        Base::Placement placement = lcs->Placement.getValue();
        if (referencePlacement) {
            placement = *referencePlacement;
        }
        Base::Vector3d customOrigin = placement.getPosition();
        
        Base::Matrix4D transform = placement.toMatrix();
        
        gp_Mat R;
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                R.SetValue(i + 1, j + 1, transform[i][j]);
            }
        }
        
        double dx = cog.X() - customOrigin.x;
        double dy = cog.Y() - customOrigin.y;
        double dz = cog.Z() - customOrigin.z;
        
        double m = globalMassProps.Mass();
        
        double r_dot_r = dx*dx + dy*dy + dz*dz;
        
        gp_Mat I_parallel;

        I_parallel.SetValue(1, 1, m * (r_dot_r - dx*dx));
        I_parallel.SetValue(2, 2, m * (r_dot_r - dy*dy));
        I_parallel.SetValue(3, 3, m * (r_dot_r - dz*dz));
        

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
        
        data.inertiaJox = I_custom(1, 1);
        data.inertiaJoy = I_custom(2, 2);
        data.inertiaJoz = I_custom(3, 3);
        data.inertiaJxy = I_custom(1, 2);
        data.inertiaJzx = I_custom(1, 3);
        data.inertiaJzy = I_custom(2, 3);
        
        math_Matrix I_mat(1, 3, 1, 3);
        for (int r = 1; r <= 3; r++)
            for (int c = 1; c <= 3; c++)
                I_mat(r, c) = I_custom.Value(r, c);
        
        math_Jacobi jacobi(I_mat);
        
        data.inertiaJx = jacobi.Value(1);
        data.inertiaJy = jacobi.Value(2);
        data.inertiaJz = jacobi.Value(3);
        
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
