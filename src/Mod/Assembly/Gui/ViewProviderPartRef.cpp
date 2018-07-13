/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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
# include <Inventor/nodes/SoGroup.h>
#include <Inventor/nodes/SoAnnotation.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoDrawStyle.h>
#endif

#include "ViewProviderPartRef.h"
//#include <Gui/Command.h>
//#include <Gui/Document.h>
#include <Mod/Assembly/App/PartRef.h>
#include <Base/Console.h>

using namespace AssemblyGui;

#ifdef ASSEMBLY_DEBUG_FACILITIES
SbColor PointColor(1.0f,0.0f,0.0f);
SbColor PseudoColor(0.0f,0.0f,1.0f);
SbColor MidpointColor(0.0f,1.0f,1.0f);
SbColor ZeroColor(1.0f,1.0f,0.0f);
#endif

PROPERTY_SOURCE(AssemblyGui::ViewProviderPartRef,AssemblyGui::ViewProviderItem)

ViewProviderPartRef::ViewProviderPartRef()
{
    sPixmap = "Assembly_Assembly_Part_Tree.svg";

#ifdef ASSEMBLY_DEBUG_FACILITIES
    ADD_PROPERTY(ShowScalePoints,(false));
#endif
}

ViewProviderPartRef::~ViewProviderPartRef()
{
}

bool ViewProviderPartRef::doubleClicked(void)
{
    return true;
}

void ViewProviderPartRef::attach(App::DocumentObject* pcFeat)
{
    // call parent attach method
    ViewProviderGeometryObject::attach(pcFeat);
    // putting all together with the switch
    addDisplayMaskMode(getChildRoot(), "Main");

#ifdef ASSEMBLY_DEBUG_FACILITIES

    m_anno = new SoAnnotation;
    m_switch = new SoSwitch;
    m_switch->addChild(m_anno);

    m_material = new SoMaterial;
    m_anno->addChild(m_material);

    SoMaterialBinding* MtlBind = new SoMaterialBinding;
    MtlBind->value = SoMaterialBinding::PER_VERTEX;
    m_anno->addChild(MtlBind);

    m_pointsCoordinate = new SoCoordinate3;
    m_anno->addChild(m_pointsCoordinate);

    SoDrawStyle* DrawStyle = new SoDrawStyle;
    DrawStyle->pointSize = 8;
    m_anno->addChild(DrawStyle);
    m_points = new SoMarkerSet;
    m_points->markerIndex = SoMarkerSet::CIRCLE_FILLED_7_7;
    m_anno->addChild(m_points);

    pcRoot->addChild(m_switch);
#endif
}

void ViewProviderPartRef::setDisplayMode(const char* ModeName)
{
    if(strcmp("Main",ModeName)==0)
        setDisplayMaskMode("Main");

    ViewProviderGeometryObject::setDisplayMode(ModeName);
}

std::vector<std::string> ViewProviderPartRef::getDisplayModes(void) const
{
    // get the modes of the father
    std::vector<std::string> StrList = ViewProviderGeometryObject::getDisplayModes();

    // add your own modes
    StrList.push_back("Main");

    return StrList;
}

std::vector<App::DocumentObject*> ViewProviderPartRef::claimChildren(void)const
{
    std::vector<App::DocumentObject*> res;

    res.insert(res.end(), static_cast<Assembly::PartRef*>(getObject())->Annotation.getValues().begin(),static_cast<Assembly::PartRef*>(getObject())->Annotation.getValues().end());

    if(static_cast<Assembly::PartRef*>(getObject())->Model.getValue())
        res.push_back(static_cast<Assembly::PartRef*>(getObject())->Model.getValue());

    return res;

}

std::vector<App::DocumentObject*> ViewProviderPartRef::claimChildren3D(void)const
{
    std::vector<App::DocumentObject*> res;

    res.insert(res.end(), static_cast<Assembly::PartRef*>(getObject())->Annotation.getValues().begin(),static_cast<Assembly::PartRef*>(getObject())->Annotation.getValues().end());

    if(static_cast<Assembly::PartRef*>(getObject())->Model.getValue())
        res.push_back(static_cast<Assembly::PartRef*>(getObject())->Model.getValue());

    return res;

}

bool ViewProviderPartRef::allowDrop(const std::vector<const App::DocumentObject*> &objList,Qt::KeyboardModifiers keys,Qt::MouseButtons mouseBts,const QPoint &pos)
{
    //for( std::vector<const App::DocumentObject*>::const_iterator it = objList.begin();it!=objList.end();++it)
    //    if ((*it)->getTypeId().isDerivedFrom(App::DocumentObjectGroup::getClassTypeId())) {
    //        if (static_cast<App::DocumentObjectGroup*>(getObject())->isChildOf(
    //            static_cast<const App::DocumentObjectGroup*>(*it))) {
    //            return false;
    //        }
    //    }

    return false;
}
void ViewProviderPartRef::drop(const std::vector<const App::DocumentObject*> &objList,Qt::KeyboardModifiers keys,Qt::MouseButtons mouseBts,const QPoint &pos)
{
        //// Open command
        //App::DocumentObjectGroup* grp = static_cast<App::DocumentObjectGroup*>(getObject());
        //App::Document* doc = grp->getDocument();
        //Gui::Document* gui = Gui::Application::Instance->getDocument(doc);
        //gui->openCommand("Move object");
        //for( std::vector<const App::DocumentObject*>::const_iterator it = objList.begin();it!=objList.end();++it) {
        //    // get document object
        //    const App::DocumentObject* obj = *it;
        //    const App::DocumentObjectGroup* par = App::DocumentObjectGroup::getGroupOfObject(obj);
        //    if (par) {
        //        // allow an object to be in one group only
        //        QString cmd;
        //        cmd = QString::fromLatin1("App.getDocument(\"%1\").getObject(\"%2\").removeObject("
        //                          "App.getDocument(\"%1\").getObject(\"%3\"))")
        //                          .arg(QString::fromLatin1(doc->getName()))
        //                          .arg(QString::fromLatin1(par->getNameInDocument()))
        //                          .arg(QString::fromLatin1(obj->getNameInDocument()));
        //        Gui::Application::Instance->runPythonCode(cmd.toUtf8());
        //    }

        //    // build Python command for execution
        //    QString cmd;
        //    cmd = QString::fromLatin1("App.getDocument(\"%1\").getObject(\"%2\").addObject("
        //                      "App.getDocument(\"%1\").getObject(\"%3\"))")
        //                      .arg(QString::fromLatin1(doc->getName()))
        //                      .arg(QString::fromLatin1(grp->getNameInDocument()))
        //                      .arg(QString::fromLatin1(obj->getNameInDocument()));
        //    
        //    Gui::Application::Instance->runPythonCode(cmd.toUtf8());
        //}
        //gui->commitCommand();

}



#ifdef ASSEMBLY_DEBUG_FACILITIES

void ViewProviderPartRef::onChanged(const App::Property* prop) {

    if(prop == &ShowScalePoints) {
        if(ShowScalePoints.getValue()) {
            m_switch->whichChild = 0;

            int counter = 0;
            boost::shared_ptr<Part3D> part = static_cast<Assembly::PartRef*>(getObject())->m_part;

            if(!part) {
                ViewProviderItem::onChanged(prop);
                return;
            }

            dcm::detail::Transform<double,3> transform = part->m_cluster->getProperty<Module3D::type<Solver>::math_prop>().m_transform;
            dcm::detail::Transform<double,3> ssrTransform = part->m_cluster->getProperty<Module3D::type<Solver>::math_prop>().m_ssrTransform;

            dcm::detail::Transform<double,3> trans = ssrTransform.inverse();

            int PseudoSize = part->m_cluster->getProperty<Module3D::type<Solver>::math_prop>().m_pseudo.size();
            typedef dcm::details::ClusterMath<Solver>::Vec Vector;
            Vector& pv = part->m_cluster->getProperty<Module3D::type<Solver>::math_prop>().m_points;

            for(Vector::iterator it = pv.begin(); it != pv.end(); it++) {

                Kernel::Vector3 vec = trans * (*it);
                m_pointsCoordinate->point.set1Value(counter, SbVec3f(vec(0),vec(1),vec(2)));

                if(counter < PseudoSize)
                    m_material->diffuseColor.set1Value(counter, PseudoColor);
                else
                    m_material->diffuseColor.set1Value(counter, PointColor);

                counter++;
            };

            //midpoint
            Kernel::Vector3 midpoint = trans * Kernel::Vector3(0,0,0);

            m_pointsCoordinate->point.set1Value(counter, SbVec3f(midpoint(0),midpoint(1),midpoint(2)));

            m_material->diffuseColor.set1Value(counter, MidpointColor);

            counter++;

            //origin
            Kernel::Vector3 origin = Kernel::Vector3(0,0,0);

            m_pointsCoordinate->point.set1Value(counter, SbVec3f(origin(0),origin(1),origin(2)));

            m_material->diffuseColor.set1Value(counter, ZeroColor);

            counter++;

            m_points->numPoints = counter;

            //test
            boost::shared_ptr<Geometry3D> g = part->m_cluster->getProperty<Module3D::type<Solver>::math_prop>().m_geometry[0];

            std::stringstream str;

            str<<"Global: "<<g->m_global.transpose()<<std::endl;

            str<<"Global TLPoint: "<<(trans * g->getPoint()).transpose()<<std::endl;

            Kernel::Vector3 v = g->m_global.head(3);

            str<<"Local Point : "<<(transform.inverse()*v).transpose()<<std::endl;

            str<<"Local TLPoint: "<<(ssrTransform.inverse()*g->getPoint()).transpose()<<std::endl;

            str<<"PVPoint : "<<(pv[0]).transpose()<<std::endl;

            str<<"Local PVPoint: "<<(ssrTransform.inverse()*pv[0]).transpose()<<std::endl;

            Base::Console().Message(str.str().c_str());

        }
        else {
            m_switch->whichChild = -1;
            m_pointsCoordinate->point.setValues(0, 0, (SbVec3f*)NULL);
            m_material->diffuseColor.setValues(0, 0, (SbColor*)NULL);
            m_points->numPoints = 0;
        }
    };

    ViewProviderItem::onChanged(prop);
}


#endif
