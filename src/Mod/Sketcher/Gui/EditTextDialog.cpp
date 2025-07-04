// SPDX - License - Identifier: LGPL - 2.1 - or -later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Pierre-Louis Boyer                                  *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QComboBox>
#include <QLineEdit>
#endif

#include <Gui/CommandT.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "EditTextDialog.h"
#include "ViewProviderSketch.h"
#include "Utils.h"
#include "ui_EditTextDialog.h"

using namespace SketcherGui;

EditTextDialog::EditTextDialog(ViewProviderSketch* viewProvider,
                               int constraintIndex,
                               QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::EditTextDialog)
    , sketchView(viewProvider)
    , constrIndex(constraintIndex)
{
    ui->setupUi(this);

    const Sketcher::SketchObject* sketch = sketchView->getSketchObject();
    const Sketcher::Constraint* constraint = sketch->Constraints[constrIndex];

    // Initialize Text
    ui->lineEdit_text->setText(QString::fromStdString(constraint->Text));

    // Initialize Font
    populateFontList();
    QString currentFontName = findFontNameFromPath(QString::fromStdString(constraint->Font));
    if (!currentFontName.isEmpty()) {
        ui->comboBox_font->setCurrentText(currentFontName);
    }
}

EditTextDialog::~EditTextDialog()
{
    delete ui;
}

void EditTextDialog::populateFontList()
{
    fontPathMap = findAvailableFontFiles();
    QStringList fontNames = fontPathMap.keys();
    fontNames.sort(Qt::CaseInsensitive);
    ui->comboBox_font->addItems(fontNames);
}

QString EditTextDialog::findFontNameFromPath(const QString& path) const
{
    return fontPathMap.key(path, QString());
}

void EditTextDialog::on_buttonBox_accepted()
{
    const Sketcher::SketchObject* sketch = sketchView->getSketchObject();

    // Get new values from the dialog
    std::string newText = ui->lineEdit_text->text().toStdString();
    QString selectedFontName = ui->comboBox_font->currentText();
    std::string newFontPath = fontPathMap.value(selectedFontName).toStdString();

    const Sketcher::Constraint* constraint = sketch->Constraints[constrIndex];

    // Check if anything changed
    if (newText == constraint->Text && newFontPath == constraint->Font) {
        return;  // Nothing to do
    }

    // Open a command to make the change undo-able
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Modify sketch text constraint"));

    try {
        Gui::cmdAppObjectArgs(sketch,
                              "setTextAndFont(%i, '%s', '%s')",
                              constrIndex,
                              newText,
                              newFontPath);

        Gui::Command::commitCommand();
    }
    catch (const Base::Exception& e) {
        Gui::Command::abortCommand();
        Base::Console().error("Failed to modify text constraint: %s\n", e.what());
    }
}
