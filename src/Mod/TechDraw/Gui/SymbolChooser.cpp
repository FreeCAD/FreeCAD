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
# include <QListWidget>
#endif

#include <App/Application.h>

#include "SymbolChooser.h"
#include "ui_SymbolChooser.h"


using namespace Gui;
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
    connect(ui->fcSymbolDir, &FileChooser::fileNameChanged,
            this, &SymbolChooser::onDirectoryChanged);
    connect(ui->lwSymbols, &QListWidget::itemClicked,    //double click?
            this, &SymbolChooser::onItemClicked);

    setUiPrimary();
}

SymbolChooser::~SymbolChooser()
{
}

void SymbolChooser::setUiPrimary()
{
    // Base::Console().Message("SC::setUiPrimary()\n");
    setWindowTitle(QObject::tr("Select a symbol"));
    resize(QSize(700, 500));
    if (!m_symbolDir.isEmpty()) {
        ui->fcSymbolDir->setFileName(m_symbolDir);
        loadSymbolNames(m_symbolDir);
    } else {
        std::string resourceDir = App::Application::getResourceDir();
        std::string defPath = "Mod/TechDraw/Symbols/Welding/AWS/";
        resourceDir = resourceDir + defPath;
        QString m_symbolDir = QString::fromUtf8(resourceDir.c_str());
        ui->fcSymbolDir->setFileName(m_symbolDir);
        loadSymbolNames(m_symbolDir);
    }

    ui->lwSymbols->setViewMode(QListView::IconMode);
    ui->lwSymbols->setFlow(QListView::LeftToRight);
    ui->lwSymbols->setWrapping(true);
    ui->lwSymbols->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->lwSymbols->setGridSize(QSize(75, 85));
    ui->lwSymbols->setIconSize(QSize(45, 45));
    ui->lwSymbols->setResizeMode(QListView::Adjust);
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
    // Base::Console().Message("SCS::onItemClicked(%s)\n", qPrintable(item->text()));
    // Are item and currentItem() the same? Should use item?
    QListWidgetItem* sourceItem = ui->lwSymbols->currentItem();
    QString targetText = sourceItem->text();
    m_symbolPath = m_symbolDir +
                   targetText +
                   QString::fromUtf8(".svg");
    Q_EMIT symbolSelected(m_symbolPath, m_source);

    // Base::Console().Message("SC::onOKClicked - symbol: %s\n", qPrintable(m_symbolPath));
    accept();
}

void SymbolChooser::onDirectoryChanged(const QString& newDir)
{
    ui->lwSymbols->clear(); // Remove all previous symbols
    // Base::Console().Message("SC::onDirectoryChanged(%s)\n", qPrintable(newDir));
    m_symbolDir = newDir + QString::fromUtf8("/");
    loadSymbolNames(m_symbolDir);
}

void SymbolChooser::loadSymbolNames(QString pathToSymbols)
{
    // Fill selection list with names and icons
    QDir symbolDir(pathToSymbols);
    symbolDir.setFilter(QDir::Files);
    QStringList fileNames = symbolDir.entryList();

    for (auto& fn: fileNames) {
        QString text = (new QFileInfo(fn))->baseName();
        QIcon icon(pathToSymbols + fn);

        // Create a symbol in the QListWidget lwSymbols
        new QListWidgetItem(icon, text, ui->lwSymbols);
    }
}

#include <Mod/TechDraw/Gui/moc_SymbolChooser.cpp>
