/***************************************************************************
 *   Copyright (c) 2015 FreeCAD Developers                                 *
 *   Author: Przemo Firszt <przemo@firszt.eu>                              *
 *   Based on DlgToolbars.cpp file                                         *
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
# include <QDebug>
# include <QInputDialog>
#endif

#include "DlgWorkbenchesImp.h"
#include "ui_DlgWorkbenches.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "Command.h"
#include "MainWindow.h"
#include "Widgets.h"
#include "Workbench.h"
#include "WorkbenchManager.h"
#include "QListWidgetCustom.h"

using namespace Gui::Dialog;

const QString DlgWorkbenchesImp::all_workbenches = QString::fromLatin1("ALL");

/* TRANSLATOR Gui::Dialog::DlgWorkbenchesImp */

DlgWorkbenchesImp::DlgWorkbenchesImp(QWidget* parent)
    : CustomizeActionPage(parent)
    , ui(new Ui_DlgWorkbenches)
{
    ui->setupUi(this);
    set_lw_properties(ui->lw_enabled_workbenches);
    set_lw_properties(ui->lw_disabled_workbenches);
    ui->lw_disabled_workbenches->setProperty("OnlyAcceptFrom",
        QStringList() << ui->lw_enabled_workbenches->objectName());
    ui->lw_disabled_workbenches->setSortingEnabled(true);

    ui->lw_enabled_workbenches->setProperty("OnlyAcceptFrom",
        QStringList() << ui->lw_enabled_workbenches->objectName()
                      << ui->lw_disabled_workbenches->objectName());

    QStringList enabled_wbs_list = load_enabled_workbenches();
    QStringList disabled_wbs_list = load_disabled_workbenches();
    QStringList workbenches = Application::Instance->workbenches();

    for (QStringList::Iterator it = enabled_wbs_list.begin(); it != enabled_wbs_list.end(); ++it) {
        if (workbenches.contains(*it)) {
            add_workbench(ui->lw_enabled_workbenches, *it);
        } else {
            qDebug() << "Ignoring unknown" << *it << "workbench found in user preferences.";
        }
    }
    for (QStringList::Iterator it = workbenches.begin(); it != workbenches.end(); ++it) {
        if (disabled_wbs_list.contains(*it)){
            add_workbench(ui->lw_disabled_workbenches, *it);
        } else if (!enabled_wbs_list.contains(*it)){
            qDebug() << "Adding unknown " << *it << "workbench.";
            add_workbench(ui->lw_enabled_workbenches, *it);
        }
    }
    ui->lw_enabled_workbenches->setCurrentRow(0);
    ui->lw_disabled_workbenches->setCurrentRow(0);
}

/** Destroys the object and frees any allocated resources */
DlgWorkbenchesImp::~DlgWorkbenchesImp()
{
}

void DlgWorkbenchesImp::set_lw_properties(QListWidgetCustom *lw)
{
    lw->setDragDropMode(QAbstractItemView::DragDrop);
    lw->setSelectionMode(QAbstractItemView::SingleSelection);
    lw->viewport()->setAcceptDrops(true);
    lw->setDropIndicatorShown(true);
    lw->setDragEnabled(true);
    lw->setDefaultDropAction(Qt::MoveAction);
}

void DlgWorkbenchesImp::add_workbench(QListWidgetCustom *lw, const QString& it)
{
    QPixmap px = Application::Instance->workbenchIcon(it);
    QString mt = Application::Instance->workbenchMenuText(it);
    QListWidgetItem *wi = (new QListWidgetItem(QIcon(px), mt));
    wi->setData(Qt::UserRole, QVariant(it));
    lw->addItem(wi);
}

void DlgWorkbenchesImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

void DlgWorkbenchesImp::hideEvent(QHideEvent * event)
{
    Q_UNUSED(event); 
    save_workbenches();
}

void DlgWorkbenchesImp::onAddMacroAction(const QByteArray& macro)
{
    Q_UNUSED(macro); 
}

void DlgWorkbenchesImp::onRemoveMacroAction(const QByteArray& macro)
{
    Q_UNUSED(macro); 
}

void DlgWorkbenchesImp::onModifyMacroAction(const QByteArray& macro)
{
    Q_UNUSED(macro); 
}

void DlgWorkbenchesImp::move_workbench(QListWidgetCustom *lwc_dest,
                                       QListWidgetItem *wi)
{
    QListWidgetItem* item = wi->clone();
    lwc_dest->addItem(item);
    lwc_dest->setCurrentItem(item);
    delete wi;
}

void DlgWorkbenchesImp::on_add_to_enabled_workbenches_btn_clicked()
{
    QListWidgetItem* ci = ui->lw_disabled_workbenches->currentItem();
    if (ci) {
        move_workbench(ui->lw_enabled_workbenches, ci);
    }
}

void DlgWorkbenchesImp::on_remove_from_enabled_workbenches_btn_clicked()
{
    QListWidgetItem* ci = ui->lw_enabled_workbenches->currentItem();
    if (ci) {
        move_workbench(ui->lw_disabled_workbenches, ci);
    }
}

void DlgWorkbenchesImp::shift_workbench(bool up)
{
    int direction;
    if (up){
        direction = -1;
    } else {
        direction = 1;
    }
    if (ui->lw_enabled_workbenches->currentItem()) {
        int index = ui->lw_enabled_workbenches->currentRow();
        QListWidgetItem *item = ui->lw_enabled_workbenches->takeItem(index);
        ui->lw_enabled_workbenches->insertItem(index + direction, item);
        ui->lw_enabled_workbenches->setCurrentRow(index + direction);
    }
}

void DlgWorkbenchesImp::on_shift_workbench_up_btn_clicked()
{
    shift_workbench(true);
}

void DlgWorkbenchesImp::on_shift_workbench_down_btn_clicked()
{
    shift_workbench(false);
}

void DlgWorkbenchesImp::on_sort_enabled_workbenches_btn_clicked()
{
    ui->lw_enabled_workbenches->sortItems();
}

void DlgWorkbenchesImp::on_add_all_to_enabled_workbenches_btn_clicked()
{
    while (ui->lw_disabled_workbenches->count() > 0) {
        QListWidgetItem* item = ui->lw_disabled_workbenches->item(0);
        move_workbench(ui->lw_enabled_workbenches, item);
    }
}

QStringList DlgWorkbenchesImp::load_enabled_workbenches()
{
    QString enabled_wbs;
    QStringList enabled_wbs_list;
    ParameterGrp::handle hGrp;

    hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Workbenches");
    enabled_wbs = QString::fromStdString(hGrp->GetASCII("Enabled", all_workbenches.toStdString().c_str()).c_str());
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
    enabled_wbs_list = enabled_wbs.split(QLatin1String(","), Qt::SkipEmptyParts);
#else
    enabled_wbs_list = enabled_wbs.split(QLatin1String(","), QString::SkipEmptyParts);
#endif

    if (enabled_wbs_list.at(0) == all_workbenches) {
        enabled_wbs_list.removeFirst();
        QStringList workbenches = Application::Instance->workbenches();
        for (QStringList::Iterator it = workbenches.begin(); it != workbenches.end(); ++it) {
            enabled_wbs_list.append(*it);
        }
        enabled_wbs_list.sort();
    }
    return enabled_wbs_list;
}

QStringList DlgWorkbenchesImp::load_disabled_workbenches()
{
    QString disabled_wbs;
    QStringList disabled_wbs_list;
    ParameterGrp::handle hGrp;

    hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Workbenches");
    disabled_wbs = QString::fromStdString(hGrp->GetASCII("Disabled", ""));
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
    disabled_wbs_list = disabled_wbs.split(QLatin1String(","), Qt::SkipEmptyParts);
#else
    disabled_wbs_list = disabled_wbs.split(QLatin1String(","), QString::SkipEmptyParts);
#endif

    return disabled_wbs_list;
}

void DlgWorkbenchesImp::save_workbenches()
{
    QString enabled_wbs;
    QString disabled_wbs;
    ParameterGrp::handle hGrp;

    hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Workbenches");
    hGrp->Clear();

    if (ui->lw_enabled_workbenches->count() == 0) {
        enabled_wbs.append(QString::fromLatin1("NoneWorkbench"));
    } else {
        for (int i = 0; i < ui->lw_enabled_workbenches->count(); i++) {
            QVariant item_data = ui->lw_enabled_workbenches->item(i)->data(Qt::UserRole);
            QString name = item_data.toString();
            enabled_wbs.append(name + QString::fromLatin1(","));
        }
    }
    hGrp->SetASCII("Enabled", enabled_wbs.toLatin1());

    for (int i = 0; i < ui->lw_disabled_workbenches->count(); i++) {
        QVariant item_data = ui->lw_disabled_workbenches->item(i)->data(Qt::UserRole);
        QString name = item_data.toString();
        disabled_wbs.append(name + QString::fromLatin1(","));
    }
    hGrp->SetASCII("Disabled", disabled_wbs.toLatin1());
}

#include "moc_DlgWorkbenchesImp.cpp"

