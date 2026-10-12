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


#include <App/Document.h>
#include <App/Link.h>
#include <App/SuppressibleExtension.h>

#include "Control.h"
#include "Document.h"

#include "Application.h"
#include "TreeItemMode.h"

#include "ViewProviderSuppressibleExtension.h"
#include "BitmapFactory.h"
#include "ViewProviderDocumentObject.h"


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
    auto owner = vp->getObject();
    if (!owner || !owner->isValid()) {
        return;
    }

    auto ext = owner->getExtensionByType<App::SuppressibleExtension>(true);

    if (ext && prop == &ext->Suppressed) {
        // update the tree item
        bool suppressed = ext->Suppressed.getValue();
        setSuppressedIcon(suppressed);
        getExtendedViewProvider()->signalChangeHighlight(suppressed, Gui::HighlightMode::StrikeOut);

        if (owner->isDerivedFrom<App::LinkElement>()) {
            for (auto* parent : owner->getInList()) {
                auto* linkExt = parent ? parent->getExtensionByType<App::LinkBaseExtension>(true)
                                       : nullptr;
                auto* elementList = linkExt ? linkExt->_getElementListProperty() : nullptr;
                if (!elementList) {
                    continue;
                }
                if (auto* parentVp = Application::Instance->getViewProvider(parent)) {
                    parentVp->updateData(elementList);
                }
            }
        }
    }
}

void ViewProviderSuppressibleExtension::setSuppressedIcon(bool onoff)
{
    isSetSuppressedIcon = onoff;

    getExtendedViewProvider()->signalChangeIcon();  // signal icon change
}

QIcon ViewProviderSuppressibleExtension::extensionMergeColorfullOverlayIcons(const QIcon& orig) const
{
    QIcon mergedicon = orig;

    if (isSetSuppressedIcon) {
        static QPixmap px(Gui::BitmapFactory().pixmapFromSvg("feature_suppressed", QSize(16, 16)));

        mergedicon
            = Gui::BitmapFactoryInst::mergePixmap(mergedicon, px, Gui::BitmapFactoryInst::TopLeft);
    }
    return Gui::ViewProviderExtension::extensionMergeColorfullOverlayIcons(mergedicon);
}


namespace Gui
{
EXTENSION_PROPERTY_SOURCE_TEMPLATE(
    Gui::ViewProviderSuppressibleExtensionPython,
    Gui::ViewProviderSuppressibleExtension
)

// explicit template instantiation
template class GuiExport ViewProviderExtensionPythonT<ViewProviderSuppressibleExtension>;

}  // namespace Gui
