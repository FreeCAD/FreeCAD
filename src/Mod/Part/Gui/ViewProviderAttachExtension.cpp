/***************************************************************************
 *   Copyright (c) 2016 Stefan Tröger <stefantroeger@gmx.net>              *
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

#include "ViewProviderAttachExtension.h"
#include <Mod/Part/App/AttachExtension.h>

#include <Gui/BitmapFactory.h>

using namespace PartGui;

EXTENSION_PROPERTY_SOURCE(PartGui::ViewProviderAttachExtension, Gui::ViewProviderExtension)


ViewProviderAttachExtension::ViewProviderAttachExtension()
{
    initExtensionType(ViewProviderAttachExtension::getExtensionClassTypeId());
}

QIcon ViewProviderAttachExtension::extensionMergeOverlayIcons(const QIcon & orig) const
{
    QIcon mergedicon = orig;

    if (getExtendedViewProvider()->getObject()->hasExtension(Part::AttachExtension::getExtensionClassTypeId())) {

        auto* attach = getExtendedViewProvider()->getObject()->getExtensionByType<Part::AttachExtension>();

        if (attach) {

            bool attached = false;

            try{
                attached = attach->positionBySupport();
            }
            catch (...) { // We are just trying to get an icon, if no placement can be calculated, set unattached.
                // set unattached
            }

            if(!attached) {
                QPixmap px;

                static const char * const feature_detached_xpm[]={
                    "9 9 3 1",
                    ". c None",
                    "# c #cc00cc",
                    "a c #ffffff",
                    "...###...",
                    ".##aaa##.",
                    "##aaaaa##",
                    "##aaaaa##",
                    "#########",
                    "#########",
                    "#########",
                    ".##aaa##.",
                    ".##aaa##.",
                    "...###..."};

                    px = QPixmap(feature_detached_xpm);

                    mergedicon = Gui::BitmapFactoryInst::mergePixmap(mergedicon, px, Gui::BitmapFactoryInst::BottomLeft);
            }
        }
    }

    return mergedicon;
}

void ViewProviderAttachExtension::extensionUpdateData(const App::Property* prop)
{
    if (getExtendedViewProvider()->getObject()->hasExtension(Part::AttachExtension::getExtensionClassTypeId())) {
        auto* attach = getExtendedViewProvider()->getObject()->getExtensionByType<Part::AttachExtension>();

        if(attach) {
            if( prop == &(attach->Support) ||
                prop == &(attach->MapMode) ||
                prop == &(attach->MapPathParameter) ||
                prop == &(attach->MapReversed) ||
                prop == &(attach->AttachmentOffset) ||
                prop == &(attach->AttacherType) ) {

                getExtendedViewProvider()->signalChangeIcon(); // signal icon change
            }
        }
    }

}

namespace Gui {
    EXTENSION_PROPERTY_SOURCE_TEMPLATE(PartGui::ViewProviderAttachExtensionPython, PartGui::ViewProviderAttachExtension)

// explicit template instantiation
    template class PartGuiExport ViewProviderExtensionPythonT<PartGui::ViewProviderAttachExtension>;
}
