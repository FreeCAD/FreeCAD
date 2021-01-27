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
#include <math.h>
#include <Inventor/nodes/SoGroup.h>
#include <QMessageBox>
#endif

#include "ViewProviderProductRef.h"
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Application.h>

#include <Mod/Assembly/App/Product.h>
#include <Mod/Assembly/App/ProductRef.h>

using namespace AssemblyGui;

extern Assembly::Item* ActiveAsmObject;

PROPERTY_SOURCE(AssemblyGui::ViewProviderProductRef,AssemblyGui::ViewProviderItem)

ViewProviderProductRef::ViewProviderProductRef()
{
  sPixmap = "Assembly_Assembly_Tree.svg";
}

ViewProviderProductRef::~ViewProviderProductRef()
{
    if(getObject() == ActiveAsmObject)
        Gui::Command::doCommand(Gui::Command::Doc,"AssemblyGui.clearActiveAssembly()");
}

bool ViewProviderProductRef::doubleClicked(void)
{
    Gui::Command::assureWorkbench("AssemblyWorkbench");
    Gui::Command::doCommand(Gui::Command::Doc,"AssemblyGui.setActiveAssembly(App.activeDocument().%s)",this->getObject()->getNameInDocument());
    return true;
}

void ViewProviderProductRef::attach(App::DocumentObject* pcFeat)
{
    // call parent attach method
    ViewProviderGeometryObject::attach(pcFeat);


    // putting all together with the switch
    addDisplayMaskMode(getChildRoot(), "Main");
}

void ViewProviderProductRef::setDisplayMode(const char* ModeName)
{
    if(strcmp("Main",ModeName)==0)
        setDisplayMaskMode("Main");

    ViewProviderGeometryObject::setDisplayMode(ModeName);
}

std::vector<std::string> ViewProviderProductRef::getDisplayModes(void) const
{
    // get the modes of the father
    std::vector<std::string> StrList = ViewProviderGeometryObject::getDisplayModes();

    // add your own modes
    StrList.push_back("Main");

    return StrList;
}


std::vector<App::DocumentObject*> ViewProviderProductRef::claimChildren(void)const
{
  App::DocumentObject * obj = static_cast<Assembly::ProductRef*>(getObject())->Item.getValue();
  if (obj){
    std::vector<App::DocumentObject*> ret(1);
    ret[0] = obj;
    return ret;
  }
  else{
    return std::vector<App::DocumentObject*>();
  }
}

std::vector<App::DocumentObject*> ViewProviderProductRef::claimChildren3D(void)const
{
    std::vector<App::DocumentObject*> ret(1);
    ret[0] =  static_cast<Assembly::ProductRef*>(getObject())->Item.getValue();
    return ret;
}

void ViewProviderProductRef::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    ViewProviderItem::setupContextMenu(menu, receiver, member); // call the base class

    //QAction* toggle = menu->addAction(QObject::tr("Rigid subassembly"), receiver, member);
    //toggle->setData(QVariant(1000)); // identifier
    //toggle->setCheckable(true);
    //toggle->setToolTip(QObject::tr("Set if the subassembly shall be solved as on part (rigid) or if all parts of this assembly are solved for themself."));
    //toggle->setStatusTip(QObject::tr("Set if the subassembly shall be solved as on part (rigid) or if all parts of this assembly are solved for themself."));
    //bool prop = static_cast<Assembly::Product*>(getObject())->Rigid.getValue();
    //toggle->setChecked(prop);
}

bool ViewProviderProductRef::setEdit(int ModNum)
{
    //if(ModNum == 1000) {  // identifier
    //    Gui::Command::openCommand("Change subassembly solving behaviour");
    //    if(!static_cast<Assembly::Product*>(getObject())->Rigid.getValue())
    //        Gui::Command::doCommand(Gui::Command::Doc,"FreeCAD.getDocument(\"%s\").getObject(\"%s\").Rigid = True",getObject()->getDocument()->getName(), getObject()->getNameInDocument());
    //    else
    //        Gui::Command::doCommand(Gui::Command::Doc,"FreeCAD.getDocument(\"%s\").getObject(\"%s\").Rigid = False",getObject()->getDocument()->getName(), getObject()->getNameInDocument());

    //    Gui::Command::commitCommand();
    //    return false;
    //}
    //return ViewProviderItem::setEdit(ModNum); // call the base class
	return false;
}

bool ViewProviderProductRef::allowDrop(const std::vector<const App::DocumentObject*> &objList,Qt::KeyboardModifiers keys,Qt::MouseButtons mouseBts,const QPoint &pos)
{
    for( std::vector<const App::DocumentObject*>::const_iterator it = objList.begin();it!=objList.end();++it){
        //if ((*it)->getTypeId().isDerivedFrom(Part::BodyBase::getClassTypeId())) {
        //    continue; 
        //} else if ((*it)->getTypeId().isDerivedFrom(Assembly::ItemPart::getClassTypeId())) {
        //    continue; 
        //} else 
            return false;
    }
    return true;
}
void ViewProviderProductRef::drop(const std::vector<const App::DocumentObject*> &objList,Qt::KeyboardModifiers keys,Qt::MouseButtons mouseBts,const QPoint &pos)
{
        // Open command
        //Assembly::Product* AsmItem = static_cast<Assembly::Product*>(getObject());
        //App::Document* doc = AsmItem->getDocument();
        //Gui::Document* gui = Gui::Application::Instance->getDocument(doc);

        //gui->openCommand("Move into Assembly");
        //for( std::vector<const App::DocumentObject*>::const_iterator it = objList.begin();it!=objList.end();++it) {
        //    if ((*it)->getTypeId().isDerivedFrom(Part::BodyBase::getClassTypeId())) {
        //        // get document object
        //        const App::DocumentObject* obj = *it;

        //        // build Python command for execution
        //        std::string PartName = doc->getUniqueObjectName("Part");
        //        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().addObject('Assembly::ItemPart','%s')",PartName.c_str());
        //        std::string fatherName = AsmItem->getNameInDocument();
        //        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.Items = App.activeDocument().%s.Items + [App.activeDocument().%s] ",fatherName.c_str(),fatherName.c_str(),PartName.c_str());
        //        Gui::Command::addModule(Gui::Command::App,"PartDesign");
        //        Gui::Command::addModule(Gui::Command::Gui,"PartDesignGui");


        //        std::string BodyName = obj->getNameInDocument();
        //        // add the standard planes 
        //        std::string Plane1Name = BodyName + "_PlaneXY";
        //        std::string Plane2Name = BodyName + "_PlaneYZ";
        //        std::string Plane3Name = BodyName + "_PlaneXZ";
        //        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().addObject('App::Plane','%s')",Plane1Name.c_str());
        //        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().ActiveObject.Label = 'XY-Plane'");
        //        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().addObject('App::Plane','%s')",Plane2Name.c_str());
        //        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().ActiveObject.Placement = App.Placement(App.Vector(),App.Rotation(App.Vector(0,1,0),90))");
        //        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().ActiveObject.Label = 'YZ-Plane'");
        //        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().addObject('App::Plane','%s')",Plane3Name.c_str());
        //        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().ActiveObject.Placement = App.Placement(App.Vector(),App.Rotation(App.Vector(1,0,0),90))");
        //        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().ActiveObject.Label = 'XZ-Plane'");
        //        // add to annotation set of the Part object
        //        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.Annotation = [App.activeDocument().%s,App.activeDocument().%s,App.activeDocument().%s] ",PartName.c_str(),Plane1Name.c_str(),Plane2Name.c_str(),Plane3Name.c_str());
        //        // add the main body
        //        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.Model = App.activeDocument().%s ",PartName.c_str(),BodyName.c_str());

        //    } else if ((*it)->getTypeId().isDerivedFrom(Assembly::ItemPart::getClassTypeId())) {
        //        continue; 
        //    } else 
        //        continue;
        //    
        //}
        //gui->commitCommand();

}