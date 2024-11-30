/***************************************************************************
 *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
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

#ifndef GUI_ViewProviderTextDocument_H
#define GUI_ViewProviderTextDocument_H

#include <QPointer>
#include "ViewProviderDocumentObject.h"


class QPlainTextEdit;

namespace Gui {

class GuiExport ViewProviderTextDocument : public ViewProviderDocumentObject {
    PROPERTY_HEADER_WITH_OVERRIDE(Gui::ViewProviderTextDocument);
public:
    ViewProviderTextDocument();
    ~ViewProviderTextDocument() override = default;

    App::PropertyBool ReadOnly;
    App::PropertyFloat FontSize;
    App::PropertyFont FontName;
    App::PropertyEnumeration SyntaxHighlighter;

    bool doubleClicked() override;
    void setupContextMenu(QMenu* menu, QObject* receiver, const char* member) override;
    bool isShow() const override { return true; }

    void onChanged(const App::Property* prop) override;

    MDIView *getMDIView() const override;

private:
    bool activateView() const;

private:
    QPointer<QPlainTextEdit> editorWidget;
    static const char* SyntaxEnums[];
};

}

#endif

