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

#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/TaskView/TaskView.h>

#include "TaskTextEditor.h"
#include "ui_TaskTextEditor.h"
#include "ViewProviderShapeText.h"

using namespace TextGui;
using namespace Gui::TaskView;

TextEditor::TextEditor(Text::ShapeText* shapeText, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui_TaskTextEditor)
    , shapeText(shapeText)
{
    ui->setupUi(this);
    setupDialog();
}

TextEditor::~TextEditor()
{}

void TextEditor::setupDialog()
{
    const char* str = shapeText->String.getValue();
    QString name = QString::fromUtf8(shapeText->FontName.getValue());
    QString file = QString::fromUtf8(shapeText->FontFile.getValue());
    Base::Quantity s = shapeText->Size.getQuantityValue();
    long j = shapeText->Justification.getValue();
    bool keepLeftMargin = shapeText->KeepLeftMargin.getValue();
    bool scaleToSize = shapeText->ScaleToSize.getValue();

    ui->textEdit->setPlainText(str);
    ui->changeFont->setCurrentFont(name);
    ui->fontFileChooser->setFileName(file);
    ui->sizeEdit->setValue(s);
    ui->changeJustification->setCurrentIndex(j);
    ui->checkKeepLeftMargin->setChecked(keepLeftMargin);
    ui->checkScaleToSize->setChecked(scaleToSize);

    ui->sizeEdit->bind(shapeText->Size);

    if (ui->sizeEdit->value() < Base::Quantity(Precision::Confusion(), Base::Unit::Length))
        ui->sizeEdit->setValue(5.0);

    connect(ui->textEdit, &QTextEdit::textChanged, this, &TextEditor::onTextEditChanged);
    connect(ui->changeFont, &QFontComboBox::currentFontChanged, this, &TextEditor::onFontNameChanged);
    connect(ui->fontFileChooser, &Gui::FileChooser::fileNameSelected, this, &TextEditor::onFontFileSelected);
    connect(ui->sizeEdit, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), this, &TextEditor::onSizeChanged);
    connect(ui->changeJustification, qOverload<int>(&QComboBox::currentIndexChanged), this, &TextEditor::onJustificationChanged);
}

void TextEditor::onTextEditChanged()
{
    if (shapeText.expired())
        return;

    shapeText->String.setValue(ui->textEdit->toPlainText().toStdString());
}

void TextEditor::onFontNameChanged(const QFont &font)
{
    if (shapeText.expired())
        return;

    shapeText->FontName.setValue(font.family().toStdString());
}

void TextEditor::onFontFileSelected(const QString &file)
{
    if (shapeText.expired())
        return;

    shapeText->FontFile.setValue(file.toStdString());
}

void TextEditor::onSizeChanged(double val)
{
    if (shapeText.expired())
        return;

    shapeText->Size.setValue(val);
}

void TextEditor::onJustificationChanged(int index)
{
    if (shapeText.expired())
        return;

    shapeText->Justification.setValue(index);
}

TaskTextEditor::TaskTextEditor(ViewProviderShapeText* shapeTextView)
    : TaskDialog()
    , shapeTextView(shapeTextView)
{
    assert(shapeTextView);

    QWidget* widget = new TextEditor(shapeTextView->getShapeText());
    TaskBox* taskbox =
        new TaskBox(Gui::BitmapFactory().pixmap("Text_ShapeText"), tr("Edit text"), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskTextEditor::~TaskTextEditor()
{
}

//==== calls from the TaskView ===============================================================


void TaskTextEditor::open()
{}

void TaskTextEditor::clicked(int)
{}

bool TaskTextEditor::accept()
{
    return true;
}

bool TaskTextEditor::reject()
{
    std::string document = getDocumentName();// needed because resetEdit() deletes this instance
    Gui::Command::doCommand(
        Gui::Command::Gui, "Gui.getDocument('%s').resetEdit()", document.c_str());
    Gui::Command::doCommand(
        Gui::Command::Doc, "App.getDocument('%s').recompute()", document.c_str());

    return true;
}


#include "moc_TaskTextEditor.cpp"
