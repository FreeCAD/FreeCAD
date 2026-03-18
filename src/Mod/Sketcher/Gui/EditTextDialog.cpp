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

#include <QCheckBox>

#include <Gui/CommandT.h>
#include <Mod/Sketcher/App/Constraint.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "CommandConstraints.h"
#include "EditTextDialog.h"
#include "Utils.h"
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

    // Height/Width radio buttons removed — always use height mode

    // Initialize Font
    populateFontList();
    QString currentFontName = findFontNameFromPath(QString::fromStdString(constraint->getFont()));
    if (!currentFontName.isEmpty()) {
        ui->comboBox_font->setCurrentText(currentFontName);
    }

    // Initialize helper flags
    Sketcher::HelperFlags flags = constraint->getHelperFlags();
    ui->checkBox_bboxBottom->setChecked(flags.testFlag(Sketcher::HelperFlag::BBoxBottom));
    ui->checkBox_bboxTop->setChecked(flags.testFlag(Sketcher::HelperFlag::BBoxTop));
    ui->checkBox_bboxLeft->setChecked(flags.testFlag(Sketcher::HelperFlag::BBoxLeft));
    ui->checkBox_bboxRight->setChecked(flags.testFlag(Sketcher::HelperFlag::BBoxRight));
    ui->checkBox_baseline->setChecked(flags.testFlag(Sketcher::HelperFlag::MetricBaseline));
    ui->checkBox_xHeight->setChecked(flags.testFlag(Sketcher::HelperFlag::MetricXHeight));
    ui->checkBox_capHeight->setChecked(flags.testFlag(Sketcher::HelperFlag::MetricCapHeight));

    // Gray out metric checkboxes for metrics the font doesn't provide
    if (!constraint->getHasXHeight()) {
        ui->checkBox_xHeight->setEnabled(false);
        ui->checkBox_xHeight->setChecked(false);
    }
    if (!constraint->getHasCapHeight()) {
        ui->checkBox_capHeight->setEnabled(false);
        ui->checkBox_capHeight->setChecked(false);
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
    Sketcher::SketchObject* sketch = sketchView->getSketchObject();
    const Sketcher::Constraint* constraint = sketch->Constraints[constrIndex];

    // Get new values from the dialog
    std::string newText = ui->lineEdit_text->text().toStdString();
    QString selectedFontName = ui->comboBox_font->currentText();
    std::string newFontPath = fontPathMap.value(selectedFontName).toStdString();

    // Collect helper flags from checkboxes
    Sketcher::HelperFlags newFlags;
    if (ui->checkBox_bboxBottom->isChecked()) {
        newFlags.setFlag(Sketcher::HelperFlag::BBoxBottom);
    }
    if (ui->checkBox_bboxTop->isChecked()) {
        newFlags.setFlag(Sketcher::HelperFlag::BBoxTop);
    }
    if (ui->checkBox_bboxLeft->isChecked()) {
        newFlags.setFlag(Sketcher::HelperFlag::BBoxLeft);
    }
    if (ui->checkBox_bboxRight->isChecked()) {
        newFlags.setFlag(Sketcher::HelperFlag::BBoxRight);
    }
    if (ui->checkBox_baseline->isChecked()) {
        newFlags.setFlag(Sketcher::HelperFlag::MetricBaseline);
    }
    if (ui->checkBox_xHeight->isChecked()) {
        newFlags.setFlag(Sketcher::HelperFlag::MetricXHeight);
    }
    if (ui->checkBox_capHeight->isChecked()) {
        newFlags.setFlag(Sketcher::HelperFlag::MetricCapHeight);
    }

    Sketcher::HelperFlags oldFlags = constraint->getHelperFlags();

    bool textChanged = newText != constraint->getText() || newFontPath != constraint->getFont();
    bool helpersChanged = !newFlags.isEqual(oldFlags);

    if (!textChanged && !helpersChanged) {
        return;  // Nothing to do
    }

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Modify sketch text constraint"));

    try {
        if (textChanged) {
            // Find if it was construction geometry to preserve that state
            int firstTextGeoId = constraint->getGeoId(1);
            bool isConstruction = false;
            if (firstTextGeoId != Sketcher::GeoEnum::GeoUndef) {
                isConstruction = Sketcher::GeometryFacade::getConstruction(
                    sketch->getGeometry(firstTextGeoId)
                );
            }

            std::string escText = escapeForPython(newText);
            std::string escFont = escapeForPython(newFontPath);
            Gui::cmdAppObjectArgs(
                sketch,
                "setTextAndFont(%i, '%s', '%s', %s, %i)",
                constrIndex,
                escText.c_str(),
                escFont.c_str(),
                isConstruction ? "True" : "False",
                newFlags.toUnderlyingType()
            );
        }
        else if (helpersChanged) {
            // Store the flags via Python (for undo journaling)
            Gui::cmdAppObjectArgs(
                sketch,
                "updateGroupHelperLines(%i, %i)",
                constrIndex,
                newFlags.toUnderlyingType()
            );
        }

        // Toggle visibility using shared utility
        applyHelperVisibility(
            sketch,
            constrIndex,
            newFlags,
            Sketcher::HelperOrder.data(),
            static_cast<int>(Sketcher::HelperOrder.size())
        );

        Gui::Command::commitCommand();
    }
    catch (const Base::Exception& e) {
        Gui::Command::abortCommand();
        Base::Console().error("Failed to modify text constraint: %s\n", e.what());
    }
}
