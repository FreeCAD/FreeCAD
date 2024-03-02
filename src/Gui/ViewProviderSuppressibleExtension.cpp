/***************************************************************************
 *   Copyright (c) 2024 Florian Foinant-Willig <ffw@2f2v.fr>               *
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

#include <App/SuppressibleExtension.h>

#include "ActionFunction.h"
#include "Control.h"
#include "Document.h"

#include "Application.h"
#include "TreeItemMode.h"

#include "ViewProviderSuppressibleExtension.h"
#include "BitmapFactory.h"
#include "ViewProviderDocumentObject.h"
#include "qmenu.h"


using namespace Gui;

EXTENSION_PROPERTY_SOURCE(Gui::ViewProviderSuppressibleExtension, Gui::ViewProviderExtension)

ViewProviderSuppressibleExtension::ViewProviderSuppressibleExtension()
{
    initExtensionType(ViewProviderSuppressibleExtension::getExtensionClassTypeId());
}

ViewProviderSuppressibleExtension::~ViewProviderSuppressibleExtension() = default;

void ViewProviderSuppressibleExtension::extensionUpdateData(const App::Property* prop)
{
    auto vp = getExtendedViewProvider();
    auto obj = vp->getObject()->getExtensionByType<App::SuppressibleExtension>();
    if (obj && prop == &obj->Suppressed) {
        //update the tree item
        bool suppressed = obj->Suppressed.getValue();
        this->setSuppressedIcon(suppressed);
        auto activeDoc = Gui::Application::Instance->activeDocument();
        activeDoc->signalHighlightObject(*vp, Gui::HighlightMode::StrikeOut, suppressed, 0, 0);
    }
}

void ViewProviderSuppressibleExtension::setSuppressedIcon(bool onoff) {
    isSetSuppressedIcon = onoff;

    getExtendedViewProvider()->signalChangeIcon(); // signal icon change
}

QIcon ViewProviderSuppressibleExtension::extensionMergeColorfullOverlayIcons (const QIcon & orig) const
{
    QIcon mergedicon = orig;

    if(isSetSuppressedIcon) {
        QPixmap px;
        static const char * feature_suppressed_xpm[] = {
                                                       "16 16 2 1",
                                                       "     c None",
                                                       ".    c #FF0000",
                                                       ".               ",
                                                       " ..             ",
                                                       " ...            ",
                                                       "  ...           ",
                                                       "   ...          ",
                                                       "    ...         ",
                                                       "     ...        ",
                                                       "      ...       ",
                                                       "       ...      ",
                                                       "        ...     ",
                                                       "         ...    ",
                                                       "          ...   ",
                                                       "           ...  ",
                                                       "            ... ",
                                                       "             .. ",
                                                       "               ."};

        px = QPixmap(feature_suppressed_xpm);

        mergedicon = Gui::BitmapFactoryInst::mergePixmap(mergedicon, px, Gui::BitmapFactoryInst::TopLeft);
    }
    return Gui::ViewProviderExtension::extensionMergeColorfullOverlayIcons(mergedicon);
}


void ViewProviderSuppressibleExtension::extensionSetupContextMenu(QMenu* menu, QObject*, const char*)
{
    auto vp = getExtendedViewProvider();
    auto obj = vp->getObject()->getExtensionByType<App::SuppressibleExtension>();
    //show (Un)Suppress action if the Suppressed property is visible
    if (obj && ! obj->Suppressed.testStatus(App::Property::Hidden)) {
        Gui::ActionFunction* func = new Gui::ActionFunction(menu);
        QAction* act;
        if (obj->Suppressed.getValue())
            act = menu->addAction(QObject::tr("UnSuppress"));
        else
            act = menu->addAction(QObject::tr("Suppress"));

        func->trigger(act, [obj](){
            obj->Suppressed.setValue(! obj->Suppressed.getValue());
        });
    }
}


namespace Gui {
EXTENSION_PROPERTY_SOURCE_TEMPLATE(Gui::ViewProviderSuppressibleExtensionPython, Gui::ViewProviderSuppressibleExtension)

// explicit template instantiation
template class GuiExport ViewProviderExtensionPythonT<ViewProviderSuppressibleExtension>;

} //namespace Gui
