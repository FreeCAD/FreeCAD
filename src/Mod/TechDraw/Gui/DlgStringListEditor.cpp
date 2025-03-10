/****************************************************************************
 *   Copyright (c) 2022 Wanderer Fan <wandererfan@gmail.com>                *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QDialogButtonBox>
#include <QVBoxLayout>
#endif

#include "DlgStringListEditor.h"

using namespace TechDrawGui;

/* TRANSLATOR Gui::DlgStringListEditor */

DlgStringListEditor::DlgStringListEditor(const std::vector<std::string> texts,
                                         QWidget* parent,
                                         Qt::WindowFlags fl)
    : QDialog(parent, fl)
    , textEdit_(new QPlainTextEdit(this))
{
    auto buttons = new QDialogButtonBox(QDialogButtonBox::StandardButton::Ok
                                        | QDialogButtonBox::StandardButton::Cancel);
    auto layout = new QVBoxLayout();
    layout->addWidget(textEdit_);
    layout->addWidget(buttons);
    setLayout(layout);
    fillList(texts);
    connect(buttons, &QDialogButtonBox::accepted, this, &DlgStringListEditor::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &DlgStringListEditor::reject);
}

void DlgStringListEditor::fillList(std::vector<std::string> texts)
{
    QStringList lines;
    lines.reserve(static_cast<int>(texts.size()));
    for (auto const& text : texts) {
        lines.append(QString::fromStdString(text));
    }
    textEdit_->setPlainText(lines.join(QChar::SpecialCharacter::LineFeed));
}

std::vector<std::string> DlgStringListEditor::getTexts() const
{
    return texts_;
}

void DlgStringListEditor::accept()
{
    auto lines = textEdit_->toPlainText().split(QChar::SpecialCharacter::LineFeed);
    texts_.clear();
    texts_.reserve(static_cast<size_t>(lines.size()));
    for (auto const& line : lines) {
        texts_.push_back(line.toStdString());
    }
    QDialog::accept();
}

void DlgStringListEditor::reject()
{
    QDialog::reject();
}
