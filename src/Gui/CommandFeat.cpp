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
#include "Command.h"
#include "Document.h"
#include "Selection.h"
#include "ViewProvider.h"
#include "ViewProviderDocumentObject.h"

using namespace Gui;



//===========================================================================
// Std_Recompute
//===========================================================================

DEF_STD_CMD(StdCmdFeatRecompute);

StdCmdFeatRecompute::StdCmdFeatRecompute()
  :Command("Std_Recompute")
{
    // seting the
    sGroup        = QT_TR_NOOP("File");
    sMenuText     = QT_TR_NOOP("&Recompute");
    sToolTipText  = QT_TR_NOOP("Recompute feature or document");
    sWhatsThis    = QT_TR_NOOP("Recompute feature or document");
    sStatusTip    = QT_TR_NOOP("Recompute feature or document");
    sPixmap       = "view-refresh";
    sAccel        = "Ctrl+R";
}

void StdCmdFeatRecompute::activated(int iMsg)
{
}

//===========================================================================
// Std_RandomColor
//===========================================================================

DEF_STD_CMD_A(StdCmdRandomColor);

StdCmdRandomColor::StdCmdRandomColor()
  :Command("Std_RandomColor")
{
    sGroup        = QT_TR_NOOP("File");
    sMenuText     = QT_TR_NOOP("Random color");
    sToolTipText  = QT_TR_NOOP("Random color");
    sWhatsThis    = QT_TR_NOOP("Random color");
    sStatusTip    = QT_TR_NOOP("Random color");
}

void StdCmdRandomColor::activated(int iMsg)
{
    // get the complete selection
    std::vector<SelectionSingleton::SelObj> sel = Selection().getCompleteSelection();
    for (std::vector<SelectionSingleton::SelObj>::iterator it = sel.begin(); it != sel.end(); ++it) {
        float fMax = (float)RAND_MAX;
        float fRed = (float)rand()/fMax;
        float fGrn = (float)rand()/fMax;
        float fBlu = (float)rand()/fMax;

        ViewProvider* view = Application::Instance->getDocument(it->pDoc)->getViewProvider(it->pObject);
        App::Property* color = view->getPropertyByName("ShapeColor");
        if (color && color->getTypeId() == App::PropertyColor::getClassTypeId()) {
            // get the view provider of the selected object and set the shape color
            doCommand(Gui, "Gui.getDocument(\"%s\").getObject(\"%s\").ShapeColor=(%.2f,%.2f,%.2f)"
                         , it->DocName, it->FeatName, fRed, fGrn, fBlu);
        }
    }
}

bool StdCmdRandomColor::isActive(void)
{
    return (Gui::Selection().size() != 0);
}


namespace Gui {

void CreateFeatCommands(void)
{
    CommandManager &rcCmdMgr = Application::Instance->commandManager();

    rcCmdMgr.addCommand(new StdCmdFeatRecompute());
    rcCmdMgr.addCommand(new StdCmdRandomColor());
}

} // namespace Gui
