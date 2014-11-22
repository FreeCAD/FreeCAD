/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel (juergen.riegel@web.de)              *
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


#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include <sstream>

#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Mesh/App/MeshFeature.h>
#include <Mod/Mesh/App/Core/Approximation.h>

#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Gui/FileDialog.h>
#include <Gui/Selection.h>
#include <Base/CoordinateSystem.h>

#include "../App/ApproxSurface.h"

using namespace std;

DEF_STD_CMD_A(CmdApproxSurface);

CmdApproxSurface::CmdApproxSurface()
  : Command("Reen_ApproxSurface")
{
    sAppModule      = "Reen";
    sGroup          = QT_TR_NOOP("Reverse Engineering");
    sMenuText       = QT_TR_NOOP("Approximate surface...");
    sToolTipText    = QT_TR_NOOP("Approximate a B-Spline surface");
    sWhatsThis      = "Reen_ApproxSurface";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/FitSurface";
}

void CmdApproxSurface::activated(int iMsg)
{
    std::vector<App::DocumentObject*> obj = Gui::Selection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    const Mesh::MeshObject& mesh = static_cast<Mesh::Feature*>(obj[0])->Mesh.getValue();
    if (mesh.countSegments() > 0) {
        const Mesh::Segment& segm = mesh.getSegment(0);
        const std::vector<unsigned long>& inds = segm.getIndices();
        MeshCore::MeshFacetIterator f_iter(mesh.getKernel());
        std::set<unsigned long> points;
        for (std::vector<unsigned long>::const_iterator it = inds.begin(); it != inds.end(); ++it) {
            f_iter.Set(*it);
            MeshCore::MeshFacet face = f_iter.GetIndices();
            points.insert(face._aulPoints[0]);
            points.insert(face._aulPoints[1]);
            points.insert(face._aulPoints[2]);
        }

        std::stringstream str;
        str << "__points=[]" << std::endl;
        for (std::set<unsigned long>::iterator it=points.begin(); it != points.end(); ++it) {
            Mesh::MeshPoint p = mesh.getPoint(*it);
            str << "__points.append((" << p.x << "," << p.y << "," << p.z << "))" << std::endl;
        }

        str << "import ReverseEngineering" << std::endl;
        str << "__spline = ReverseEngineering.approxSurface(__points)" << std::endl;
        str << "App.ActiveDocument.addObject(\"Part::Feature\",\"Surface\").Shape"
               "=__spline.toShape(0.0,1.0,0.0,1.0)" << std::endl;
        str << "App.ActiveDocument.recompute()" << std::endl;
        str << "del __points" << std::endl;
        str << "del __spline" << std::endl;
        
        openCommand("Fit surface");
        doCommand(Gui::Command::Doc, str.str().c_str());
        commitCommand(); 
        updateActive();
    }
}

bool CmdApproxSurface::isActive(void)
{
    if (getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) == 1)
        return true;
    return false;
}

DEF_STD_CMD_A(CmdApproxPlane);

CmdApproxPlane::CmdApproxPlane()
  : Command("Reen_ApproxPlane")
{
    sAppModule      = "Reen";
    sGroup          = QT_TR_NOOP("Reverse Engineering");
    sMenuText       = QT_TR_NOOP("Approximate plane...");
    sToolTipText    = QT_TR_NOOP("Approximate a plane");
    sWhatsThis      = "Reen_ApproxPlane";
    sStatusTip      = sToolTipText;
}

void CmdApproxPlane::activated(int iMsg)
{
    std::vector<App::GeoFeature*> obj = Gui::Selection().getObjectsOfType<App::GeoFeature>();
    for (std::vector<App::GeoFeature*>::iterator it = obj.begin(); it != obj.end(); ++it) {
        std::map<std::string, App::Property*> Map;
        (*it)->getPropertyMap(Map);
        for (std::map<std::string, App::Property*>::iterator jt = Map.begin(); jt != Map.end(); ++jt) {
            if (jt->second->getTypeId().isDerivedFrom(App::PropertyComplexGeoData::getClassTypeId())) {
                std::vector<Base::Vector3d> aPoints;
                std::vector<Data::ComplexGeoData::Facet> aTopo;
                static_cast<App::PropertyComplexGeoData*>(jt->second)->getFaces(aPoints, aTopo,0.01f);

                // get a reference normal for the plane fit
                Base::Vector3f refNormal(0,0,0);
                if (!aTopo.empty()) {
                    Data::ComplexGeoData::Facet f = aTopo.front();
                    Base::Vector3d v1 = aPoints[f.I2] - aPoints[f.I1];
                    Base::Vector3d v2 = aPoints[f.I3] - aPoints[f.I1];
                    refNormal = Base::convertTo<Base::Vector3f>(v1 % v2);
                }

                std::vector<Base::Vector3f> aData;
                aData.reserve(aPoints.size());
                for (std::vector<Base::Vector3d>::iterator jt = aPoints.begin(); jt != aPoints.end(); ++jt)
                    aData.push_back(Base::toVector<float>(*jt));
                MeshCore::PlaneFit fit;
                fit.AddPoints(aData);
                float sigma = fit.Fit();
                Base::Vector3f base = fit.GetBase();
                Base::Vector3f dirU = fit.GetDirU();
                Base::Vector3f dirV = fit.GetDirV();
                Base::Vector3f norm = fit.GetNormal();

                // if the dot product of the reference with the plane normal is negative
                // a flip must be done
                if (refNormal * norm < 0) {
                    norm = -norm;
                    dirU = -dirU;
                }

                float width, length;
                fit.Dimension(width, length);

                // move to the corner point
                base = base - (0.5f * length * dirU + 0.5f * width * dirV);

                Base::CoordinateSystem cs;
                cs.setPosition(Base::convertTo<Base::Vector3d>(base));
                cs.setAxes(Base::convertTo<Base::Vector3d>(norm),
                           Base::convertTo<Base::Vector3d>(dirU));
                Base::Placement pm = Base::CoordinateSystem().displacement(cs);
                double q0, q1, q2, q3;
                pm.getRotation().getValue(q0, q1, q2, q3);

                Base::Console().Log("RMS value for plane fit with %lu points: %.4f\n", aData.size(), sigma);
                Base::Console().Log("  Plane base(%.4f, %.4f, %.4f)\n", base.x, base.y, base.z);
                Base::Console().Log("  Plane normal(%.4f, %.4f, %.4f)\n", norm.x, norm.y, norm.z);

                std::stringstream str;
                str << "from FreeCAD import Base" << std::endl;
                str << "App.ActiveDocument.addObject('Part::Plane','Plane_fit')" << std::endl;
                str << "App.ActiveDocument.ActiveObject.Length = " << length << std::endl;
                str << "App.ActiveDocument.ActiveObject.Width = " << width << std::endl;
                str << "App.ActiveDocument.ActiveObject.Placement = Base.Placement("
                    << "Base.Vector(" << base.x << "," << base.y << "," << base.z << "),"
                    << "Base.Rotation(" << q0 << "," << q1 << "," << q2 << "," << q3 << "))" << std::endl;
                
                openCommand("Fit plane");
                doCommand(Gui::Command::Doc, str.str().c_str());
                commitCommand(); 
                updateActive();
            }
        }
    }
}

bool CmdApproxPlane::isActive(void)
{
    if (getSelection().countObjectsOfType(App::GeoFeature::getClassTypeId()) == 1)
        return true;
    return false;
}

void CreateReverseEngineeringCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();
    rcCmdMgr.addCommand(new CmdApproxSurface());
    rcCmdMgr.addCommand(new CmdApproxPlane());
}
