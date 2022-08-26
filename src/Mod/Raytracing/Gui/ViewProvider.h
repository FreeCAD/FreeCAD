/***************************************************************************
 *   Copyright (c) 2013 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef RAYTRACINGGUI_VIEWPROVIDER_H
#define RAYTRACINGGUI_VIEWPROVIDER_H


#include <Gui/ViewProviderDocumentObjectGroup.h>
#include <QCoreApplication>

namespace RaytracingGui {

class ViewProviderLux : public Gui::ViewProviderDocumentObjectGroup
{
    PROPERTY_HEADER_WITH_OVERRIDE(RaytracingGui::ViewProviderLux);
    Q_DECLARE_TR_FUNCTIONS(RaytracingGui::ViewProviderLux)

public:
    ViewProviderLux();
    ~ViewProviderLux() override;

    bool doubleClicked(void) override;
    void setupContextMenu(QMenu* menu, QObject* receiver, const char* member) override;

protected:
    bool setEdit(int ModNum) override;
    void unsetEdit(int ModNum) override;
};

class ViewProviderPovray : public Gui::ViewProviderDocumentObjectGroup
{
    PROPERTY_HEADER_WITH_OVERRIDE(RaytracingGui::ViewProviderPovray);
    Q_DECLARE_TR_FUNCTIONS(RaytracingGui::ViewProviderPovray)

public:
    ViewProviderPovray();
    ~ViewProviderPovray() override;

    bool doubleClicked(void) override;
    void setupContextMenu(QMenu* menu, QObject* receiver, const char* member) override;

protected:
    bool setEdit(int ModNum) override;
    void unsetEdit(int ModNum) override;
};

} // namespace RaytracingGui

#endif // RAYTRACINGGUI_VIEWPROVIDER_H

