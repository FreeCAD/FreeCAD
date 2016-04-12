/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QApplication>
# include <QPixmap>
#endif

#include <App/Part.h>
#include <App/Document.h>

#include "ActiveObjectList.h"
#include "BitmapFactory.h"
#include "Command.h"

#include "ViewProviderPart.h"


using namespace Gui;


PROPERTY_SOURCE(Gui::ViewProviderPart, Gui::ViewProviderOriginGroup)


/**
 * Creates the view provider for an object group.
 */
ViewProviderPart::ViewProviderPart()
{ }

ViewProviderPart::~ViewProviderPart()
{ }

/**
 * TODO
 * Whenever a property of the group gets changed then the same property of all
 * associated view providers of the objects of the object group get changed as well.
 */
void ViewProviderPart::onChanged(const App::Property* prop) {
    ViewProviderOriginGroup::onChanged(prop);
}

bool ViewProviderPart::doubleClicked(void)
{
    //make the part the active one
    Gui::Command::doCommand(Gui::Command::Gui,
            "Gui.activeView().setActiveObject('%s', App.activeDocument().%s)",
            PARTKEY, this->getObject()->getNameInDocument());

    return true;
}

bool ViewProviderPart::canDropObject(App::DocumentObject* obj) const {
    
    //it is not allowed to have any part or assembly object within a part, hence we exclude origin groups
    if(obj->isDerivedFrom(App::OriginGroup::getClassTypeId()))
        return false;
    
    return Gui::ViewProvider::canDropObject(obj);
}


/**
 * Returns the pixmap for the list item.
 */
QIcon ViewProviderPart::getIcon() const
{
    // TODO Make a nice icon for the part (2015-09-01, Fat-Zer)
    QIcon groupIcon;
    groupIcon.addPixmap(QApplication::style()->standardPixmap(QStyle::SP_DirClosedIcon),
                        QIcon::Normal, QIcon::Off);
    groupIcon.addPixmap(QApplication::style()->standardPixmap(QStyle::SP_DirOpenIcon),
                        QIcon::Normal, QIcon::On);
    return groupIcon;
}

// Python feature -----------------------------------------------------------------------

namespace Gui {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Gui::ViewProviderPartPython, Gui::ViewProviderPart)
/// @endcond

// explicit template instantiation
template class GuiExport ViewProviderPythonFeatureT<ViewProviderPart>;
}
