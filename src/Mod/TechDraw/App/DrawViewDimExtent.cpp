/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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
# include <sstream>
# include <cstring>
# include <cstdlib>
# include <exception>
# include <QString>
# include <QStringList>
# include <QRegExp>

#endif

#include <QLocale>

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Base/Quantity.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>


#include "Geometry.h"
#include "DrawViewPart.h"
#include "DrawViewDimExtent.h"
#include "DrawDimHelper.h"
#include "DrawUtil.h"
#include "LineGroup.h"

#include <Mod/TechDraw/App/DrawViewDimExtentPy.h>  // generated from DrawViewDimExtentPy.xml

using namespace TechDraw;

//===========================================================================
// DrawViewDimExtent
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawViewDimExtent, TechDraw::DrawViewDimension)

DrawViewDimExtent::DrawViewDimExtent(void)
{
    App::PropertyLinkSubList       Source;                       //DrawViewPart & SubElements(Edges)
                                                                 //Cosmetic End points are stored in DVD::References2d
    App::PropertyLinkSubList       Source3d;                     //Part::Feature & SubElements  TBI

    ADD_PROPERTY_TYPE(Source,(0,0),"",(App::PropertyType)(App::Prop_Output),"View (Edges) to dimension");
    Source.setScope(App::LinkScope::Global);
    ADD_PROPERTY_TYPE(Source3d,(0,0),"",(App::PropertyType)(App::Prop_Output),"View (Edges) to dimension");   //TBI
    Source3d.setScope(App::LinkScope::Global);
    ADD_PROPERTY_TYPE(DirExtent ,(0),"",App::Prop_Output,"Horizontal / Vertical");

    ADD_PROPERTY_TYPE(CosmeticTags ,(""),"",App::Prop_Output,"Id of cosmetic endpoints");

    //hide the properties the user can't edit in the property editor
    Source3d.setStatus(App::Property::Hidden,true);   //TBI

}

DrawViewDimExtent::~DrawViewDimExtent()
{
}

void DrawViewDimExtent::onChanged(const App::Property* prop)
{
    if (!isRestoring()) {
        if (prop == &Source) {
//            Base::Console().Message("DVDE::onChanged - Source: %X\n", Source.getValue());
            //recalculate the points?
        }
    }
    DrawViewDimension::onChanged(prop);
}

short DrawViewDimExtent::mustExecute() const
{
    return DrawViewDimension::mustExecute();
}

App::DocumentObjectExecReturn *DrawViewDimExtent::execute(void)
{
//    Base::Console().Message("DVDE::execute() - %s\n", getNameInDocument());
    if (!keepUpdated()) {
        return App::DocumentObject::StdReturn;
    }

    App::DocumentObject* docObj = Source.getValue();
    if (docObj == nullptr) {
        return App::DocumentObject::StdReturn;
    }
    DrawViewPart* dvp = dynamic_cast<DrawViewPart*>(docObj);
     if (dvp == nullptr) {
        return App::DocumentObject::StdReturn;
    }

    double tolerance = 0.00001;
    std::vector<std::string> edgeNames = getSubNames();
    int direction = DirExtent.getValue();

    std::pair<Base::Vector3d, Base::Vector3d> endPoints =
                                        DrawDimHelper::minMax(dvp,
                                                              edgeNames,
                                                              direction);
    Base::Vector3d refMin = endPoints.first;
    Base::Vector3d refMax = endPoints.second;

    TechDraw::VertexPtr v0 = nullptr;
    TechDraw::VertexPtr v1 = nullptr;
    std::vector<std::string> cTags = CosmeticTags.getValues();
    if (cTags.size() > 1) {
        v0 = dvp->getProjVertexByCosTag(cTags[0]);
        v1 = dvp->getProjVertexByCosTag(cTags[1]);
        if ( (v0 != nullptr) &&
             (v1 != nullptr) ) {
            double length00 = (v0->pnt - refMin).Length();
            double length11 = (v1->pnt - refMax).Length();
            double length01 = (v0->pnt - refMax).Length();
            double length10 = (v1->pnt - refMin).Length();
            if (  ((length00 < tolerance) &&
                   (length11 < tolerance))  ||
                  ((length01 < tolerance) &&
                   (length10 < tolerance))  ) {
                //nothing changed - nop
            } else {
                //update GV
                v0->pnt = refMin;
                v1->pnt = refMax;
        //        v0->occVertex = ???
        //        v1->occVertex = ???
                //update CV
                double scale = dvp->getScale();
                CosmeticVertex* cvTemp = dvp->getCosmeticVertex(cTags[0]);
                cvTemp->permaPoint = refMin / scale;
                cvTemp = dvp->getCosmeticVertex(cTags[1]);
                cvTemp->permaPoint = refMax / scale;
            }
        }
    }

    return DrawViewDimension::execute();
}

//getSubValues returns a garbage 1st entry if there are no subelements.
std::vector<std::string> DrawViewDimExtent::getSubNames(void)
{
    std::vector<std::string> result;
    std::vector<std::string> edgeNames = Source.getSubValues();
    if (!edgeNames.empty() &&
         (edgeNames[0].size() == 0)) {
         //garbage first entry - nop
    } else {
        result = edgeNames;
    }
    return result;
}

pointPair DrawViewDimExtent::getPointsTwoVerts()
{
//    Base::Console().Message("DVDE::getPointsTwoVerts() - %s\n",getNameInDocument());
    pointPair result;
    result.first = Base::Vector3d(0.0, 0.0, 0.0);
    result.second = Base::Vector3d(0.0, 0.0, 0.0);
    TechDraw::VertexPtr v0 = nullptr;
    TechDraw::VertexPtr v1 = nullptr;
    TechDraw::DrawViewPart* dvp = getViewPart();
    if (dvp == nullptr) {
        return result;
    }

    std::vector<std::string> cTags = CosmeticTags.getValues();
    if (cTags.size() > 1) {
        v0 = dvp->getProjVertexByCosTag(cTags[0]);
        v1 = dvp->getProjVertexByCosTag(cTags[1]);
        if ( (v0 != nullptr) &&
             (v1 != nullptr) ) {
            result.first  = v0->pnt;
            result.second = v1->pnt;
        }
    }
    return result;
}

//! validate 2D references - only checks if the target exists
bool DrawViewDimExtent::checkReferences2D() const
{
//    Base::Console().Message("DVDE::checkReFerences2d() - %s\n",getNameInDocument());
    bool result = false;
    TechDraw::DrawViewPart* dvp = getViewPart();
    if (dvp == nullptr) {
        return result;
    }

    std::vector<std::string> cTags = CosmeticTags.getValues();
    if (cTags.size() > 1) {
        CosmeticVertex* cv0 = dvp->getCosmeticVertex(cTags[0]);
        CosmeticVertex* cv1 = dvp->getCosmeticVertex(cTags[1]);
        if ( (cv0 != nullptr) &&
             (cv1 != nullptr) ) {
            result = true;
        }
    }
    return result;
}

void DrawViewDimExtent::unsetupObject()
{
//    bool isRemoving = testStatus(App::ObjectStatus::Remove);
//    Base::Console().Message("DVDE::unsetupObject - isRemove: %d status: %X\n",
//                            isRemoving, getStatus());
    TechDraw::DrawViewPart* dvp = getViewPart();

    std::vector<std::string> cTags = CosmeticTags.getValues();
    dvp->removeCosmeticVertex(cTags);
    DrawViewDimension::unsetupObject();

    //dvp probably needs recomp/repaint here.
    dvp->enforceRecompute();
}

PyObject *DrawViewDimExtent::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawViewDimExtentPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}
