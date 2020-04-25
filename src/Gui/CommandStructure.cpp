/***************************************************************************
 *   Copyright (c) 2017 Stefan Tröger <stefantroeger@gmx.net>              *
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
#include <QApplication>
#include <QMessageBox>
#endif

#include "App/Part.h"
#include "App/Document.h"
#include "Command.h"
#include "Application.h"
#include "Document.h"
#include "MDIView.h"
#include "ViewProviderDocumentObject.h"


using namespace Gui;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//===========================================================================
// Std_Part
//===========================================================================
DEF_STD_CMD_A(StdCmdPart)

StdCmdPart::StdCmdPart()
  : Command("Std_Part")
{
    sGroup        = QT_TR_NOOP("Structure");
    sMenuText     = QT_TR_NOOP("Create part");
    sToolTipText  = QT_TR_NOOP("Create a new part and make it active");
    sWhatsThis    = "Std_Part";
    sStatusTip    = sToolTipText;
    sPixmap       = "Geofeaturegroup";
}

void StdCmdPart::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    openCommand("Add a part");
    std::string FeatName = getUniqueObjectName("Part");

    std::string PartName;
    PartName = getUniqueObjectName("Part");
    doCommand(Doc,"App.activeDocument().Tip = App.activeDocument().addObject('App::Part','%s')",PartName.c_str());
    // TODO We really must set label ourselves? (2015-08-17, Fat-Zer)
    doCommand(Doc,"App.activeDocument().%s.Label = '%s'", PartName.c_str(),
            QObject::tr(PartName.c_str()).toUtf8().data());
    doCommand(Gui::Command::Gui, "Gui.activateView('Gui::View3DInventor', True)\n"
                                 "Gui.activeView().setActiveObject('%s', App.activeDocument().%s)",
            PARTKEY, PartName.c_str());

    updateActive();
}

bool StdCmdPart::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// Std_Group
//===========================================================================
DEF_STD_CMD_A(StdCmdGroup)

StdCmdGroup::StdCmdGroup()
  : Command("Std_Group")
{
    sGroup        = QT_TR_NOOP("Structure");
    sMenuText     = QT_TR_NOOP("Create group");
    sToolTipText  = QT_TR_NOOP("Create a new group for ordering objects");
    sWhatsThis    = "Std_Group";
    sStatusTip    = sToolTipText;
    sPixmap       = "folder";
}

void StdCmdGroup::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    openCommand("Add a group");

    std::string GroupName;
    GroupName = getUniqueObjectName("Group");
    QString label = QApplication::translate("Std_Group", "Group");
    doCommand(Doc,"App.activeDocument().Tip = App.activeDocument().addObject('App::DocumentObjectGroup','%s')",GroupName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Label = '%s'", GroupName.c_str(),
              label.toUtf8().data());
    commitCommand();

    Gui::Document* gui = Application::Instance->activeDocument();
    App::Document* app = gui->getDocument();
    ViewProvider* vp = gui->getViewProvider(app->getActiveObject());
    if (vp && vp->getTypeId().isDerivedFrom(ViewProviderDocumentObject::getClassTypeId()))
        gui->signalScrollToObject(*static_cast<ViewProviderDocumentObject*>(vp));
}

bool StdCmdGroup::isActive(void)
{
    return hasActiveDocument();
}

/* Datum feature commands
 * Brought here as a shortcut for quick access. The actual command still lives in PartDesign
 */

#define DATUM_CMD_DEF(_name,_desc,_icon) \
class StdCmdDatum##_name : public Command \
{\
public:\
    virtual const char* className() const { return "StdCmdDatum" #_name; }\
    StdCmdDatum##_name():Command("Std_Datum" #_name) {\
        sGroup          = QT_TR_NOOP("Structure");\
        sMenuText       = QT_TR_NOOP("Create a " _desc);\
        sToolTipText    = QT_TR_NOOP("Create a new " _desc);\
        sWhatsThis      = "PartDesign_" # _name;\
        sStatusTip      = sToolTipText;\
        sPixmap         = "Std_" #_icon;\
    }\
protected: \
    virtual void activated(int) {\
        runCommand(Doc, \
                  "import PartDesignGui\n" \
                  "import PartDesign\n" \
                  "FreeCADGui.runCommand('PartDesign_" #_name "')");\
    }\
    virtual bool isActive() { return hasActiveDocument(); }\
};\

DATUM_CMD_DEF(Point, "datum point", DatumPoint)
DATUM_CMD_DEF(Line, "datum line", DatumLine)
DATUM_CMD_DEF(Plane, "datum plane", DatumPlane)
DATUM_CMD_DEF(CoordinateSystem, "local coordinate system", DatumCS)
DATUM_CMD_DEF(SubShapeBinder, "sub-shape binder", SubShapeBinder)

//===========================================================================
// Std_DatumActions
//===========================================================================

class StdCmdDatumActions : public GroupCommand
{
public:
    StdCmdDatumActions()
        : GroupCommand("Std_DatumActions")
    {
        sGroup        = QT_TR_NOOP("Structure");
        sMenuText     = QT_TR_NOOP("Datum actions");
        sToolTipText  = QT_TR_NOOP("Actions for making various datum features");
        sWhatsThis    = "Std_DatumActions";
        sStatusTip    = sToolTipText;
        eType         = AlterDoc;
        bCanLog       = false;

        addCommand(new StdCmdDatumCoordinateSystem);
        addCommand(new StdCmdDatumPoint);
        addCommand(new StdCmdDatumLine);
        addCommand(new StdCmdDatumPlane);
        addCommand(new StdCmdDatumSubShapeBinder);
    }

    virtual const char* className() const {return "StdCmdDatumActions";}
};

namespace Gui {

void CreateStructureCommands(void)
{
    CommandManager &rcCmdMgr = Application::Instance->commandManager();

    rcCmdMgr.addCommand(new StdCmdPart());
    rcCmdMgr.addCommand(new StdCmdGroup());
    rcCmdMgr.addCommand(new StdCmdDatumActions());
}

} // namespace Gui
