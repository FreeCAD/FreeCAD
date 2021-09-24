/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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
#endif

#include <App/Application.h>
#include <Gui/FileDialog.h>

#include "DrawGuiStd.h"
#include "Rez.h"

#include <Mod/TechDraw/Gui/ui_SymbolChooser.h>

#include "SymbolChooser.h"

using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;


SymbolChooser::SymbolChooser(QWidget *parent,
                             QString startDir,
                             QString source) :
    QDialog(parent),
    ui(new Ui_SymbolChooser),
    m_symbolDir(startDir),
    m_source(source)
{
    ui->setupUi(this);
    connect(ui->fcSymbolDir, SIGNAL(fileNameSelected(const QString&)),
            this, SLOT(onDirectorySelected(const QString&)));
    connect(ui->lwSymbols, SIGNAL(itemClicked(QListWidgetItem*)),    //double click?
            this, SLOT(onItemClicked(QListWidgetItem*)));

    setUiPrimary();
}

void SymbolChooser::setUiPrimary()
{
//    Base::Console().Message("SC::setUiPrimary()\n");
    setWindowTitle(QObject::tr("Select a symbol"));
    if (!m_symbolDir.isEmpty()) {
        ui->fcSymbolDir->setFileName(m_symbolDir);
        loadSymbolNames(m_symbolDir);
    } else {
        std::string resourceDir = App::Application::getResourceDir();
        std::string defPath = "Mod/TechDraw/Symbols/Welding/AWS/";
        resourceDir = resourceDir + defPath;
        QString defDir = QString::fromUtf8(resourceDir.c_str());
        ui->fcSymbolDir->setFileName(defDir);
        loadSymbolNames(defDir);
        m_symbolDir = defDir;
    }

    ui->lwSymbols->setViewMode(QListView::IconMode);
    ui->lwSymbols->setFlow(QListView::LeftToRight);
    ui->lwSymbols->setWrapping(true);
    ui->lwSymbols->setDragEnabled(true);
    ui->lwSymbols->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->lwSymbols->setAcceptDrops(false);
}

void SymbolChooser::onOKClicked()
{
    QDialog::accept();
    QListWidgetItem* sourceItem = ui->lwSymbols->currentItem();
    if (!sourceItem)
        return;
    QString targetText = sourceItem->text();
    m_symbolPath = m_symbolDir +
                   targetText +
                   QString::fromUtf8(".svg");

    Q_EMIT symbolSelected(m_symbolPath, m_source);
}

void SymbolChooser::onCancelClicked()
{
    QDialog::reject();
}

void SymbolChooser::onItemClicked(QListWidgetItem* item)
{
    Q_UNUSED(item);
//    Base::Console().Message("SCS::onItemClicked(%s)\n", qPrintable(item->text()));
    //are item and currentItem() the same?  should use item?
    QListWidgetItem* sourceItem = ui->lwSymbols->currentItem();
    QString targetText = sourceItem->text();
    m_symbolPath = m_symbolDir +
                   targetText +
                   QString::fromUtf8(".svg");
    Q_EMIT symbolSelected(m_symbolPath, m_source);

//    Base::Console().Message("SC::onOKClicked - symbol: %s\n", qPrintable(m_symbolPath));
    accept();
}

void SymbolChooser::onDirectorySelected(const QString& newDir)
{
//    Base::Console().Message("SC::onDirectorySelected(%s)\n", qPrintable(newDir));
    m_symbolDir = newDir + QString::fromUtf8("/");
    loadSymbolNames(m_symbolDir);
}

void SymbolChooser::loadSymbolNames(QString pathToSymbols)
{
    //fill selection list with names and icons
    QDir symbolDir(pathToSymbols);
    symbolDir.setFilter(QDir::Files);
    QStringList fileNames = symbolDir.entryList();

    for (auto& fn: fileNames) {
        QListWidgetItem* item = new QListWidgetItem(fn, ui->lwSymbols);
        QFileInfo fi(fn);
        item->setText(fi.baseName());
        QIcon symbolIcon(pathToSymbols + fn);
        item->setIcon(symbolIcon);
        ui->lwSymbols->addItem(item);
    }
    ui->lwSymbols->setCurrentRow(0);
    ui->lwSymbols->setAcceptDrops(false);       //have to do this every time you update the items
}

//QString SymbolChooser::getSymbolPath(void)
//{
//    Base::Console().Message("SC::getSymbolPath returns: %s\n", qPrintable(m_symbolPath));
//    return m_symbolPath;
//}

#include <Mod/TechDraw/Gui/moc_SymbolChooser.cpp>
