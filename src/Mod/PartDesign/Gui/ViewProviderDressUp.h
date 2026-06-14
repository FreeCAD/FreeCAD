// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer                                    *
 *                                   <jrheinlaender@users.sourceforge.net> *
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


#pragma once

#include "ViewProvider.h"


namespace PartDesignGui
{

class TaskDlgDressUpParameters;

class PartDesignGuiExport ViewProviderDressUp: public ViewProvider
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesignGui::ViewProviderDressUp);

public:
    /// constructor
    ViewProviderDressUp() = default;
    /// destructor
    ~ViewProviderDressUp() override = default;

    void attach(App::DocumentObject* pcObject) override;

    /// grouping handling
    void setupContextMenu(QMenu*, QObject*, const char*) override;

    /// Highlight the references that have been selected
    void highlightReferences(const bool on);

    /// Set preview parameters to indicate error state
    void setErrorState(bool error);

    /**
     * Returns the feature Name associated with the view provider.
     * Should be reimplemented in the successor.
     */
    virtual const std::string& featureName() const;
    std::string featureIcon() const;
    QString menuName;

protected:
    bool setEdit(int ModNum) override;
};


}  // namespace PartDesignGui
