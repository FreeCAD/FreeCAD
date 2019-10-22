/****************************************************************************
 *   Copyright (c) 2019 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#ifndef PARTGUI_ViewProviderSolid_H
#define PARTGUI_ViewProviderSolid_H

#include "ViewProvider.h"

namespace PartDesignGui {

class PartDesignGuiExport ViewProviderSolid : public PartGui::ViewProviderPart
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesignGui::ViewProviderSolid);
    typedef PartGui::ViewProviderPart inherited;

public:
    ViewProviderSolid();

    virtual void updateData(const App::Property* prop) override;
    virtual QIcon getIcon(void) const override;
    virtual void setupContextMenu(QMenu* menu, QObject* receiver, const char* member) override;
    virtual bool setEdit(int ModNum) override;
};

} // namespace PartDesignGui


#endif // PARTGUI_ViewProviderSolid_H
