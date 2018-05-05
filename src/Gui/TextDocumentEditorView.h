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


#ifndef GUI_TEXTDOCUMENTEDITORVIEW_H
#define GUI_TEXTDOCUMENTEDITORVIEW_H

#include "PreCompiled.h"

#include <string>
#include <boost/signals2.hpp>
#include <QPlainTextEdit>

#include <App/TextDocument.h>
#include <Gui/MDIView.h>
#include <Gui/Window.h>


namespace Gui {

class GuiExport TextDocumentEditorView : public MDIView {
    Q_OBJECT
public:
    TextDocumentEditorView(
            App::TextDocument* textDocument,
            QPlainTextEdit* editor, QWidget* parent);
    ~TextDocumentEditorView();
    const char *getName() const override { return "TextDocumentEditorView"; }
    bool onMsg(const char* msg, const char**) override;
    bool onHasMsg(const char* msg) const override;
    bool canClose() override;

    bool event(QEvent *event) override;

    QPlainTextEdit* getEditor() const { return editor; }
    App::TextDocument* getTextObject() const { return textDocument; }
private:
    void setupEditor();
    void setupConnection();
    void saveToObject();
    void sourceChanged();
    void refresh();
    bool isEditorModified() const;
    QPlainTextEdit *const editor;
    App::TextDocument *const textDocument;
    boost::signals2::connection textConnection;
    bool sourceModified = false;
};

}

#endif
