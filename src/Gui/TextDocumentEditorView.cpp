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

#include <iostream>
#include <boost/bind.hpp>
#include <boost/signals2.hpp>
#include <QString>
#include <QMessageBox>
#include <QPushButton>

#include <App/TextDocument.h>
#include <Gui/Document.h>
#include <Gui/Application.h>
#include <Gui/MainWindow.h>

#include "TextDocumentEditorView.h"


using namespace Gui;

TextDocumentEditorView::TextDocumentEditorView(
        App::TextDocument* txtDoc, QPlainTextEdit* e,
        QWidget* parent)
    : MDIView(
            Application::Instance->getDocument(txtDoc->getDocument()),
            parent),
    editor {e}, textDocument {txtDoc}
{
    setupEditor();
    setupConnection();
    setCentralWidget(editor);
}

TextDocumentEditorView::~TextDocumentEditorView()
{
    textConnection.disconnect();
}

bool TextDocumentEditorView::event(QEvent *event)
{
    if (event->type() == QEvent::Show && sourceModified) {
        refresh();
        sourceModified = false;
    }
    return MDIView::event(event);
}

void TextDocumentEditorView::setupEditor()
{
    connect(getEditor()->document(), SIGNAL(modificationChanged(bool)),
            this, SLOT(setWindowModified(bool)));
    getEditor()->setReadOnly(textDocument->ReadOnly.getValue());
    setWindowTitle(QString::fromLatin1(textDocument->Label.getValue())
            + QString::fromLatin1("[*]"));
    getEditor()->setPlainText(
            QString::fromUtf8(textDocument->Text.getValue()));
}

void TextDocumentEditorView::setupConnection()
{
    textConnection = textDocument->connect(
            boost::bind(&TextDocumentEditorView::sourceChanged, this));
}

void TextDocumentEditorView::sourceChanged()
{
    if (getMainWindow()->activeWindow() == this) {
        refresh();
        sourceModified = false;
    } else {
        sourceModified = true;
    }
}

void TextDocumentEditorView::refresh()
{
    QString text = QString::fromStdString(
            textDocument->Text.getStrValue());
    if (isEditorModified()) {
        QMessageBox msgBox {this};
        msgBox.setWindowTitle(QString::fromUtf8("Text updated"));
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setText(QString::fromUtf8(
                    "The text of the underlying object has changed. "
                    "Discard changes and reload the text from the object?"));
        QPushButton* yesBtt = msgBox.addButton(
                QString::fromUtf8("Yes, reload."), QMessageBox::YesRole);
        QPushButton* noBtt = msgBox.addButton(
                QString::fromUtf8("No"), QMessageBox::NoRole);
        msgBox.exec();
        if (msgBox.clickedButton() == noBtt)
            return;
    }
    getEditor()->setPlainText(text);
}

bool TextDocumentEditorView::onMsg(const char* msg, const char**)
{
    if (strcmp(msg,"Save") == 0) {
        saveToObject();
        return true;
    }
    return false;
}

bool TextDocumentEditorView::isEditorModified() const
{
    return getEditor()->document()->isModified();
}

bool TextDocumentEditorView::onHasMsg(const char* msg) const
{
    if (strcmp(msg,"Save") == 0)
        return isEditorModified();
    return false;
}

bool TextDocumentEditorView::canClose()
{
    if (!getEditor()->document()->isModified())
        return true;

    this->setFocus();

    QString question {
        tr("The document has been modified.\n"
                "Do you want to save your changes?")};
    auto reply = QMessageBox::question(
            this, tr("Unsaved document"), question,
            QMessageBox::Yes|QMessageBox::Default, QMessageBox::No,
            QMessageBox::Cancel|QMessageBox::Escape);
    if (reply == QMessageBox::Yes)
        saveToObject();
    return reply != QMessageBox::Cancel;
}

void TextDocumentEditorView::saveToObject()
{
    boost::signals2::shared_connection_block textBlock {textConnection};
    textDocument->Text.setValue(
            getEditor()->document()->toPlainText().toStdString());
    getEditor()->document()->setModified(false);
}


#include "moc_TextDocumentEditorView.cpp"
