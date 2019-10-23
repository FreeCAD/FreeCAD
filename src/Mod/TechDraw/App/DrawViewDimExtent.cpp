/***************************************************************************
 *   Copyright (c) 2019 Wandererfan <wandererfan@gmail.com>                *
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

    TechDraw::Vertex* v0 = nullptr;
    TechDraw::Vertex* v1 = nullptr;
    const std::vector<std::string> &subElements = References2D.getSubValues();
    if (subElements.size() > 1) {
        int idx0 = DrawUtil::getIndexFromName(subElements[0]);
        int idx1 = DrawUtil::getIndexFromName(subElements[1]);
        v0 = getViewPart()->getProjVertexByIndex(idx0);
        v1 = getViewPart()->getProjVertexByIndex(idx1);

        double length00 = (v0->pnt - refMin).Length();
        double length11 = (v1->pnt - refMax).Length();
        double length01 = (v0->pnt - refMax).Length();
        double length10 = (v1->pnt - refMin).Length();
        if (  ((length00 < tolerance) &&
               (length11 < tolerance))  ||
              ((length01 < tolerance) &&
               (length10 < tolerance))  ) {
        } else {
            //update GV
            v0->pnt = refMin;
            v1->pnt = refMax;
    //        v0->occVertex = ???
    //        v1->occVertex = ???
            //update CV
            double scale = dvp->getScale();
            int cv0 = v0->cosmeticLink;
            CosmeticVertex* cvTemp = dvp->getCosmeticVertexByIndex(cv0);
            cvTemp->permaPoint = refMin / scale;
            int cv1 = v1->cosmeticLink;
            cvTemp = dvp->getCosmeticVertexByIndex(cv1);
            cvTemp->permaPoint = refMax / scale; 
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

void DrawViewDimExtent::unsetupObject()
{
    //CV's need to be deleted, but deleting messes up all the indices to other CV's!
//    TechDraw::DrawViewPart* dvp = getViewPart();
//    std::vector<TechDraw::CosmeticVertex*> cVerts = dvp->CosmeticVertexes.getValues();
//    Base::Console().Message("DVDE::unsetupObject() - cVerts: %d\n", cVerts.size());
//    TechDraw::Vertex* v0 = nullptr;
//    TechDraw::Vertex* v1 = nullptr;
//    const std::vector<std::string> &subElements = References2D.getSubValues();
//    if (subElements.size() > 1) {
//        Base::Console().Message("DVDE::unsetupObject - more than 1 subElement\n");
//        int idx1 = DrawUtil::getIndexFromName(subElements[1]);
//        v1 = dvp->getProjVertexByIndex(idx1);
//        if (v1 != nullptr) {
//            int cv1 = v1->cosmeticLink;
//            dvp->removeCosmeticVertex(cv1);
//        }

//        int idx0 = DrawUtil::getIndexFromName(subElements[0]);
//        v0 = dvp->getProjVertexByIndex(idx0);
//        if (v0 != nullptr) {
//            int cv0 = v0->cosmeticLink;
//            dvp->removeCosmeticVertex(cv0);
//        }
//    }
//    cVerts = dvp->CosmeticVertexes.getValues();
//    Base::Console().Message("DVDE::unsetupObject - exit - cVerts: %d\n", cVerts.size());
    DrawViewDimension::unsetupObject();
//    App::DocumentObject::unsetUpObject();
}

PyObject *DrawViewDimExtent::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawViewDimExtentPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}
