/***************************************************************************
 *   Copyright (c) 2025 Pieter Hijma <info@pieterhijma.net>                *
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

#include <qapplication.h>
#include <qstyle.h>

#include <App/VariantExtension.h>

#include <Gui/BitmapFactory.h>
#include <Gui/MainWindow.h>
#include <Gui/Application.h>
#include <Gui/Document.h>

#include "ViewProviderVariant.h"


using namespace PartGui;
using namespace Gui;
using namespace std;

FC_LOG_LEVEL_INIT("App", true, true, true)

PROPERTY_SOURCE_WITH_EXTENSIONS(PartGui::ViewProviderVariant, PartGui::ViewProviderPart)

ViewProviderVariant::ViewProviderVariant()
{
    Gui::ViewProviderGroupExtension::initExtension(this);
}

ViewProviderVariant::~ViewProviderVariant() = default;

std::vector<std::string> ViewProviderVariant::getDisplayModes() const
{
  // get the modes of the father
  std::vector<std::string> StrList;

  // add your own modes
  StrList.emplace_back("Flat Lines");
  StrList.emplace_back("Shaded");
  StrList.emplace_back("Wireframe");
  StrList.emplace_back("Points");

  return StrList;
}

static ViewProvider* getViewProvider(App::DocumentObject* obj)
{
    Document *doc = Application::Instance->getDocument(obj->getDocument());
    if (doc) {
        return doc->getViewProvider(obj);
    }
    return nullptr;
 }

std::vector<App::DocumentObject*> ViewProviderVariant::extensionClaimChildren() const
{
    std::vector<App::DocumentObject*> children = Gui::ViewProviderGroupExtension::extensionClaimChildren();
    for (auto obj : children) {
        ViewProvider* vp = getViewProvider(obj);
        auto vpObj = dynamic_cast<ViewProviderDocumentObject*>(vp);
        if (vpObj) {
            vpObj->ShowInTree.setValue(false);
        }
    }
    return children;
}


QIcon ViewProviderVariant::getNonOverlayIcon() const
{
    auto ext = pcObject->getExtensionByType<App::VariantExtension>(true);
    if (ext) {
        auto support = ext->Support.getValue();
        if (support) {
            ViewProvider* vp = getViewProvider(support);
            if (vp && vp->isDerivedFrom(Gui::ViewProviderDocumentObject::getClassTypeId())) {
                return vp->getIcon();
            }
        }
    }
    return ViewProviderPart::getIcon();
}


QIcon ViewProviderVariant::getIcon() const
{
    int sizeOverlay = 12 * getMainWindow()->devicePixelRatioF();
    static int iconSize = -1;
    if(iconSize < 0) {
        iconSize = QApplication::style()->standardPixmap(QStyle::SP_DirClosedIcon).width();
    }

    QPixmap overlay = BitmapFactory().pixmapFromSvg("VariantOverlay", QSizeF(sizeOverlay, sizeOverlay));
    QIcon nonOverlayIcon = getNonOverlayIcon();
    QIcon icon = QIcon();
    icon.addPixmap(BitmapFactory().merge(nonOverlayIcon.pixmap(iconSize, iconSize, QIcon::Normal, QIcon::Off),
                                         overlay, BitmapFactoryInst::BottomRight), QIcon::Normal, QIcon::Off);
    icon.addPixmap(BitmapFactory().merge(nonOverlayIcon.pixmap(iconSize, iconSize, QIcon::Normal, QIcon::On),
                                         overlay, BitmapFactoryInst::BottomRight), QIcon::Normal, QIcon::On);
    return icon;
}

