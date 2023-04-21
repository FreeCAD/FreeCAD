/***************************************************************************
 *   Copyright (c) 2019 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#endif

#include "DlgParameterFind.h"
#include "ui_DlgParameterFind.h"
#include "DlgParameterImp.h"


using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgParameterFind */

DlgParameterFind::DlgParameterFind(DlgParameterImp* parent)
  : QDialog(parent)
  , ui(new Ui_DlgParameterFind)
  , dialog(parent)
{
    ui->setupUi(this);
    setupConnections();

    QPushButton* btn = ui->buttonBox->button(QDialogButtonBox::Ok);
    if (btn) {
        btn->setText(tr("Find Next"));
        btn->setDisabled(true);
    }
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgParameterFind::~DlgParameterFind()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}

void DlgParameterFind::setupConnections()
{
    connect(ui->lineEdit, &QLineEdit::textChanged,
            this, &DlgParameterFind::onLineEditTextChanged);
    connect(ui->checkGroups, &QCheckBox::toggled,
            this, &DlgParameterFind::onCheckGroupsToggled);
    connect(ui->checkNames, &QCheckBox::toggled,
            this, &DlgParameterFind::onCheckNamesToggled);
    connect(ui->checkValues, &QCheckBox::toggled,
            this, &DlgParameterFind::onCheckValuesToggled);
}

void DlgParameterFind::onLineEditTextChanged(const QString& text)
{
    QPushButton* btn = ui->buttonBox->button(QDialogButtonBox::Ok);
    if (btn) {
        bool ok = ui->checkGroups->isChecked() ||
                  ui->checkNames->isChecked() ||
                  ui->checkValues->isChecked();
        btn->setDisabled(!ok || text.isEmpty());
    }
}

void DlgParameterFind::onCheckGroupsToggled(bool)
{
    QPushButton* btn = ui->buttonBox->button(QDialogButtonBox::Ok);
    if (btn) {
        bool ok = ui->checkGroups->isChecked() ||
                  ui->checkNames->isChecked() ||
                  ui->checkValues->isChecked();
        QString text = ui->lineEdit->text();
        btn->setDisabled(!ok || text.isEmpty());
    }
}

void DlgParameterFind::onCheckNamesToggled(bool)
{
    QPushButton* btn = ui->buttonBox->button(QDialogButtonBox::Ok);
    if (btn) {
        bool ok = ui->checkGroups->isChecked() ||
                  ui->checkNames->isChecked() ||
                  ui->checkValues->isChecked();
        QString text = ui->lineEdit->text();
        btn->setDisabled(!ok || text.isEmpty());
    }
}

void DlgParameterFind::onCheckValuesToggled(bool)
{
    QPushButton* btn = ui->buttonBox->button(QDialogButtonBox::Ok);
    if (btn) {
        bool ok = ui->checkGroups->isChecked() ||
                  ui->checkNames->isChecked() ||
                  ui->checkValues->isChecked();
        QString text = ui->lineEdit->text();
        btn->setDisabled(!ok || text.isEmpty());
    }
}

bool DlgParameterFind::matches(QTreeWidgetItem* item, const Options& opt) const
{
    // check the group name
    if (opt.group) {
        // whole word matches
        if (opt.match) {
            if (item->text(0).compare(opt.text, Qt::CaseInsensitive) == 0)
                return true;
        }
        else {
            if (item->text(0).indexOf(opt.text, 0, Qt::CaseInsensitive) >= 0)
                return true;
        }
    }

    if (opt.name || opt.value) {
        auto group = static_cast<ParameterGroupItem*>(item);
        Base::Reference<ParameterGrp> hGrp = group->_hcGrp;

        auto boolMap = hGrp->GetBoolMap();
        auto intMap = hGrp->GetIntMap();
        auto uintMap = hGrp->GetUnsignedMap();
        auto floatMap = hGrp->GetFloatMap();
        auto asciiMap = hGrp->GetASCIIMap();

        // check the name of an entry in the group
        if (opt.name) {
            if (opt.match) {
                for (const auto& it : boolMap) {
                    QString text = QString::fromUtf8(it.first.c_str());
                    if (text.compare(opt.text, Qt::CaseInsensitive) == 0)
                        return true;
                }
                for (const auto& it : intMap) {
                    QString text = QString::fromUtf8(it.first.c_str());
                    if (text.compare(opt.text, Qt::CaseInsensitive) == 0)
                        return true;
                }
                for (const auto& it : uintMap) {
                    QString text = QString::fromUtf8(it.first.c_str());
                    if (text.compare(opt.text, Qt::CaseInsensitive) == 0)
                        return true;
                }
                for (const auto& it : floatMap) {
                    QString text = QString::fromUtf8(it.first.c_str());
                    if (text.compare(opt.text, Qt::CaseInsensitive) == 0)
                        return true;
                }
                for (const auto& it : asciiMap) {
                    QString text = QString::fromUtf8(it.first.c_str());
                    if (text.compare(opt.text, Qt::CaseInsensitive) == 0)
                        return true;
                }
            }
            else {
                for (const auto& it : boolMap) {
                    QString text = QString::fromUtf8(it.first.c_str());
                    if (text.indexOf(opt.text, 0, Qt::CaseInsensitive) >= 0)
                        return true;
                }
                for (const auto& it : intMap) {
                    QString text = QString::fromUtf8(it.first.c_str());
                    if (text.indexOf(opt.text, 0, Qt::CaseInsensitive) >= 0)
                        return true;
                }
                for (const auto& it : uintMap) {
                    QString text = QString::fromUtf8(it.first.c_str());
                    if (text.indexOf(opt.text, 0, Qt::CaseInsensitive) >= 0)
                        return true;
                }
                for (const auto& it : floatMap) {
                    QString text = QString::fromUtf8(it.first.c_str());
                    if (text.indexOf(opt.text, 0, Qt::CaseInsensitive) >= 0)
                        return true;
                }
                for (const auto& it : asciiMap) {
                    QString text = QString::fromUtf8(it.first.c_str());
                    if (text.indexOf(opt.text, 0, Qt::CaseInsensitive) >= 0)
                        return true;
                }
            }
        }

        // check the value of an entry in the group
        if (opt.value) {
            if (opt.match) {
                for (const auto& it : asciiMap) {
                    QString text = QString::fromUtf8(it.second.c_str());
                    if (text.compare(opt.text, Qt::CaseInsensitive) == 0)
                        return true;
                }
            }
            else {
                for (const auto& it : asciiMap) {
                    QString text = QString::fromUtf8(it.second.c_str());
                    if (text.indexOf(opt.text, 0, Qt::CaseInsensitive) >= 0)
                        return true;
                }
            }
        }
    }

    return false;
}

QTreeWidgetItem* DlgParameterFind::findItem(QTreeWidgetItem* root, const Options& opt) const
{
    if (!root)
        return nullptr;

    if (matches(root, opt)) {
        // if the root matches then only return if it's not the current element
        // as otherwise it would never move forward
        if (root->treeWidget()->currentItem() != root)
            return root;
    }

    for (int i=0; i<root->childCount(); i++) {
        QTreeWidgetItem* item = root->child(i);
        if (matches(item, opt))
            return item;
        item = findItem(item, opt);
        if (item)
            return item;
    }

    return nullptr;
}

void DlgParameterFind::accept()
{
    auto groupTree = dialog->findChild<ParameterGroup*>();
    if (groupTree) {
        Options opt;
        opt.text = ui->lineEdit->text();
        opt.group = ui->checkGroups->isChecked();
        opt.name = ui->checkNames->isChecked();
        opt.value = ui->checkValues->isChecked();
        opt.match = ui->checkMatch->isChecked();

        QTreeWidgetItem* current = groupTree->currentItem();
        QTreeWidgetItem* next = findItem(current, opt);
        while (!next && current) {
            // go to the parent item and try again for each sibling after the current item
            QTreeWidgetItem* parent = current->parent();
            if (!parent) {
                // switch from one top-level group to the next
                QTreeWidgetItem* root = groupTree->invisibleRootItem();
                if (root->indexOfChild(current) >= 0) {
                    parent = root;
                }
            }
            if (parent) {
                int index = parent->indexOfChild(current);
                for (int i=index+1; i<parent->childCount(); i++) {
                    next = findItem(parent->child(i), opt);
                    if (next)
                        break;
                }
            }

            if (!next) {
                current = parent;
            }
        }

        // if search was successful then make it the current item
        if (next)
            groupTree->setCurrentItem(next);
        else
            QMessageBox::warning(this, tr("Not found"), tr("Can't find the text: %1").arg(opt.text));
    }
}

void DlgParameterFind::reject()
{
    QDialog::reject();
}

#include "moc_DlgParameterFind.cpp"
