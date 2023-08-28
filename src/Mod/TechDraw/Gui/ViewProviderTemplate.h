/***************************************************************************
 *   Copyright (c) 2014 Luke Parry <l.parry@warwick.ac.uk>                 *
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

#ifndef DRAWINGGUI_VIEWPROVIDERTEMPLATE_H
#define DRAWINGGUI_VIEWPROVIDERTEMPLATE_H

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <Gui/ViewProviderDocumentObject.h>

#include "ViewProviderTemplateExtension.h"
namespace TechDraw{
    class DrawTemplate;
}

namespace TechDrawGui {
class QGITemplate;
class MDIViewPage;

class TechDrawGuiExport ViewProviderTemplate : public Gui::ViewProviderDocumentObject,
                                               public ViewProviderTemplateExtension
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDrawGui::ViewProviderTemplate);

public:
    /// constructor
    ViewProviderTemplate();
    /// destructor
    ~ViewProviderTemplate() override = default;

    void attach(App::DocumentObject *) override;

    bool useNewSelectionModel() const override {return false;}
    void updateData(const App::Property*) override;
    void onChanged(const App::Property *prop) override;
    void hide() override;
    void show() override;
    bool isShow() const override;
    QGITemplate* getQTemplate(void);
    TechDraw::DrawTemplate* getTemplate() const;
    MDIViewPage* getMDIViewPage(void) const;
    Gui::MDIView *getMDIView() const override;

    void setMarkers(bool state);
    bool onDelete(const std::vector<std::string> &) override;

    const char* whoAmI() const;

private:
    std::string m_myName;
};

} // namespace TechDrawGui


#endif // DRAWINGGUI_VIEWPROVIDERTEMPLATE_H

