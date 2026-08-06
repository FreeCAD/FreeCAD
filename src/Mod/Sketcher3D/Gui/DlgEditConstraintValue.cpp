// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Yash Suthar <yashsuthar983@gmail.com>              *
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

#include <QByteArray>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QPushButton>
#include <QVBoxLayout>

#include <Base/Quantity.h>
#include <Gui/PrefWidgets.h>

#include "DlgEditConstraintValue.h"


using namespace Sketcher3DGui;

namespace
{

QByteArray historyPathForUnit(const Base::Unit& unit)
{
    if (unit == Base::Unit::Angle) {
        return QByteArray("User parameter:BaseApp/History/SketcherAngle");
    }
    if (unit == Base::Unit::Length) {
        return QByteArray("User parameter:BaseApp/History/SketcherLength");
    }
    return QByteArray("User parameter:BaseApp/History/Sketcher3DDatum");
}

}  // namespace


DlgEditConstraintValue::DlgEditConstraintValue(
    const QString& title,
    const QString& label,
    double initialValue,
    const Base::Unit& u,
    QWidget* parent,
    bool allowNegative
)
    : QDialog(parent)
{
    setWindowTitle(title);
    setModal(true);

    auto* root = new QVBoxLayout(this);

    auto* form = new QFormLayout();
    spin = new Gui::PrefQuantitySpinBox(this);
    spin->setUnit(u);
    spin->setParamGrpPath(historyPathForUnit(u));
    spin->setEntryName(QByteArray("DatumValue"));
    spin->onRestore();
    spin->setValue(Base::Quantity(initialValue, u));
    const double minValue = (u == Base::Unit::Length && !allowNegative) ? 0.0 : -1.0e9;
    spin->setMinimum(minValue);
    spin->setMaximum(1.0e9);
    spin->selectNumber();
    form->addRow(label, spin);
    root->addLayout(form);

    auto* buttons
        = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    connect(buttons, &QDialogButtonBox::accepted, this, [this] {
        spin->pushToHistory();
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    if (auto* okBtn = buttons->button(QDialogButtonBox::Ok)) {
        okBtn->setDefault(true);
    }
    root->addWidget(buttons);

    spin->setFocus();
}

DlgEditConstraintValue::~DlgEditConstraintValue() = default;

double DlgEditConstraintValue::value() const
{
    return spin ? spin->rawValue() : 0.0;
}

#include "moc_DlgEditConstraintValue.cpp"
