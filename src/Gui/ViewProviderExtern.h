/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
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


#ifndef GUI_VIEWPROVIDER_EXTERN_H
#define GUI_VIEWPROVIDER_EXTERN_H

#include "ViewProvider.h"


class SoNode;
class SoInput;

namespace Gui {


class GuiExport ViewProviderExtern:public ViewProvider
{
    PROPERTY_HEADER_WITH_OVERRIDE(Gui::ViewProviderExtern);

public:
    /// constructor.
    ViewProviderExtern();

    /// destructor.
    ~ViewProviderExtern() override;

    void setModeByString(const char* name, const char* ivFragment);
    void setModeByFile(const char* name, const char* ivFileName);
    void setModeBySoInput(const char* name, SoInput &ivFileInput);
    void adjustDocumentName(const char* docname);

    const char* getDefaultDisplayMode() const override;
    std::vector<std::string> getDisplayModes() const override;
    void updateData(const App::Property*) override{}

private:
    void adjustRecursiveDocumentName(SoNode*, const char* docname);

protected:
    std::vector<std::string> modes;
};

} // namespace Gui

#endif // GUI_VIEWPROVIDER_EXTERN_H

