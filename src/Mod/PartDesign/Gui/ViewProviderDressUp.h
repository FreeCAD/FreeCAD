/***************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net>        *
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


#ifndef PARTGUI_ViewProviderDressUp_H
#define PARTGUI_ViewProviderDressUp_H

#include "ViewProvider.h"


namespace PartDesignGui {

class TaskDlgDressUpParameters;

class PartDesignGuiExport ViewProviderDressUp : public ViewProvider
{
    PROPERTY_HEADER(PartDesignGui::ViewProviderDressUp);

public:
    /// constructor
    ViewProviderDressUp()
        : featureName("undefined") {}
    /// destructor
    virtual ~ViewProviderDressUp()
        {}

    /// grouping handling
    void setupContextMenu(QMenu*, QObject*, const char*);

    virtual bool onDelete(const std::vector<std::string> &);

    /// Highlight the references that have been selected
    void highlightReferences(const bool on);

    // The feature name of the subclass
    std::string featureName;

protected:
    const bool checkDlgOpen(TaskDlgDressUpParameters* dressUpDlg);

private:
    std::vector<App::Color> originalFaceColors;
    std::vector<App::Color> originalLineColors;

};



} // namespace PartDesignGui


#endif // PARTGUI_ViewProviderDressUp_H
