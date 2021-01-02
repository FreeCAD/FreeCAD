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

#include <App/DocumentObject.h>

#include "Application.h"
#include "CommandT.h"
#include "Document.h"
#include "Selection.h"
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
    sGroup        = QT_TR_NOOP("File");
    sMenuText     = QT_TR_NOOP("&Recompute");
    sToolTipText  = QT_TR_NOOP("Recompute feature or document");
    sWhatsThis    = "Std_Recompute";
    sStatusTip    = QT_TR_NOOP("Recompute feature or document");
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
    sGroup        = QT_TR_NOOP("File");
    sMenuText     = QT_TR_NOOP("Random color");
    sToolTipText  = QT_TR_NOOP("Random color");
    sWhatsThis    = "Std_RandomColor";
    sStatusTip    = QT_TR_NOOP("Random color");
    sPixmap       = "Std_RandomColor";
}

void StdCmdRandomColor::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    // get the complete selection
    std::vector<SelectionSingleton::SelObj> sel = Selection().getCompleteSelection();
    for (std::vector<SelectionSingleton::SelObj>::iterator it = sel.begin(); it != sel.end(); ++it) {
        float fMax = (float)RAND_MAX;
        float fRed = (float)rand()/fMax;
        float fGrn = (float)rand()/fMax;
        float fBlu = (float)rand()/fMax;

        ViewProvider* view = Application::Instance->getDocument(it->pDoc)->getViewProvider(it->pObject);
        auto vpLink = dynamic_cast<ViewProviderLink*>(view);
        if(vpLink) {
            if(!vpLink->OverrideMaterial.getValue())
                cmdGuiObjectArgs(it->pObject, "OverrideMaterial = True");
            cmdGuiObjectArgs(it->pObject, "ShapeMaterial.DiffuseColor=(%.2f,%.2f,%.2f)", fRed, fGrn, fBlu);
            continue;
        }
        auto color = dynamic_cast<App::PropertyColor*>(view->getPropertyByName("ShapeColor"));
        if (color) {
            // get the view provider of the selected object and set the shape color
            cmdGuiObjectArgs(it->pObject, "ShapeColor=(%.2f,%.2f,%.2f)", fRed, fGrn, fBlu);
        }
    }
}

bool StdCmdRandomColor::isActive(void)
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
    sGroup        = QT_TR_NOOP("Edit");
    sMenuText     = QT_TR_NOOP("&Send to Python Console");
    sToolTipText  = QT_TR_NOOP("Sends the selected object to the Python console");
    sWhatsThis    = "Std_SendToPythonConsole";
    sStatusTip    = QT_TR_NOOP("Sends the selected object to the Python console");
    sPixmap       = "applications-python";
    sAccel        = "Ctrl+Shift+P";
}

bool StdCmdSendToPythonConsole::isActive(void)
{
    return (Gui::Selection().size() == 1);
}

void StdCmdSendToPythonConsole::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    const std::vector<Gui::SelectionObject> &sels = Gui::Selection().getSelectionEx("*",App::DocumentObject::getClassTypeId(),true,true);
    if (sels.empty())
        return;
    const App::DocumentObject *obj = sels[0].getObject();
    QString docname = QString::fromLatin1(obj->getDocument()->getName());
    QString objname = QString::fromLatin1(obj->getNameInDocument());
    try {
        QString cmd = QString::fromLatin1("obj = App.getDocument(\"%1\").getObject(\"%2\")").arg(docname,objname);
        Gui::Command::runCommand(Gui::Command::Gui,cmd.toLatin1());
        if (sels[0].hasSubNames()) {
            std::vector<std::string> subnames = sels[0].getSubNames();
            if (obj->getPropertyByName("Shape")) {
                QString subname = QString::fromLatin1(subnames[0].c_str());
                cmd = QString::fromLatin1("shp = App.getDocument(\"%1\").getObject(\"%2\").Shape")
                    .arg(docname, objname);
                Gui::Command::runCommand(Gui::Command::Gui,cmd.toLatin1());
                cmd = QString::fromLatin1("elt = App.getDocument(\"%1\").getObject(\"%2\").Shape.%4")
                    .arg(docname,objname,subname);
                Gui::Command::runCommand(Gui::Command::Gui,cmd.toLatin1());
            }
        }
    }
    catch (const Base::Exception& e) {
        e.ReportException();
    }

}


namespace Gui {

void CreateFeatCommands(void)
{
    CommandManager &rcCmdMgr = Application::Instance->commandManager();

    rcCmdMgr.addCommand(new StdCmdFeatRecompute());
    rcCmdMgr.addCommand(new StdCmdRandomColor());
    rcCmdMgr.addCommand(new StdCmdSendToPythonConsole());
}

} // namespace Gui
