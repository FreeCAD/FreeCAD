/***************************************************************************
 *   Copyright (c) 2024 Martin Rodriguez Reboredo <yakoyoku@gmail.com>     *
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

#include <QMenu>
#include <QMessageBox>
#endif

#include <Base/Vector3D.h>
#include <Gui/CommandT.h>
#include <Gui/Control.h>
#include <Gui/MainWindow.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/Utilities.h>
#include <Mod/Part/App/Part2DObject.h>
#include <Mod/Text/App/ShapeText.h>

#include "ViewProviderShapeText.h"


FC_LOG_LEVEL_INIT("Text", true, true)

using namespace TextGui;
using namespace Text;


PROPERTY_SOURCE_WITH_EXTENSIONS(TextGui::ViewProviderShapeText, PartGui::ViewProvider2DObject)


ViewProviderShapeText::ViewProviderShapeText()
{
    PartGui::ViewProviderAttachExtension::initExtension(this);

    sPixmap = "Text_ShapeText";
}

ViewProviderShapeText::~ViewProviderShapeText()
{}

Text::ShapeText* ViewProviderShapeText::getShapeText() const
{
    return dynamic_cast<Text::ShapeText*>(pcObject);
}

void ViewProviderShapeText::attach(App::DocumentObject* pcFeat)
{
    ViewProviderPart::attach(pcFeat);
}

namespace Gui
{
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TextGui::ViewProviderPython, TextGui::ViewProviderShapeText)
/// @endcond

// explicit template instantiation
template class TextGuiExport ViewProviderFeaturePythonT<TextGui::ViewProviderShapeText>;
}// namespace Gui
