/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include "ViewProviderGeoFeatureGroup.h"
#include "Application.h"
#include "Command.h"
#include "BitmapFactory.h"
#include "Document.h"
#include "Tree.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"


using namespace Gui;


PROPERTY_SOURCE(Gui::ViewProviderGeoFeatureGroup, Gui::ViewProviderGeometryObject)


/**
 * Creates the view provider for an object group.
 */
ViewProviderGeoFeatureGroup::ViewProviderGeoFeatureGroup()
{

    pcGroupChildren = new SoGroup();
    pcGroupChildren->ref();
}

ViewProviderGeoFeatureGroup::~ViewProviderGeoFeatureGroup()
{
    pcGroupChildren->unref();
    pcGroupChildren = 0;
}



std::vector<App::DocumentObject*> ViewProviderGeoFeatureGroup::claimChildren(void)const
{
    return std::vector<App::DocumentObject*>(static_cast<App::Part*>(getObject())->Items.getValues());
}

std::vector<App::DocumentObject*> ViewProviderGeoFeatureGroup::claimChildren3D(void)const
{
   return std::vector<App::DocumentObject*>(static_cast<App::Part*>(getObject())->Items.getValues());
}


bool ViewProviderGeoFeatureGroup::onDelete(const std::vector<std::string> &)
{
    //Gui::Command::doCommand(Gui::Command::Doc,"App.getDocument(\"%s\").getObject(\"%s\").removeObjectsFromDocument()"
    //                                 ,getObject()->getDocument()->getName(), getObject()->getNameInDocument());
    return true;
}



/**
 * Returns the pixmap for the list item.
 */
QIcon ViewProviderGeoFeatureGroup::getIcon() const
{
    QIcon groupIcon;
    groupIcon.addPixmap(QApplication::style()->standardPixmap(QStyle::SP_DirClosedIcon),
                        QIcon::Normal, QIcon::Off);
    groupIcon.addPixmap(QApplication::style()->standardPixmap(QStyle::SP_DirOpenIcon),
                        QIcon::Normal, QIcon::On);
    return groupIcon;
}

void ViewProviderGeoFeatureGroup::attach(App::DocumentObject* pcObject)
{
    addDisplayMaskMode(pcGroupChildren, "Part");
    Gui::ViewProviderGeometryObject::attach(pcObject);
}

void ViewProviderGeoFeatureGroup::setDisplayMode(const char* ModeName)
{
    if ( strcmp("Part",ModeName)==0 )
        setDisplayMaskMode("Part");

    ViewProviderGeometryObject::setDisplayMode( ModeName );
}

std::vector<std::string> ViewProviderGeoFeatureGroup::getDisplayModes(void) const
{
    // get the modes of the father
    std::vector<std::string> StrList = ViewProviderGeometryObject::getDisplayModes();

    // add your own modes
    StrList.push_back("Part");

    return StrList;
}

// Python feature -----------------------------------------------------------------------

namespace Gui {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Gui::ViewProviderGeoFeatureGroupPython, Gui::ViewProviderGeoFeatureGroup)
/// @endcond

// explicit template instantiation
template class GuiExport ViewProviderPythonFeatureT<ViewProviderGeoFeatureGroup>;
}
