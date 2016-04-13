/***************************************************************************
 *   Copyright (c) 2014 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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


#ifndef ASSEMBLYGUI_ViewProviderProductRef_H
#define ASSEMBLYGUI_ViewProviderProductRef_H

#include "ViewProvider.h"
#include <QMenu>
#include <QObject>


namespace AssemblyGui {

class AssemblyGuiExport ViewProviderProductRef : public AssemblyGui::ViewProviderItem
{
    PROPERTY_HEADER(PartGui::ViewProviderProductRef);

public:
    /// constructor
    ViewProviderProductRef();
    /// destructor
    virtual ~ViewProviderProductRef();

    virtual bool doubleClicked(void);
    virtual void attach(App::DocumentObject *);
    virtual void setDisplayMode(const char* ModeName);
    /// returns a list of all possible modes
    virtual std::vector<std::string> getDisplayModes(void) const;

    virtual std::vector<App::DocumentObject*> claimChildren(void)const;

    virtual std::vector<App::DocumentObject*> claimChildren3D(void)const;
    virtual bool showInTree() const
    {
      return false;
    }
    
    virtual void setupContextMenu(QMenu* menu, QObject* receiver, const char* member);
    virtual bool setEdit(int ModNum);

    /// get called if the user hover over a object in the tree 
    virtual bool allowDrop(const std::vector<const App::DocumentObject*> &objList,Qt::KeyboardModifiers keys,Qt::MouseButtons mouseBts,const QPoint &pos);
    /// get called if the user drops some objects
    virtual void drop(const std::vector<const App::DocumentObject*> &objList,Qt::KeyboardModifiers keys,Qt::MouseButtons mouseBts,const QPoint &pos);

};



} // namespace AssemblyGui


#endif // ASSEMBLYGUI_ViewProviderProductRef_H
