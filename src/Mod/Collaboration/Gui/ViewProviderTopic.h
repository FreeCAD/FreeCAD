// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Pieter Hijma <info@pieterhijma.net>                 *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#ifndef COLLABORATION_GUI_VIEWPROVIDER_TOPIC_H
#define COLLABORATION_GUI_VIEWPROVIDER_TOPIC_H

#include <Gui/Utilities.h>

#include <Gui/ViewProviderAnnotation.h>
#include <Gui/ViewProviderGroupExtension.h>

#include <Mod/Collaboration/CollaborationGlobal.h>

namespace CollaborationGui
{

class CollaborationGuiExport ViewProviderTopic: public Gui::ViewProviderAnnotationLabel,
                                                public Gui::ViewProviderGroupExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(Gui::ViewProviderAnnotationLabel);

public:
    ViewProviderTopic();
    ~ViewProviderTopic() override;

    static inline const std::string NoneDisplayMode = "None";

    std::vector<std::string> getDisplayModes() const override;
    void setDisplayMode(const char* mode) override;
    void updateData(const App::Property* prop) override;
    bool doubleClicked() override;

protected:
    void labelDoubleClicked() override;

private:
    bool updateBasePosition(const App::Property* prop);
};

}  // namespace CollaborationGui

#endif  // GUI_VIEWPROVIDER_TOPIC_H
