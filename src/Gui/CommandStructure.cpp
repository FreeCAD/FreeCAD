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

#include <Base/Console.h>
#include "App/Part.h"
#include "App/Document.h"
#include "CommandT.h"
#include "Application.h"
#include "Document.h"
#include "MDIView.h"
#include "MainWindow.h"
#include "ViewProviderDocumentObject.h"

FC_LOG_LEVEL_INIT("Gui", true, true);

using namespace Gui;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//===========================================================================
// Std_Part
//===========================================================================
DEF_STD_CMD_A(StdCmdPart)

StdCmdPart::StdCmdPart()
  : Command("Std_Part")
{
    sGroup        = "Structure";
    sMenuText     = QT_TR_NOOP("Create part");
    sToolTipText  = QT_TR_NOOP("Create a new part and make it active");
    sWhatsThis    = "Std_Part";
    sStatusTip    = sToolTipText;
    sPixmap       = "Geofeaturegroup";
}

static App::SubObjectT addGroup(const char *type, const char *name, const QString &label)
{
    Gui::Document* gui = Application::Instance->activeDocument();
    App::Document* app = gui->getDocument();

    try {
        App::Document *doc = nullptr;
        auto sels = Gui::Selection().getSelectionT(nullptr, 0);
        App::DocumentObject *group = nullptr;
        App::DocumentObject *geogroup = nullptr;
        std::vector<App::DocumentObject *> children;
        App::SubObjectT ctx;
        for (auto &sel : sels) {
            auto obj = sel.getSubObject();
            if (!obj || std::find(children.begin(), children.end(), obj) != children.end())
                continue;
            bool skip = false;
            auto geogrp = App::GeoFeatureGroupExtension::getGroupOfObject(obj);
            if (geogrp) {
                if (!geogroup) {
                    if (children.empty()) {
                        geogroup = geogrp;
                        auto objs = sel.getSubObjectList();
                        auto it = std::find(objs.begin(), objs.end(), geogroup);
                        if (it != objs.end())
                            ctx = App::SubObjectT(objs.begin(), it+1);
                    } else
                        skip = true;
                } else if (geogrp != geogroup)
                    skip = true;
            }
            App::DocumentObject *grp = !skip ? App::GroupExtension::getGroupOfObject(obj) : nullptr;
            if (grp) {
                if (!group) {
                    if (children.empty()) {
                        group = grp;
                        auto objs = sel.getSubObjectList();
                        auto it = std::find(objs.begin(), objs.end(), group);
                        if (it != objs.end())
                            ctx = App::SubObjectT(objs.begin(), it+1);
                    } else
                        skip = true;
                } else if (grp != group)
                    skip = true;
            }
            if (!skip && doc && doc != obj->getDocument()) {
                FC_WARN("Ignore external object " << sel.getObjectFullName());
                continue;
            }
            if (!skip) {
                children.push_back(obj);
                doc = obj->getDocument();
            } else
                FC_WARN("Ignore selected object " << sel.getObjectFullName()
                        << " from group '" << (grp ? grp : geogrp)->Label.getValue() << "'");
        }

        if (!doc)
            doc = app;
        std::string GroupName = doc->getUniqueObjectName(name);
        cmdAppDocument(app, std::ostringstream()
                << "addObject('" << type << "','" << GroupName << "')");
        auto newgrp = App::GetApplication().getActiveDocument()->getObject(GroupName.c_str());
        if (!newgrp)
            throw Base::RuntimeError("Failed to create object");
        cmdAppDocument(app, std::ostringstream() << "Tip = " << newgrp->getFullName(true));
        cmdAppObjectArgs(newgrp, "Label = u'%s'", label.toUtf8().constData());

        for (int i=0; i<2; ++i) {
            auto grp = i ? geogroup : group;
            if (!grp)
                continue;
            auto ext = grp->getExtensionByType<App::GroupExtension>();
            if (!ext->allowObject(newgrp)) {
                geogroup = nullptr;
                group = nullptr;
                children.clear();
                FC_WARN("Ignore selections because "
                        << grp->getFullName() << " does not accept " << type);
            }
        }
        bool remove = false;
        if (geogroup && newgrp->hasExtension(App::GeoFeatureGroupExtension::getExtensionClassTypeId())) {
            remove = true;
            auto vp = Application::Instance->getViewProvider(geogroup);
            if (vp) {
                if (!vp->canDragObjects()) {
                    FC_WARN("Ignore selections because "
                            << geogroup->getFullName() << " does not allow removing children");
                    children.clear();
                }
                for (auto it=children.begin(); it!=children.end();) {
                    if (!vp->canDragObject(*it)) {
                        it = children.erase(it);
                        FC_WARN("Ignore selected object " << (*it)->getFullName()
                                << " cannot be removed from " << geogroup->getFullName());
                    } else
                        ++it;
                }
            }
        }

        if (children.size()) {
            if (geogroup)
                cmdAppObjectArgs(geogroup, "addObject(%s)", newgrp->getFullName(true));
            if (group)
                cmdAppObjectArgs(group, "addObject(%s)", newgrp->getFullName(true));
            std::ostringstream ss;
            for (auto child : children) {
                if (remove)
                    cmdAppObjectArgs(geogroup, "removeObject(%s)", child->getFullName(true));
                ss << child->getFullName(true) << ", ";
            }
            cmdAppObjectArgs(newgrp, "addObjects([%s])", ss.str());
        } else
            ctx = App::SubObjectT();
        
        Gui::Selection().clearSelection();
        ctx = ctx.getChild(newgrp);
        Gui::Selection().addSelection(ctx);
        Application::Instance->commandManager().runCommandByName("Std_TreeSelection");
        Gui::Selection().clearSelection();

        Command::updateActive();

        return ctx;
    } catch (Base::Exception &e) {
        e.ReportException();
        QMessageBox::critical(getMainWindow(),
                              QObject::tr("Error"),
                              QString::fromUtf8(e.what()));
    }
    return App::SubObjectT();
}

void StdCmdPart::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    openCommand(QT_TRANSLATE_NOOP("Command", "Add a part"));
    auto res = addGroup("App::Part", "Part", QApplication::translate("Std_Part", "Part"));
    if (res.getObjectName().empty())
        return;

    doCommand(Gui::Command::Gui, "Gui.activateView('Gui::View3DInventor', True)");
    cmdGuiDocument(App::GetApplication().getActiveDocument(), std::ostringstream()
                << "ActiveView.setActiveObject('" << PARTKEY << "', "
                << res.getObjectPython() << ", '" << res.getSubName() << "')");
}

bool StdCmdPart::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// Std_PartActive
//===========================================================================
DEF_STD_CMD_A(StdCmdPartActive)

StdCmdPartActive::StdCmdPartActive()
  : Command("Std_PartActive")
{
    sGroup        = "Structure";
    sMenuText     = QT_TR_NOOP("Create part in active container");
    sToolTipText  = QT_TR_NOOP("Create a new part and add it to current active container");
    sWhatsThis    = "Std_PartActive";
    sStatusTip    = sToolTipText;
    sPixmap       = "GeofeaturegroupAddActive";
}


void StdCmdPartActive::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    App::SubObjectT activePart;
    auto *activeView = Gui::Application::Instance->activeView();
    if(activeView) {
        App::DocumentObject *parent = nullptr;
        std::string subname;
        auto active = activeView->getActiveObject<App::DocumentObject*>(PARTKEY,&parent,&subname);
        if (active) {
            for(auto &sel : Gui::Selection().getSelection()) {
                if (App::GeoFeatureGroupExtension::getGroupOfObject(sel.pObject)) {
                    active = nullptr;
                    break;
                }
            }
        }
        if (active) {
            if (parent)
                activePart = App::SubObjectT(parent, subname.c_str());
            else
                activePart = active;
        }
    }

    auto res = addGroup("App::Part", "Part", QApplication::translate("Std_Part", "Part"));
    auto newPart = res.getSubObject();
    if (!newPart)
        return;
    auto active = activePart.getSubObject();
    if (active) {
        cmdAppObjectArgs(active, "addObject(%s)", newPart->getFullName(true));
        res = activePart.getChild(newPart);
    }

    doCommand(Gui::Command::Gui, "Gui.activateView('Gui::View3DInventor', True)");
    cmdGuiDocument(App::GetApplication().getActiveDocument(), std::ostringstream()
                << "ActiveView.setActiveObject('" << PARTKEY << "', "
                << res.getObjectPython() << ", '" << res.getSubName() << "')");
}

bool StdCmdPartActive::isActive(void)
{
    return hasActiveDocument();
}

class StdCmdPartActions : public GroupCommand
{
public:
    StdCmdPartActions()
        : GroupCommand("Std_PartActions")
    {
        sGroup        = "Structure";
        sMenuText     = QT_TR_NOOP("Part actions");
        sToolTipText  = QT_TR_NOOP("Actions for making a part");
        sWhatsThis    = "Std_PartActions";
        sStatusTip    = sToolTipText;
        eType         = AlterDoc;
        bCanLog       = false;

        addCommand(new StdCmdPart);
        addCommand(new StdCmdPartActive);
    }

    virtual const char* className() const {return "StdCmdPartActions";}
};


//===========================================================================
// Std_Group
//===========================================================================
DEF_STD_CMD_A(StdCmdGroup)

StdCmdGroup::StdCmdGroup()
  : Command("Std_Group")
{
    sGroup        = "Structure";
    sMenuText     = QT_TR_NOOP("Create group");
    sToolTipText  = QT_TR_NOOP("Create a new group for ordering objects");
    sWhatsThis    = "Std_Group";
    sStatusTip    = sToolTipText;
    sPixmap       = "folder";
}

void StdCmdGroup::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    openCommand(QT_TRANSLATE_NOOP("Command", "Add a group"));
    addGroup("App::DocumentObjectGroup","Group", QApplication::translate("Std_Group", "Group"));
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
        sGroup          = "Structure";\
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
        sGroup        = "Structure";
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

    rcCmdMgr.addCommand(new StdCmdPartActions());
    rcCmdMgr.addCommand(new StdCmdGroup());
    rcCmdMgr.addCommand(new StdCmdDatumActions());
}

} // namespace Gui
