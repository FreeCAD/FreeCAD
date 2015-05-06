/******************************************************************************
 *   Copyright (c)2012 Jan Rheinlaender <jrheinlaender@users.sourceforge.net> *
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
# include <QPixmap>
# include <QDialog>
# include <QListIterator>
#endif

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/MainWindow.h>
#include <App/Document.h>
#include <Base/Tools.h>

#include "ui_TaskFeaturePick.h"
#include "TaskFeaturePick.h"

using namespace PartDesignGui;

const QString TaskFeaturePick::getFeatureStatusString(const featureStatus st)
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

TaskFeaturePick::TaskFeaturePick(std::vector<App::DocumentObject*>& objects,
                                     const std::vector<featureStatus>& status,
                                     QWidget* parent)
  : TaskBox(Gui::BitmapFactory().pixmap("edit-select-box"),
            QString::fromAscii("Select feature"), true, parent), ui(new Ui_TaskFeaturePick)
{
    
    proxy = new QWidget(this);
    ui->setupUi(proxy);

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
    for (std::vector<App::DocumentObject*>::const_iterator o = objects.begin(); o != objects.end(); o++) {
        QListWidgetItem* item = new QListWidgetItem(QString::fromAscii((*o)->getNameInDocument()) +
                                                    QString::fromAscii(" (") + getFeatureStatusString(*st) + QString::fromAscii(")"));
        ui->listWidget->addItem(item);
        st++;
    }

    groupLayout()->addWidget(proxy);
    statuses = status;
    updateList();
}

TaskFeaturePick::~TaskFeaturePick()
{

}

void TaskFeaturePick::updateList()
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

void TaskFeaturePick::onCheckReverse(bool checked)
{
}

void TaskFeaturePick::onCheckOtherFeature(bool checked)
{
    ui->radioIndependent->setEnabled(checked);
    // TODO: Not implemented yet
    //ui->radioDependent->setEnabled(checked);
    //ui->radioXRef->setEnabled(checked);

    updateList();
}

void TaskFeaturePick::onCheckOtherBody(bool checked)
{
    ui->radioIndependent->setEnabled(checked);
    // TODO: Not implemented yet
    //ui->radioDependent->setEnabled(checked);
    //ui->radioXRef->setEnabled(checked);

    updateList();
}

void TaskFeaturePick::onUpdate(bool)
{
    updateList();
}

bool TaskFeaturePick::getReverse()
{
    return ui->checkReverse->isChecked();
}

std::vector<App::DocumentObject*> TaskFeaturePick::getFeatures() {
    
    features.clear();
    QListIterator<QListWidgetItem*> i(ui->listWidget->selectedItems());
    while (i.hasNext()) {
        QString t = i.next()->text();
        t = t.left(t.indexOf(QString::fromAscii("(")) - 1);
        features.push_back(t);
    }
    
    std::vector<App::DocumentObject*> result;

    for (std::vector<QString>::const_iterator s = features.begin(); s != features.end(); s++)
        result.push_back(App::GetApplication().getActiveDocument()->getObject(s->toAscii().data()));

    return result;
}

void TaskFeaturePick::onSelectionChanged(const Gui::SelectionChanges& msg)
{

}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgFeaturePick::TaskDlgFeaturePick(std::vector<App::DocumentObject*> &objects, 
                                        const std::vector<TaskFeaturePick::featureStatus> &status,
                                        boost::function<bool (std::vector<App::DocumentObject*>)> afunc,
                                        boost::function<void (std::vector<App::DocumentObject*>)> wfunc)
    : TaskDialog()      
{
    pick  = new TaskFeaturePick(objects, status);
    Content.push_back(pick);
    
    acceptFunction = afunc;
    workFunction = wfunc;
}

TaskDlgFeaturePick::~TaskDlgFeaturePick()
{
    //do the work now as before in accept() the dialog is still open, hence the work 
    //function could not open annother dialog
    if(accepted)
        workFunction(pick->getFeatures());
}

//==== calls from the TaskView ===============================================================


void TaskDlgFeaturePick::open()
{
    
}

void TaskDlgFeaturePick::clicked(int)
{
    
}

bool TaskDlgFeaturePick::accept()
{
    accepted = acceptFunction(pick->getFeatures());
     
    return accepted;
}

bool TaskDlgFeaturePick::reject()
{
    accepted = false;
    return true;
}

#include "moc_TaskFeaturePick.cpp"
