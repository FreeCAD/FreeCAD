/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <App/DocumentObjectGroup.h>
#include <App/GroupExtension.h>
#include <App/Part.h>
#include "Application.h"
#include "Action.h"
#include "cet_lut.hpp"
#include "CommandT.h"
#include "DockWindowManager.h"
#include "Document.h"
#include "PythonConsole.h"
#include "Selection.h"
#include "SelectionObject.h"
#include "ViewProvider.h"
#include "ViewProviderDocumentObject.h"
#include "ViewProviderLink.h"

using namespace Gui;



//===========================================================================
// Std_Recompute
//===========================================================================

DEF_STD_CMD(StdCmdFeatRecompute)

StdCmdFeatRecompute::StdCmdFeatRecompute()
  :Command("Std_Recompute")
{
    // setting the
    sGroup        = "File";
    sMenuText     = QT_TR_NOOP("&Recompute");
    sToolTipText  = QT_TR_NOOP("Recomputes a feature or document");
    sWhatsThis    = "Std_Recompute";
    sStatusTip    = sToolTipText;
    sPixmap       = "view-refresh";
    sAccel        = "Ctrl+R";
}

void StdCmdFeatRecompute::activated(int iMsg)
{
    Q_UNUSED(iMsg);
}

//===========================================================================
// Std_RandomColor
//===========================================================================

DEF_STD_CMD_A(StdCmdRandomColor)

StdCmdRandomColor::StdCmdRandomColor()
  :Command("Std_RandomColor")
{
    sGroup        = "File";
    sMenuText     = QT_TR_NOOP("Random &Color");
    sToolTipText  = QT_TR_NOOP("Assigns random diffuse colors for the selected objects");
    sWhatsThis    = "Std_RandomColor";
    sStatusTip    = sToolTipText;
    sPixmap       = "Std_RandomColor";
}

void StdCmdRandomColor::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    auto setRandomColor = [](ViewProvider* view) {
        // NOLINTBEGIN
        int colIndex = rand() % (CET::R1.size() / 3) * 3;
        float fRed = CET::R1[colIndex] / 255.0F;
        float fGrn = CET::R1[colIndex + 1] / 255.0F;
        float fBlu = CET::R1[colIndex + 2] / 255.0F;
        // NOLINTEND
        auto objColor = Base::Color(fRed, fGrn, fBlu);

        auto vpLink = freecad_cast<ViewProviderLink*>(view);
        if (vpLink) {
            if (!vpLink->OverrideMaterial.getValue()) {
                vpLink->OverrideMaterial.setValue(true);
            }
            vpLink->ShapeMaterial.setDiffuseColor(objColor);
        }
        else if (view) {
            // clang-format off
            // get the view provider of the selected object and set the shape color
            if (auto prop = dynamic_cast<App::PropertyMaterialList*>(view->getPropertyByName("ShapeAppearance"))) {
                prop->setDiffuseColor(objColor);
            }
            else if (auto prop = dynamic_cast<App::PropertyMaterial*>(view->getPropertyByName("ShapeAppearance"))) {
                prop->setDiffuseColor(objColor);
            }
            // clang-format on
        }
    };

    auto allowToChangeColor = [](const App::DocumentObject* obj) {
        return (obj->isDerivedFrom<App::Part>() || obj->isDerivedFrom<App::DocumentObjectGroup>());
    };

    // get the complete selection
    std::vector<SelectionSingleton::SelObj> sel = Selection().getCompleteSelection();

    Command::openCommand(QT_TRANSLATE_NOOP("Command", "Set Random Color"));
    for (const auto & it : sel) {
        ViewProvider* view = Application::Instance->getViewProvider(it.pObject);
        setRandomColor(view);

        if (auto grp = it.pObject->getExtension<App::GroupExtension>()) {
            if (allowToChangeColor(it.pObject)) {
                std::vector<App::DocumentObject*> objs = grp->getObjects();
                for (auto obj : objs) {
                    ViewProvider* view = Application::Instance->getViewProvider(obj);
                    setRandomColor(view);
                }
            }
        }
    }

    Command::commitCommand();
}

bool StdCmdRandomColor::isActive()
{
    return (Gui::Selection().size() != 0);
}

//===========================================================================
// Std_ToggleFreeze
//===========================================================================
DEF_STD_CMD_A(StdCmdToggleFreeze)

StdCmdToggleFreeze::StdCmdToggleFreeze()
    : Command("Std_ToggleFreeze")
{
    sGroup = "File";
    sMenuText = QT_TR_NOOP("Toggle Freeze");
    static std::string toolTip = std::string("<p>")
        + QT_TR_NOOP("Toggles freeze state of the selected objects. A frozen object is not recomputed when its parents change.")
        + "</p>";
    sToolTipText = toolTip.c_str();
    sStatusTip = sToolTipText;
    sWhatsThis = "Std_ToggleFreeze";
    sPixmap = "Std_ToggleFreeze";
    sAccel = "";
    eType = AlterDoc;
}

void StdCmdToggleFreeze::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    std::vector<Gui::SelectionSingleton::SelObj> sels = Gui::Selection().getCompleteSelection();

    Command::openCommand(QT_TRANSLATE_NOOP("Command", "Toggle freeze"));
    for (Gui::SelectionSingleton::SelObj& sel : sels) {
        App::DocumentObject* obj = sel.pObject;
        if (!obj)
            continue;

        if (obj->isFreezed()){
            obj->unfreeze();
            for (auto child : obj->getInListRecursive())
                child->unfreeze();
            for (auto child : obj->getOutListRecursive())
                child->unfreeze();
        } else {
            obj->freeze();
            for (auto parent : obj->getOutListRecursive())
                parent->freeze();
        }

    }
    Command::commitCommand();
}

bool StdCmdToggleFreeze::isActive()
{
    return (Gui::Selection().size() != 0);
}




//===========================================================================
// Std_SendToPythonConsole
//===========================================================================

DEF_STD_CMD_A(StdCmdSendToPythonConsole)

StdCmdSendToPythonConsole::StdCmdSendToPythonConsole()
  :Command("Std_SendToPythonConsole")
{
    // setting the
    sGroup        = "Edit";
    sMenuText     = QT_TR_NOOP("&Send to Python Console");
    sToolTipText  = QT_TR_NOOP("Sends the selected object to the Python console");
    sWhatsThis    = "Std_SendToPythonConsole";
    sStatusTip    = sToolTipText;
    sPixmap       = "applications-python";
    sAccel        = "Ctrl+Shift+P";
}

bool StdCmdSendToPythonConsole::isActive()
{
    //active only if either 1 object is selected or multiple subobjects from the same object
    return Gui::Selection().getSelectionEx().size() == 1;
}

void StdCmdSendToPythonConsole::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    const std::vector<Gui::SelectionObject> &sels = Gui::Selection().getSelectionEx("*", App::DocumentObject::getClassTypeId(),
                                                                                    ResolveMode::OldStyleElement, false);
    if (sels.empty())
        return;
    const App::DocumentObject *obj = sels[0].getObject();
    if (!obj)
        return;
    QString docname = QString::fromLatin1(obj->getDocument()->getName());
    QString objname = QString::fromLatin1(obj->getNameInDocument());
    try {
        // clear variables from previous run, if any
        QString cmd = QLatin1String("try:\n    del(doc,lnk,obj,shp,sub,subs)\nexcept Exception:\n    pass\n");
        Gui::Command::runCommand(Gui::Command::Gui,cmd.toLatin1());
        cmd = QStringLiteral("doc = App.getDocument(\"%1\")").arg(docname);
        Gui::Command::runCommand(Gui::Command::Gui,cmd.toLatin1());
        //support links
        if (obj->isDerivedFrom<App::Link>()) {
            cmd = QStringLiteral("lnk = doc.getObject(\"%1\")").arg(objname);
            Gui::Command::runCommand(Gui::Command::Gui,cmd.toLatin1());
            cmd = QStringLiteral("obj = lnk.getLinkedObject()");
            Gui::Command::runCommand(Gui::Command::Gui,cmd.toLatin1());
            const auto link = static_cast<const App::Link*>(obj);
            obj = link->getLinkedObject();
        } else {
            cmd = QStringLiteral("obj = doc.getObject(\"%1\")").arg(objname);
            Gui::Command::runCommand(Gui::Command::Gui,cmd.toLatin1());
        }
        if (obj->isDerivedFrom<App::GeoFeature>()) {
            const auto geoObj = static_cast<const App::GeoFeature*>(obj);
            const App::PropertyGeometry* geo = geoObj->getPropertyOfGeometry();
            if (geo){
                cmd = QStringLiteral("shp = obj.") + QLatin1String(geo->getName()); //"Shape", "Mesh", "Points", etc.
                Gui::Command::runCommand(Gui::Command::Gui, cmd.toLatin1());
                if (sels[0].hasSubNames()) {
                    std::vector<std::string> subnames = sels[0].getSubNames();
                    QString subname = QString::fromLatin1(subnames[0].c_str());
                    cmd = QStringLiteral("sub = obj.getSubObject(\"%1\")").arg(subname);
                    Gui::Command::runCommand(Gui::Command::Gui,cmd.toLatin1());
                    if (subnames.size() > 1) {
                        std::ostringstream strm;
                        strm << "subs = [";
                        for (const auto & subname : subnames) {
                            strm << "obj.getSubObject(\"" << subname << "\"),";
                        }
                        strm << "]";
                        Gui::Command::runCommand(Gui::Command::Gui, strm.str().c_str());
                    }
                }
            }
        }
        //show the python console if it's not already visible, and set the keyboard focus to it
        QWidget* pc = DockWindowManager::instance()->getDockWindow("Python console");
        auto pcPython = qobject_cast<PythonConsole*>(pc);
        if (pcPython) {
            DockWindowManager::instance()->activate(pcPython);
            pcPython->setFocus();
        }
    }
    catch (const Base::Exception& e) {
        e.reportException();
    }

}

//===========================================================================
// Std_ToggleSkipRecompute
//===========================================================================

DEF_STD_CMD_AC(StdCmdToggleSkipRecompute)

StdCmdToggleSkipRecompute::StdCmdToggleSkipRecompute()
    : Command("Std_ToggleSkipRecompute")
{
    sGroup = "File";
    sMenuText = QT_TR_NOOP("Skip Recomputes");
    
    static std::string toolTip = QT_TR_NOOP("Enables or disables the recomputations of the document");

    sToolTipText = toolTip.c_str();
    sStatusTip = sToolTipText;
    sWhatsThis = "Std_ToggleSkipRecompute";
    eType = AlterDoc;
}

Gui::Action* StdCmdToggleSkipRecompute::createAction()
{
    Action* pcAction = Command::createAction();
    pcAction->setCheckable(true);
    pcAction->setIcon(QIcon());
    _pcAction = pcAction;
    isActive();
    return pcAction;
}

void StdCmdToggleSkipRecompute::activated(int iMsg)
{
    const auto doc = this->getDocument();
    if (doc == nullptr) {
        return;
    }

    Command::openCommand(QT_TRANSLATE_NOOP("Command", "Skip recomputes"));
    doc->setStatus(App::Document::SkipRecompute, (bool) iMsg);
    if (_pcAction) {
        _pcAction->setChecked((bool) iMsg);
    }
    Command::commitCommand();    
}

bool StdCmdToggleSkipRecompute::isActive()
{
    const auto doc = this->getDocument();
    if (doc == nullptr) {
        return false;
    }

    const bool skipRecomputeStatus = doc->testStatus(App::Document::SkipRecompute);
    if (_pcAction && _pcAction->isChecked() != skipRecomputeStatus) {
        _pcAction->setChecked(skipRecomputeStatus);
    }
    return true;
}

namespace Gui {

void CreateFeatCommands()
{
    CommandManager &rcCmdMgr = Application::Instance->commandManager();

    rcCmdMgr.addCommand(new StdCmdFeatRecompute());
    rcCmdMgr.addCommand(new StdCmdToggleFreeze());
    rcCmdMgr.addCommand(new StdCmdRandomColor());
    rcCmdMgr.addCommand(new StdCmdSendToPythonConsole());
    rcCmdMgr.addCommand(new StdCmdToggleSkipRecompute());
}

} // namespace Gui
