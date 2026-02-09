// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
# include <algorithm>
# include <QApplication>
# include <QComboBox>
# include <QLabel>
# include <QMessageBox>
# include <QPushButton>
#endif

#include <boost/range/algorithm/transform.hpp>

#include <App/Document.h>
#include <App/License.h>
#include <Gui/BitmapFactory.h>

#include "Dialogs/DlgProjectLicence.h"
#include "ui_DlgProjectLicence.h"

using namespace Gui::Dialog;

constexpr const char* propName = "AdditionalLicenses";

/* TRANSLATOR Gui::Dialog::DlgProjectLicence */

DlgProjectLicence::DlgProjectLicence(App::Document* doc, QWidget* parent, Qt::WindowFlags fl)
    : QDialog(parent, fl)
    , _doc(doc)
    , gridLayout(nullptr)
    , textLabel(nullptr)
    , addButton(nullptr)
    , ui(new Ui_DlgProjectLicence)
{
    ui->setupUi(this);
    setupDialog();
}

DlgProjectLicence::~DlgProjectLicence() = default;

QStringList DlgProjectLicence::getLicencesFromDocument() const
{
    QStringList lics;
    if (auto list = dynamic_cast<App::PropertyStringList*>(_doc->getPropertyByName(propName))) {
        auto values = list->getValues();
        for (const auto& value : values) {
            lics << QString::fromStdString(value);
        }
    }

    return lics;
}

QStringList DlgProjectLicence::getLicencesFromDialog() const
{
    QStringList lics;
    for (const auto& it : buttonMap) {
        if (it.first->isVisible()) {
            lics << it.first->currentData().toString();
        }
    }

    lics.removeDuplicates();

    return lics;
}

void DlgProjectLicence::addLicenceItems(QComboBox* cb)
{
    // load comboBox with license names
    for (const auto& item : App::licenseItems) {
        const char* name {item.at(App::posnOfFullName)};
        QString translated = QApplication::translate("Gui::Dialog::DlgSettingsDocument", name);
        cb->addItem(translated, QByteArray(name));
    }
}

void DlgProjectLicence::setLicenceIndex(QComboBox* cb, const QString& lic)
{
    // set default position to match document
    int index = cb->findData(lic.toUtf8());
    if (index >= 0) {
        cb->setCurrentIndex(index);
    }
    else {
        index = cb->count();
        cb->addItem(lic, lic.toUtf8());
        cb->setCurrentIndex(index);
    }
}

void DlgProjectLicence::addComboBoxes(const QStringList& lics)
{
    const int numLicences = lics.size();
    for (int row = 0; row < maxLicences; row++) {
        auto licence = new QComboBox(this);
        addLicenceItems(licence);

        gridLayout->addWidget(licence, row, 0, 1, 1);
        auto removeButton = new QPushButton(this);
        removeButton->setIcon(BitmapFactory().iconFromTheme("list-remove"));
        gridLayout->addWidget(removeButton, row, 1, 1, 1);

        if (row < numLicences) {
            setLicenceIndex(licence, lics[row]);
        }
        else {
            licence->hide();
            removeButton->hide();
        }

        buttonMap.append(qMakePair(licence, removeButton));
        connect(removeButton, &QPushButton::clicked, this, &DlgProjectLicence::removeLicence);
    }
}

void DlgProjectLicence::addStandardButtons(int numLicences)
{
    textLabel = new QLabel(this);
    textLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    textLabel->setText(tr("Add or remove licence"));
    addButton = new QPushButton(this);
    addButton->setIcon(BitmapFactory().iconFromTheme("list-add"));
    gridLayout->addWidget(textLabel, maxLicences, 0, 1, 1);
    gridLayout->addWidget(addButton, maxLicences, 1, 1, 1);

    connect(addButton, &QPushButton::clicked, this, &DlgProjectLicence::addLicence);
    if (numLicences >= maxLicences) {
        addButton->setDisabled(true);
    }
}

void DlgProjectLicence::setupDialog()
{
    const QStringList lics = getLicencesFromDocument();
    const int numLicences = lics.size();

    gridLayout = new QGridLayout();
    auto mainLayout = new QGridLayout(ui->groupBoxMain);
    mainLayout->addLayout(gridLayout, 0, 0, 1, 1);
    auto vSpacer = new QSpacerItem(20, 100, QSizePolicy::Minimum, QSizePolicy::Expanding);  // NOLINT
    mainLayout->addItem(vSpacer, 1, 0, 1, 1);

    addComboBoxes(lics);
    addStandardButtons(numLicences);
}

void DlgProjectLicence::addLicence()
{
    int countHidden = -1;
    for (const auto& it : std::as_const(buttonMap)) {
        if (it.first->isHidden()) {
            countHidden++;
            if (countHidden == 0) {
                QComboBox* licence = it.first;
                licence->setVisible(true);
                QPushButton* removeButton = it.second;
                removeButton->setVisible(true);
            }
        }
    }

    if (countHidden <= 0) {
        addButton->setDisabled(true);
    }
}

void DlgProjectLicence::removeLicence()
{
    auto remove = qobject_cast<QPushButton*>(sender());
    QComboBox* licence = nullptr;
    for (const auto& it : std::as_const(buttonMap)) {
        if (it.second == remove) {
            licence = it.first;
            addButton->setEnabled(true);
            licence->hide();
            remove->hide();
        }
    }
}

void DlgProjectLicence::setLicencesToDocument(const QStringList& lics)
{
    auto prop = dynamic_cast<App::PropertyStringList*>(_doc->getPropertyByName(propName));
    if (!prop && lics.isEmpty()) {
        // nothing to do
        return;
    }

    if (!prop) {
        prop = dynamic_cast<App::PropertyStringList*>(
            _doc->addDynamicProperty("App::PropertyStringList", propName)
        );
    }

    std::vector<std::string> values(lics.size());
    boost::range::transform(lics, values.begin(), [](const auto& it) { return it.toStdString(); });

    prop->setValues(values);
}

void DlgProjectLicence::accept()
{
    try {
        const QStringList lics = getLicencesFromDialog();
        setLicencesToDocument(lics);
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(this, tr("Cannot add licences"), QString::fromUtf8(e.what()));
    }
    QDialog::accept();
}

#include "moc_DlgProjectLicence.cpp"
