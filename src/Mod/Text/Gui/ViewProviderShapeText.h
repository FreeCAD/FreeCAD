/***************************************************************************
 *   Copyright (c) 2024 Martin Rodriguez Reboredo <yakoyoku@gmail.com>     *
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

#ifndef TEXTGUI_ViewProviderShapeText_H
#define TEXTGUI_ViewProviderShapeText_H

#include <QCoreApplication>

#include <Gui/Document.h>
#include <Gui/ViewProviderFeaturePython.h>
#include <Mod/Part/Gui/ViewProvider2DObject.h>
#include <Mod/Part/Gui/ViewProviderAttachExtension.h>
#include <Mod/Text/TextGlobal.h>


namespace Text {
class ShapeText;
}

namespace TextGui {

class TextGuiExport ViewProviderShapeText : public PartGui::ViewProvider2DObject,
                                            public PartGui::ViewProviderAttachExtension
{
    Q_DECLARE_TR_FUNCTIONS(TextGui::ViewProviderShapeText)

    PROPERTY_HEADER_WITH_OVERRIDE(TextGui::ViewProviderShapeText);

public:
    ViewProviderShapeText();
    ~ViewProviderShapeText() override;

public:
    Text::ShapeText* getShapeText() const;

    void attach(App::DocumentObject* pcFeat) override;
    const char* getTransactionText() const override
    {
        return nullptr;
    }
};

using ViewProviderPython = Gui::ViewProviderFeaturePythonT<ViewProviderShapeText>;

} //namespace TextGui


#endif // TEXTGUI_ViewProviderShapeText_H
