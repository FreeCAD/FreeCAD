/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <Mod/Part/App/PartFeature.h>

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Exception.h>
#include <Base/Sequencer.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/SoFCSelection.h>
#include <App/Application.h>

#include "ViewProviderImport.h"


using namespace PartGui;


//**************************************************************************
// Construction/Destruction

PROPERTY_SOURCE(PartGui::ViewProviderImport,PartGui::ViewProviderPart)

ViewProviderImport::ViewProviderImport()
{
    sPixmap = "PartFeatureImport";
}

ViewProviderImport::~ViewProviderImport()
{

}

// **********************************************************************************

bool ViewProviderImport::setEdit(int ModNum)
{
    return ViewProviderPart::setEdit(ModNum);
}

void ViewProviderImport::unsetEdit(int ModNum)
{
    ViewProviderPart::unsetEdit(ModNum);
}
