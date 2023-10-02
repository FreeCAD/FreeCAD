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

#include "PreCompiled.h"

#ifndef _PreComp_
# include <QMenu>
# include <QPlainTextEdit>
#endif

#include <App/Application.h>
#include <Base/Type.h>

#include "ViewProviderTextDocument.h"
#include "ActionFunction.h"
#include "Document.h"
#include "MainWindow.h"
#include "PythonEditor.h"
#include "TextDocumentEditorView.h"
#include "ViewProviderDocumentObject.h"


using namespace Gui;

PROPERTY_SOURCE(Gui::ViewProviderTextDocument, Gui::ViewProviderDocumentObject)
const char* ViewProviderTextDocument::SyntaxEnums[]= {"None","Python",nullptr};

ViewProviderTextDocument::ViewProviderTextDocument()
{
    sPixmap = "TextDocument";

    ADD_PROPERTY_TYPE(
            ReadOnly, (false), "Editor", App::Prop_None,
            "Defines whether the content can be edited.");

    QFont font;
    font.setFamily(QString::fromLatin1(App::GetApplication().GetUserParameter().
        GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Editor")->GetASCII("Font", font.family().toLatin1()).c_str()));
    font.setPointSize(App::GetApplication().GetUserParameter().
        GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Editor")->GetInt("FontSize", font.pointSize()));

    ADD_PROPERTY_TYPE(FontSize,(font.pointSize()), "Editor", App::Prop_None, "Font size");
    ADD_PROPERTY_TYPE(FontName,((const char*)font.family().toLatin1()), "Editor", App::Prop_None, "Font name");

    ADD_PROPERTY_TYPE(SyntaxHighlighter,(static_cast<long>(0)), "Editor", App::Prop_None, "Syntax highlighting");
    SyntaxHighlighter.setEnums(SyntaxEnums);

    DisplayMode.setStatus(App::Property::Hidden, true);
    OnTopWhenSelected.setStatus(App::Property::Hidden, true);
    SelectionStyle.setStatus(App::Property::Hidden, true);
    Visibility.setStatus(App::Property::Hidden, true);
}

void ViewProviderTextDocument::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    auto func = new Gui::ActionFunction(menu);
    QAction* act = menu->addAction(QObject::tr("Edit text"));
    func->trigger(act, [this](){
        this->doubleClicked();
    });

    ViewProviderDocumentObject::setupContextMenu(menu, receiver, member);
}

bool ViewProviderTextDocument::doubleClicked()
{
    if (!activateView()) {
        editorWidget = new QPlainTextEdit {};
        editorWidget->setReadOnly(ReadOnly.getValue());
        FontName.touch();
        SyntaxHighlighter.touch();

        getMainWindow()->addWindow(
            new TextDocumentEditorView {
                static_cast<App::TextDocument*>(getObject()),
                editorWidget, getMainWindow()});
    }
    return true;
}

void ViewProviderTextDocument::onChanged(const App::Property* prop)
{
    if (editorWidget) {
        if (prop == &ReadOnly) {
            editorWidget->setReadOnly(ReadOnly.getValue());
        }
        else if (prop == &FontSize || prop == &FontName) {
            QFont font(QString::fromLatin1(this->FontName.getValue()), (int)this->FontSize.getValue());
            editorWidget->setFont(font);
        }
        else if (prop == &SyntaxHighlighter) {
            long value = SyntaxHighlighter.getValue();
            if (value == 1) {
                auto pythonSyntax = new PythonSyntaxHighlighter(editorWidget);
                pythonSyntax->setDocument(editorWidget->document());
            }
            else {
                auto shl = editorWidget->findChild<QSyntaxHighlighter*>();
                if (shl)
                    shl->deleteLater();
            }
        }
    }
    ViewProviderDocumentObject::onChanged(prop);
}

MDIView* ViewProviderTextDocument::getMDIView() const
{
    auto views = getDocument()->getMDIViewsOfType(
            TextDocumentEditorView::getClassTypeId());
    for (auto v : views) {
        auto textView = static_cast<TextDocumentEditorView *>(v);
        if (textView->getTextObject() == getObject()) {
            return textView;
        }
    }
    return nullptr;
}

bool ViewProviderTextDocument::activateView() const
{
    auto views = getDocument()->getMDIViewsOfType(
            TextDocumentEditorView::getClassTypeId());
    for (auto v : views) {
        auto textView = static_cast<TextDocumentEditorView *>(v);
        if (textView->getTextObject() == getObject()) {
            getMainWindow()->setActiveWindow(textView);
            return true;
        }
    }
    return false;
}
