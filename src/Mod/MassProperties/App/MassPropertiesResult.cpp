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
#include <Mod/Part/App/TopoShape.h>

#include <Base/Quantity.h>

#include <TopoDS_Shape.hxx>
#include <GProp_GProps.hxx>
#include <BRepGProp.hxx>
#include <GProp_PrincipalProps.hxx>
#include <gp_Pnt.hxx>
#include <gp_Mat.hxx>
#include <gp_Ax3.hxx>
#include <math_Jacobi.hxx>
#include <math_Vector.hxx>

#include <map>
#include <string>
#include <vector>
#include <cmath>
#include <QString>

MassPropertiesData CalculateMassProperties(const std::vector<App::DocumentObject*>& objects, std::string& mode, const App::DocumentObject* referenceDatum)
{
    MassPropertiesData data{};
    if (objects.empty())
        return data;

    GProp_GProps globalMassProps;
    GProp_GProps globalSurfaceProps;
    GProp_GProps globalVolumeProps;

    double totalVolume = 0.0;
    bool hasShape = false;

    for (App::DocumentObject* obj : objects) {
        if (!obj)
            continue;

        if (!obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
            continue;

        Part::Feature* part = static_cast<Part::Feature*>(obj);
        const TopoDS_Shape& shape = part->Shape.getShape().getShape();

        if (shape.IsNull())
            continue;

        double density = 1.0e-6;

        Materials::Material mat = part->ShapeMaterial.getValue();
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
        data.density = (data.mass / data.volume) * 1.0e9;

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
    
    if (mode == "Center of Gravity") {
        data.inertiaJox = inertia(1, 1);
        data.inertiaJoy = inertia(2, 2);
        data.inertiaJoz = inertia(3, 3);
        data.inertiaJxy = inertia(1, 2);
        data.inertiaJzx = inertia(1, 3);
        data.inertiaJzy = inertia(2, 3);

        principal = globalMassProps.PrincipalProperties();

        if (principal.HasSymmetryPoint()) {
            data.principalAxisX = { 1.0, 0.0, 0.0 };
            data.principalAxisY = { 0.0, 1.0, 0.0 };
            data.principalAxisZ = { 0.0, 0.0, 1.0 };
        }
        else if (principal.HasSymmetryAxis()) {
            gp_Vec zVec = principal.FirstAxisOfInertia();
            Vec3 Z = { zVec.X(), zVec.Y(), zVec.Z() };

            double lenZ = std::sqrt(Z.x * Z.x + Z.y * Z.y + Z.z * Z.z);
            if (lenZ > 0.0) {
                Z.x /= lenZ;
                Z.y /= lenZ;
                Z.z /= lenZ;
            }


            Vec3 ref = { 0.0, 0.0, 1.0 };
            if (std::fabs(Z.z) > 0.9) {
                ref = { 0.0, 1.0, 0.0 };
            }
            Vec3 X = { ref.y * Z.z - ref.z * Z.y,
                       ref.z * Z.x - ref.x * Z.z,
                       ref.x * Z.y - ref.y * Z.x };

            double lenX = std::sqrt(X.x*X.x + X.y*X.y + X.z*X.z);
            if (lenX > 0.0) {
                X.x /= lenX;
                X.y /= lenX;
                X.z /= lenX;
            }

            Vec3 Y = { Z.y*X.z - Z.z*X.y, Z.z*X.x - Z.x*X.z, Z.x*X.y - Z.y*X.x };
            double lenY = std::sqrt(Y.x*Y.x + Y.y*Y.y + Y.z*Y.z);
            if (lenY > 0.0) {
                Y.x /= lenY;
                Y.y /= lenY;
                Y.z /= lenY;
            }
            
            data.principalAxisZ = Z;
            data.principalAxisX = X;
            data.principalAxisY = Y;
        }
        else {
            gp_Vec v1 = principal.FirstAxisOfInertia();
            gp_Vec v2 = principal.SecondAxisOfInertia();
            gp_Vec v3 = principal.ThirdAxisOfInertia();
    
            gp_Dir Z(v1);
            gp_Dir X(v2);
            gp_Dir Y = Z.Crossed(X);
    
            data.principalAxisZ = { Z.X(), Z.Y(), Z.Z() };
            data.principalAxisX = { X.X(), X.Y(), X.Z() };
            data.principalAxisY = { Y.X(), Y.Y(), Y.Z() };
        }

        auto axisMoment = [&](const Vec3& u) {
            return u.x * (inertia(1, 1) * u.x + inertia(1, 2) * u.y + inertia(1, 3) * u.z)
                 + u.y * (inertia(2, 1) * u.x + inertia(2, 2) * u.y + inertia(2, 3) * u.z)
                 + u.z * (inertia(3, 1) * u.x + inertia(3, 2) * u.y + inertia(3, 3) * u.z);
        };

        data.inertiaJx = axisMoment(data.principalAxisX);
        data.inertiaJy = axisMoment(data.principalAxisY);
        data.inertiaJz = axisMoment(data.principalAxisZ);
    }
    else if (mode == "Global") {
        double dx2 = cog.X() * cog.X();
        double dy2 = cog.Y() * cog.Y();
        double dz2 = cog.Z() * cog.Z();

        double cx = cog.X();
        double cy = cog.Y();
        double cz = cog.Z();

        data.inertiaJox = inertia(1, 1) + globalMassProps.Mass() * (dy2 + dz2);
        data.inertiaJoy = inertia(2, 2) + globalMassProps.Mass() * (dx2 + dz2);
        data.inertiaJoz = inertia(3, 3) + globalMassProps.Mass() * (dx2 + dy2);
        data.inertiaJxy = inertia(1, 2) - globalMassProps.Mass() * cx * cy;
        data.inertiaJzx = inertia(1, 3) - globalMassProps.Mass() * cx * cz;
        data.inertiaJzy = inertia(2, 3) - globalMassProps.Mass() * cy * cz;
        
        gp_Mat Iglobal;
        Iglobal.SetValue(1, 1, data.inertiaJox);
        Iglobal.SetValue(2, 2, data.inertiaJoy);
        Iglobal.SetValue(3, 3, data.inertiaJoz);

        Iglobal.SetValue(1, 2, data.inertiaJxy);
        Iglobal.SetValue(2, 1, data.inertiaJxy);
        
        Iglobal.SetValue(1, 3, data.inertiaJzx);
        Iglobal.SetValue(3, 1, data.inertiaJzx);
        
        Iglobal.SetValue(2, 3, data.inertiaJzy);
        Iglobal.SetValue(3, 2, data.inertiaJzy);

        math_Matrix I(1,3,1,3);
        for (int r=1;r<=3;r++)
            for (int c=1;c<=3;c++)
                I(r,c) = Iglobal.Value(r,c);
        
        math_Jacobi jacobi(I);
        
        data.inertiaJx = jacobi.Value(1);
        data.inertiaJy = jacobi.Value(2);
        data.inertiaJz = jacobi.Value(3);

        math_Vector v1(1, 3);
        math_Vector v2(1, 3);
        math_Vector v3(1, 3);
        
        jacobi.Vector(1, v1);
        jacobi.Vector(2, v2);
        jacobi.Vector(3, v3);
        
        data.principalAxisX = { v1(1), v1(2), v1(3) };
        data.principalAxisY = { v2(1), v2(2), v2(3) };
        data.principalAxisZ = { v3(1), v3(2), v3(3) };
    }
    else if (mode == "Custom") {
        if (!referenceDatum) {
            return data;
        }

        auto referenceDatumElement = static_cast<const App::DatumElement*>(referenceDatum);
        
        
        if (referenceDatum->isDerivedFrom<App::Line>()) {
            App::Line const* line = static_cast<App::Line const*>(referenceDatum);
    
            Base::Vector3d axisOrigin = line->getBasePoint();
            Base::Vector3d axisDir = line->getDirection();
            axisDir.Normalize();
            
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

        auto lcs = referenceDatumElement->getLCS();
        if (!lcs) {
            return data;
        }

        Base::Placement placement = lcs->Placement.getValue();
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
        
        data.principalAxisX = { v1(1), v1(2), v1(3) };
        data.principalAxisY = { v2(1), v2(2), v2(3) };
        data.principalAxisZ = { v3(1), v3(2), v3(3) };
    }

    return data;
}
