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

#pragma once

#include <QPlainTextEdit>

#include <App/TextDocument.h>
#include <Gui/MDIView.h>


namespace Gui
{

class GuiExport TextDocumentEditorView: public MDIView
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(TextDocumentEditorView)
    TYPESYSTEM_HEADER_WITH_OVERRIDE();  // NOLINT

public:
    TextDocumentEditorView(App::TextDocument* textDocument, QPlainTextEdit* editor, QWidget* parent);
    ~TextDocumentEditorView() override;
    const char* getName() const override
    {
        return "TextDocumentEditorView";
    }
    bool onMsg(const char* msg, const char** output) override;
    bool onHasMsg(const char* msg) const override;

    QPlainTextEdit* getEditor() const
    {
        return editor;
    }
    App::TextDocument* getTextObject() const
    {
        return textDocument;
    }
    QStringList undoActions() const override;
    QStringList redoActions() const override;

protected:
    void showEvent(QShowEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private:
    void setupEditor();
    void setupConnection();
    void saveToObject();
    void sourceChanged();
    void textChanged();
    void labelChanged();
    void refresh();

private:
    QPlainTextEdit* const editor;
    App::TextDocument* const textDocument;
    fastsignals::advanced_connection textConnection;
    fastsignals::connection labelConnection;
    bool aboutToClose = false;
};

}  // namespace Gui
