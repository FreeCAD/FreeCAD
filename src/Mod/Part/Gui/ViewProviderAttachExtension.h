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


#ifndef GUI_VIEWPROVIDERATTACHEXTENSION_H
#define GUI_VIEWPROVIDERATTACHEXTENSION_H

#include <App/Extension.h>
#include <Gui/ViewProviderExtension.h>

namespace PartGui
{

class PartGuiExport ViewProviderAttachExtension : public Gui::ViewProviderExtension
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(PartGui::ViewProviderAttachExtension);

public:
    /// Constructor
    ViewProviderAttachExtension(void);
    virtual ~ViewProviderAttachExtension() = default;

    virtual QIcon extensionMergeOverlayIcons(const QIcon & orig) const override;

    virtual void extensionUpdateData(const App::Property*) override;

};

typedef Gui::ViewProviderExtensionPythonT<PartGui::ViewProviderAttachExtension> ViewProviderAttachExtensionPython;

} //namespace Part::Gui

#endif // GUI_VIEWPROVIDERATTACHMENTEXTENSION_H
