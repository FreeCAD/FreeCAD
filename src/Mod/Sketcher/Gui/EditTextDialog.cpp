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
# include <QComboBox>
# include <QLineEdit>
#endif

#include <Gui/CommandT.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "EditTextDialog.h"
#include "ViewProviderSketch.h"
#include "Utils.h"
#include "ui_EditTextDialog.h"

using namespace SketcherGui;

EditTextDialog::EditTextDialog(ViewProviderSketch* viewProvider, int constraintIndex, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::EditTextDialog)
    , sketchView(viewProvider)
    , constrIndex(constraintIndex)
{
    ui->setupUi(this);

    ui->comboBox_font->setMaxVisibleItems(20);

    const Sketcher::SketchObject* sketch = sketchView->getSketchObject();
    const Sketcher::Constraint* constraint = sketch->Constraints[constrIndex];

    // Initialize Text
    ui->lineEdit_text->setText(QString::fromStdString(constraint->getText()));

    ui->radioButton_height->setChecked(constraint->getIsTextHeight());
    ui->radioButton_width->setChecked(!constraint->getIsTextHeight());

    // Initialize Font
    populateFontList();
    QString currentFontName = findFontNameFromPath(QString::fromStdString(constraint->getFont()));
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
    bool newIsHeight = ui->radioButton_height->isChecked();

    const Sketcher::Constraint* constraint = sketch->Constraints[constrIndex];

    // Check if anything changed
    if (newText == constraint->getText() && newFontPath == constraint->getFont()
        && newIsHeight == constraint->getIsTextHeight()) {
        return;  // Nothing to do
    }

    // Open a command to make the change undo-able
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Modify sketch text constraint"));

    try {
        // Find if it was construction geometry to preserve that state
        int firstTextGeoId = constraint->getGeoId(1);
        bool isConstruction = false;
        if (firstTextGeoId != Sketcher::GeoEnum::GeoUndef) {
            isConstruction = Sketcher::GeometryFacade::getConstruction(
                sketch->getGeometry(firstTextGeoId)
            );
        }

        // Send the updated 5-parameter call to Python
        Gui::cmdAppObjectArgs(
            sketch,
            "setTextAndFont(%i, '%s', '%s', %s, %s)",
            constrIndex,
            newText.c_str(),
            newFontPath.c_str(),
            newIsHeight ? "True" : "False",
            isConstruction ? "True" : "False"
        );

        Gui::Command::commitCommand();
    }
    catch (const Base::Exception& e) {
        Gui::Command::abortCommand();
        Base::Console().error("Failed to modify text constraint: %s\n", e.what());
    }
}
