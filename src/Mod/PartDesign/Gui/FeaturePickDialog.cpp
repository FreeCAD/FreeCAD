/******************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
# include <QDialog>
# include <QListIterator>
#endif

#include <App/DocumentObject.h>
#include <Gui/Application.h>
#include <Gui/MainWindow.h>

#include "ui_FeaturePickDialog.h"
#include "FeaturePickDialog.h"

using namespace PartDesignGui;

const QString FeaturePickDialog::getFeatureStatusString(const featureStatus st)
{
    switch (st) {
        case validFeature: return tr("Valid");
        case invalidShape: return tr("Invalid shape");
        case noWire: return tr("No wire in sketch");
        case isUsed: return tr("Sketch already used by other feature");
        case otherBody: return tr("Sketch belongs to another Body feature");
        case basePlane: return tr("Base plane");
        case afterTip: return tr("Feature is located after the Tip feature");
    }

    return tr("");
}

FeaturePickDialog::FeaturePickDialog(std::vector<App::DocumentObject*>& objects,
                                     const std::vector<featureStatus>& status)
  : QDialog(Gui::getMainWindow()), ui(new Ui_FeaturePickDialog)
{
    ui->setupUi(this);

    connect(ui->checkReverse, SIGNAL(toggled(bool)), this, SLOT(onCheckReverse(bool)));
    connect(ui->checkOtherBody, SIGNAL(toggled(bool)), this, SLOT(onCheckOtherBody(bool)));
    connect(ui->checkOtherFeature, SIGNAL(toggled(bool)), this, SLOT(onCheckOtherFeature(bool)));
    connect(ui->radioIndependent, SIGNAL(toggled(bool)), this, SLOT(onUpdate(bool)));
    connect(ui->radioDependent, SIGNAL(toggled(bool)), this, SLOT(onUpdate(bool)));
    connect(ui->radioXRef, SIGNAL(toggled(bool)), this, SLOT(onUpdate(bool)));

    ui->checkReverse->setChecked(false);
    ui->checkOtherBody->setChecked(true);
    ui->checkOtherBody->setEnabled(false); // TODO: implement
    ui->checkOtherFeature->setChecked(false);
    ui->checkOtherFeature->setEnabled(false); // TODO: implement
    ui->radioIndependent->setChecked(true);
    ui->radioIndependent->setEnabled(false);
    // These are not implemented yet
    ui->radioDependent->setEnabled(false);
    ui->radioXRef->setEnabled(false);

    std::vector<featureStatus>::const_iterator st = status.begin();
    for (std::vector<App::DocumentObject*>::const_iterator o = objects.begin(); o != objects.end(); ++o) {
        QListWidgetItem* item = new QListWidgetItem(QString::fromLatin1((*o)->getNameInDocument()) +
                                                    QString::fromLatin1(" (") + getFeatureStatusString(*st) + QString::fromLatin1(")"));
        ui->listWidget->addItem(item);
        st++;
    }

    statuses = status;
    updateList();
}

FeaturePickDialog::~FeaturePickDialog()
{

}

void FeaturePickDialog::updateList()
{
    int index = 0;

    for (std::vector<featureStatus>::const_iterator st = statuses.begin(); st != statuses.end(); st++) {
        QListWidgetItem* item = ui->listWidget->item(index);

        switch (*st) {
            case validFeature: item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled); break;
            case invalidShape: item->setFlags(Qt::NoItemFlags); break;
            case noWire: item->setFlags(Qt::NoItemFlags); break;
            case isUsed: item->setFlags(ui->checkOtherFeature->isChecked() ? Qt::ItemIsSelectable | Qt::ItemIsEnabled : Qt::NoItemFlags); break;
            case otherBody: item->setFlags(ui->checkOtherBody->isChecked() ? Qt::ItemIsSelectable | Qt::ItemIsEnabled : Qt::NoItemFlags); break;
            case basePlane: item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled); break;
            case afterTip: item->setFlags(Qt::NoItemFlags); break;
        }

        index++;
    }
}

void FeaturePickDialog::onCheckReverse(bool checked)
{
}

void FeaturePickDialog::onCheckOtherFeature(bool checked)
{
    ui->radioIndependent->setEnabled(checked);
    // TODO: Not implemented yet
    //ui->radioDependent->setEnabled(checked);
    //ui->radioXRef->setEnabled(checked);

    updateList();
}

void FeaturePickDialog::onCheckOtherBody(bool checked)
{
    ui->radioIndependent->setEnabled(checked);
    // TODO: Not implemented yet
    //ui->radioDependent->setEnabled(checked);
    //ui->radioXRef->setEnabled(checked);

    updateList();
}

void FeaturePickDialog::onUpdate(bool)
{
    updateList();
}

bool FeaturePickDialog::getReverse()
{
    return ui->checkReverse->isChecked();
}

std::vector<App::DocumentObject*> FeaturePickDialog::getFeatures() {
    std::vector<App::DocumentObject*> result;

    for (std::vector<QString>::const_iterator s = features.begin(); s != features.end(); ++s)
        result.push_back(App::GetApplication().getActiveDocument()->getObject(s->toLatin1().data()));

    return result;
}



void FeaturePickDialog::accept()
{
    features.clear();
    QListIterator<QListWidgetItem*> i(ui->listWidget->selectedItems());
    while (i.hasNext()) {
        QString t = i.next()->text();
        t = t.left(t.indexOf(QString::fromLatin1("(")) - 1);
        features.push_back(t);
    }

    QDialog::accept();
}
#include "moc_FeaturePickDialog.cpp"
