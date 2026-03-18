// SPDX - License - Identifier: LGPL - 2.1 - or -later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 FreeCAD Project Association                         *
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

#include <Gui/CommandT.h>
#include <Mod/Sketcher/App/Constraint.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "EditGroupDialog.h"
#include "Utils.h"
#include "ViewProviderSketch.h"
#include "ui_EditGroupDialog.h"

using namespace SketcherGui;

EditGroupDialog::EditGroupDialog(ViewProviderSketch* viewProvider, int constraintIndex, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::EditGroupDialog)
    , sketchView(viewProvider)
    , constrIndex(constraintIndex)
{
    ui->setupUi(this);

    const Sketcher::SketchObject* sketch = sketchView->getSketchObject();
    const Sketcher::Constraint* constraint = sketch->Constraints[constrIndex];

    Sketcher::HelperFlags flags = constraint->getHelperFlags();
    ui->checkBox_bboxBottom->setChecked(flags.testFlag(Sketcher::HelperFlag::BBoxBottom));
    ui->checkBox_bboxTop->setChecked(flags.testFlag(Sketcher::HelperFlag::BBoxTop));
    ui->checkBox_bboxLeft->setChecked(flags.testFlag(Sketcher::HelperFlag::BBoxLeft));
    ui->checkBox_bboxRight->setChecked(flags.testFlag(Sketcher::HelperFlag::BBoxRight));
}

EditGroupDialog::~EditGroupDialog()
{
    delete ui;
}

void EditGroupDialog::on_buttonBox_accepted()
{
    Sketcher::SketchObject* sketch = sketchView->getSketchObject();
    const Sketcher::Constraint* constraint = sketch->Constraints[constrIndex];

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

    Sketcher::HelperFlags oldFlags = constraint->getHelperFlags();
    if (newFlags.isEqual(oldFlags)) {
        return;
    }

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Modify group helper lines"));

    try {
        // Store flags via Python for undo journaling
        Gui::cmdAppObjectArgs(
            sketch,
            "updateGroupHelperLines(%i, %i)",
            constrIndex,
            newFlags.toUnderlyingType()
        );

        // Toggle visibility using shared utility
        applyHelperVisibility(
            sketch,
            constrIndex,
            newFlags,
            Sketcher::HelperOrder.data(),
            Sketcher::BBoxHelperCount
        );

        Gui::Command::commitCommand();
    }
    catch (const Base::Exception& e) {
        Gui::Command::abortCommand();
        Base::Console().error("Failed to modify group helper lines: %s\n", e.what());
    }
}
