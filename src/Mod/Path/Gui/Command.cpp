/***************************************************************************
 *   Copyright (c) Yorik van Havre (yorik@uncreated.net) 2014              *
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

#include <TopExp_Explorer.hxx>

#include <Base/Console.h>
#include <App/Application.h>
#include <Gui/Application.h>
#include <Gui/MainWindow.h>
#include <Gui/Command.h>
#include <Gui/Selection.h>
#include <Gui/SelectionFilter.h>
#include <Gui/Document.h>
#include <Gui/Control.h>

#include <Mod/Path/App/FeaturePath.h>
#include <Mod/Path/App/FeaturePathCompound.h>
#include <Mod/Path/App/FeaturePathShape.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Path/App/FeatureArea.h>


// Path Area  #####################################################################################################


DEF_STD_CMD_A(CmdPathArea)

CmdPathArea::CmdPathArea()
    :Command("Path_Area")
{
    sAppModule      = "Path";
    sGroup          = QT_TR_NOOP("Path");
    sMenuText       = QT_TR_NOOP("Area");
    sToolTipText    = QT_TR_NOOP("Creates a feature area from selected objects");
    sWhatsThis      = "Path_Area";
    sStatusTip      = sToolTipText;
    sPixmap         = "Path-Area";
    sAccel          = "P,A";

}

void CmdPathArea::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<Gui::SelectionObject> Sel = 
            getSelection().getSelectionEx(NULL, Part::Feature::getClassTypeId());
    std::list<std::string> cmds;
    std::ostringstream sources;
    if (Sel.size() > 0) {
        for(const Gui::SelectionObject &selObj : Sel) {
            const Part::Feature *pcObj = static_cast<const Part::Feature*>(selObj.getObject());
            if(selObj.getSubNames().empty()) {
                const TopoDS_Shape &shape = pcObj->Shape.getShape().getShape();
                TopExp_Explorer it(shape, TopAbs_SHELL); 
                if(it.More()) {
                    Base::Console().Error("Selected shape is not 2D\n");
                    return;
                }
                sources << "FreeCAD.activeDocument()." << pcObj->getNameInDocument() << ",";
                continue;
            }

            for(const std::string &name : selObj.getSubNames()) {
                if(!name.compare(0,4,"Face") &&
                   !name.compare(0,4,"Edge")) 
                {
                    Base::Console().Error("Selected shape is not 2D\n");
                    return;
                }

                int index = atoi(name.substr(4).c_str());

                std::ostringstream subname;
                subname << pcObj->getNameInDocument() << '_' << name;
                std::string sub_fname = getUniqueObjectName(subname.str().c_str());

                std::ostringstream cmd;
                cmd << "FreeCAD.activeDocument().addObject('Path::Feature','" << sub_fname << "').Shape = " <<
                    pcObj->getNameInDocument() << '.' << name.substr(0,4) << '[' << index-1 << ']';
                cmds.push_back(cmd.str());
                sources << "FreeCAD.activeDocument()." << sub_fname << ",";
            }
        }
    }
    std::string FeatName = getUniqueObjectName("FeatureArea");
    openCommand("Create Path Area");
    for(const std::string &cmd : cmds)
        doCommand(Doc,cmd.c_str());
    doCommand(Doc,"FreeCAD.activeDocument().addObject('Path::FeatureArea','%s')",FeatName.c_str());
    doCommand(Doc,"FreeCAD.activeDocument().%s.Sources = [ %s ]",FeatName.c_str(),sources.str().c_str());
    commitCommand();
    updateActive();
}

bool CmdPathArea::isActive(void)
{
    return hasActiveDocument();
}

// Path compound #####################################################################################################


DEF_STD_CMD_A(CmdPathCompound)

CmdPathCompound::CmdPathCompound()
    :Command("Path_Compound")
{
    sAppModule      = "Path";
    sGroup          = QT_TR_NOOP("Path");
    sMenuText       = QT_TR_NOOP("Compound");
    sToolTipText    = QT_TR_NOOP("Creates a compound from selected paths");
    sWhatsThis      = "Path_Compound";
    sStatusTip      = sToolTipText;
    sPixmap         = "Path-Compound";
    sAccel          = "P,C";

}

void CmdPathCompound::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<Gui::SelectionSingleton::SelObj> Sel = getSelection().getSelection();
    if (Sel.size() > 0) {
        std::ostringstream cmd;
        cmd << "[";
        Path::Feature *pcPathObject;
        for (std::vector<Gui::SelectionSingleton::SelObj>::const_iterator it=Sel.begin();it!=Sel.end();++it) {
            if ((*it).pObject->getTypeId().isDerivedFrom(Path::Feature::getClassTypeId())) {
                pcPathObject = static_cast<Path::Feature*>((*it).pObject);
                cmd << "FreeCAD.activeDocument()." << pcPathObject->getNameInDocument() << ",";
            } else {
                Base::Console().Error("Only Path objects must be selected before running this command\n");
                return;
            }
        }
        cmd << "]";
        std::string FeatName = getUniqueObjectName("PathCompound");
        openCommand("Create Path Compound");
        doCommand(Doc,"FreeCAD.activeDocument().addObject('Path::FeatureCompound','%s')",FeatName.c_str());
        doCommand(Doc,"FreeCAD.activeDocument().%s.Group = %s",FeatName.c_str(),cmd.str().c_str());
        commitCommand();
        updateActive();
    } else {
        Base::Console().Error("At least one Path object must be selected\n");
        return;
    }
}

bool CmdPathCompound::isActive(void)
{
    return hasActiveDocument();
}

 // Path Shape #####################################################################################################


DEF_STD_CMD_A(CmdPathShape)

CmdPathShape::CmdPathShape()
    :Command("Path_Shape")
{
    sAppModule      = "Path";
    sGroup          = QT_TR_NOOP("Path");
    sMenuText       = QT_TR_NOOP("From Shape");
    sToolTipText    = QT_TR_NOOP("Creates a path from a selected shape");
    sWhatsThis      = "Path_Shape";
    sStatusTip      = sToolTipText;
    sPixmap         = "Path-Shape";
    sAccel          = "P,S";
}

void CmdPathShape::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<Gui::SelectionSingleton::SelObj> Sel = getSelection().getSelection();
    if (Sel.size() == 1) {
        if (Sel[0].pObject->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
            Part::Feature *pcPartObject = static_cast<Part::Feature*>(Sel[0].pObject);
            std::string FeatName = getUniqueObjectName("PathShape");
            openCommand("Create Path Compound");
            doCommand(Doc,"FreeCAD.activeDocument().addObject('Path::FeatureShape','%s')",FeatName.c_str());
            doCommand(Doc,"FreeCAD.activeDocument().%s.Shape = FreeCAD.activeDocument().%s.Shape.copy()",FeatName.c_str(),pcPartObject->getNameInDocument());
            commitCommand();
            updateActive();
        } else {
            Base::Console().Error("Exactly one shape object must be selected\n");
            return;
        }
    } else {
        Base::Console().Error("Exactly one shape object must be selected\n");
        return;
    }
}

bool CmdPathShape::isActive(void)
{
    return hasActiveDocument();
}



void CreatePathCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();
    rcCmdMgr.addCommand(new CmdPathCompound());
    rcCmdMgr.addCommand(new CmdPathShape());
    rcCmdMgr.addCommand(new CmdPathArea());
}
