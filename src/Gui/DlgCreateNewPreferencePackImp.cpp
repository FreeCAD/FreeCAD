/***************************************************************************
 *   Copyright (c) 2021 Chris Hennes <chennes@pioneerlibrarysystem.org>    *
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
# include <QMessageBox>
# include <QPushButton>
# include <QRegularExpression>
# include <QRegularExpressionMatch>
#endif

#include "DlgCreateNewPreferencePackImp.h"
#include "ui_DlgCreateNewPreferencePack.h"


using namespace Gui::Dialog;

const auto TemplateRole = Qt::UserRole;

/* TRANSLATOR Gui::Dialog::DlgCreateNewPreferencePackImp */

/**
 *  Constructs a Gui::Dialog::DlgCreateNewPreferencePackImp as a child of 'parent'
 */
DlgCreateNewPreferencePackImp::DlgCreateNewPreferencePackImp(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui_DlgCreateNewPreferencePack)
{
    ui->setupUi(this);

    QRegularExpression validNames(QString::fromUtf8(R"([^/\\?%*:|"<>]+)"));
    _nameValidator.setRegularExpression(validNames);
    ui->lineEdit->setValidator(&_nameValidator);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    connect(ui->treeWidget, &QTreeWidget::itemChanged, this, &DlgCreateNewPreferencePackImp::onItemChanged);
    connect(ui->lineEdit, &QLineEdit::textEdited, this, &DlgCreateNewPreferencePackImp::onLineEditTextEdited);
}


DlgCreateNewPreferencePackImp::~DlgCreateNewPreferencePackImp() = default;

void DlgCreateNewPreferencePackImp::setPreferencePackTemplates(const std::vector<Gui::PreferencePackManager::TemplateFile>& availableTemplates)
{
    ui->treeWidget->clear();
    _groups.clear();

    ui->treeWidget->header()->setDefaultSectionSize(250);

    _templates = availableTemplates;
    for (const auto &t : _templates) {

        QTreeWidgetItem* group;
        if (auto foundGroup = _groups.find(t.group); foundGroup != _groups.end()) {
            group = foundGroup->second;
        }
        else {
            group = new QTreeWidgetItem(ui->treeWidget, QStringList(QString::fromStdString(t.group)));
            group->setCheckState(0, Qt::Checked);
            group->setExpanded(true);
            _groups.insert(std::make_pair(t.group, group));
        }

        QStringList itemColumns;
        itemColumns.push_back(QString::fromStdString(t.name));
        auto newItem = new QTreeWidgetItem(group, itemColumns);
        newItem->setCheckState(0, Qt::Checked);
        if (group->checkState(0) != newItem->checkState(0))
            group->setCheckState(0, Qt::PartiallyChecked);
        newItem->setData(0, TemplateRole, QVariant::fromValue(t));
        group->addChild(newItem);
    }
}

void Gui::Dialog::DlgCreateNewPreferencePackImp::setPreferencePackNames(const std::vector<std::string>& usedNames)
{
    _existingPackNames = usedNames;
}

std::vector<Gui::PreferencePackManager::TemplateFile> DlgCreateNewPreferencePackImp::selectedTemplates() const
{
    std::vector<Gui::PreferencePackManager::TemplateFile> results;

    for (const auto& group : _groups)
        for (int childIndex = 0; childIndex < group.second->childCount(); ++childIndex)
            if (auto child = group.second->child(childIndex); child->checkState(0) == Qt::Checked)
                if (child->data(0, TemplateRole).canConvert<Gui::PreferencePackManager::TemplateFile>())
                    results.push_back(child->data(0, TemplateRole).value<Gui::PreferencePackManager::TemplateFile>());

    return results;
}

std::string DlgCreateNewPreferencePackImp::preferencePackName() const
{
    return ui->lineEdit->text().toStdString();
}

void DlgCreateNewPreferencePackImp::onItemChanged(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column);
    const QSignalBlocker blocker(ui->treeWidget);
    if (auto group = item->parent(); group) {
        // Child clicked
        bool firstItemChecked = false;
        for (int childIndex = 0; childIndex < group->childCount(); ++childIndex) {
            auto child = group->child(childIndex);
            if (childIndex == 0) {
                firstItemChecked = child->checkState(0) == Qt::Checked;
            }
            else {
                bool thisItemChecked = child->checkState(0) == Qt::Checked;
                if (firstItemChecked != thisItemChecked) {
                    group->setCheckState(0, Qt::PartiallyChecked);
                    return;
                }
            }
        }
        group->setCheckState(0, firstItemChecked ? Qt::Checked : Qt::Unchecked);
    }
    else {
        // Group clicked:
        auto groupCheckState = item->checkState(0);
        for (int childIndex = 0; childIndex < item->childCount(); ++childIndex) {
            auto child = item->child(childIndex);
            child->setCheckState(0, groupCheckState);
        }
    }
}

void DlgCreateNewPreferencePackImp::onLineEditTextEdited(const QString& text)
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(text.isEmpty());
}

void Gui::Dialog::DlgCreateNewPreferencePackImp::accept()
{
    // Ensure that the chosen name is either unique, or that the user actually wants to overwrite the old one
    if (auto chosenName = ui->lineEdit->text().toStdString();
        std::find(_existingPackNames.begin(), _existingPackNames.end(), chosenName) != _existingPackNames.end()) {
        auto result = QMessageBox::warning(this, tr("Pack already exists"),
                                           tr("A preference pack with that name already exists. Do you want to overwrite it?"),
                                           QMessageBox::Yes | QMessageBox::Cancel);
        if (result == QMessageBox::Cancel)
            return;
    }
    QDialog::accept();
}


#include "moc_DlgCreateNewPreferencePackImp.cpp"
