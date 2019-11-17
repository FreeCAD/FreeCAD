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


namespace Gui {

void CreateFeatCommands(void)
{
    CommandManager &rcCmdMgr = Application::Instance->commandManager();

    rcCmdMgr.addCommand(new StdCmdFeatRecompute());
    rcCmdMgr.addCommand(new StdCmdRandomColor());
}

} // namespace Gui
