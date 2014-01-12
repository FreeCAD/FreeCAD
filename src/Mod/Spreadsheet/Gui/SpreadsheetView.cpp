/***************************************************************************
 *                                                                         *
 *   Copyright (c) 2013 Eivind Kvedalen (eivind@kvedalen.name)             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *   for detail see the LICENCE text file.                                 *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QAction>
# include <QApplication>
# include <QMenu>
# include <QMouseEvent>
# include <QSlider>
# include <QStatusBar>
# include <QToolBar>
# include <QTableWidgetItem>
# include <QMessageBox>
# include <cmath>
#endif

#include "SpreadsheetView.h"
#include "SpreadsheetDelegate.h"
#include <Mod/Spreadsheet/App/Expression.h>
#include <Mod/Spreadsheet/App/Sheet.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <App/DocumentObject.h>
#include <App/PropertyStandard.h>
#include <Gui/Command.h>
#include <boost/bind.hpp>

#include "ui_Sheet.h"

using namespace SpreadsheetGui;
using namespace Gui;

/* TRANSLATOR SpreadsheetGui::SheetView */

SheetView::SheetView(App::DocumentObject *docObj, QWidget *parent)
    : MDIView(0, parent)
    , sheet(static_cast<Spreadsheet::Sheet*>(docObj))
    , model(static_cast<Spreadsheet::Sheet*>(docObj))
{
    // Set up ui
    ui = new Ui::Sheet();
    QWidget * w = new QWidget(this);
    ui->setupUi(w);
    setCentralWidget(w);

    delegate = new SpreadsheetDelegate();
    ui->cells->setModel(&model);
    ui->cells->setItemDelegate(delegate);
    ui->cells->setSheet(sheet);

    // Connect signals
    connect(ui->cells->selectionModel(), SIGNAL( currentChanged( QModelIndex, QModelIndex ) ),
            this,        SLOT( currentChanged( QModelIndex, QModelIndex ) ) );

    connect(ui->cells->horizontalHeader(), SIGNAL(sectionResized ( int, int, int ) ),
            this, SLOT(columnResized(int, int, int)));

    connect(ui->cells->verticalHeader(), SIGNAL(sectionResized ( int, int, int ) ),
            this, SLOT(rowResized(int, int, int)));

    connect(ui->cellContent, SIGNAL(returnPressed()), this, SLOT( editingFinished() ));

    columnWidthChangedConnection = sheet->columnWidthChanged.connect(bind(&SheetView::resizeColumn, this, _1, _2));
    rowHeightChangedConnection = sheet->rowHeightChanged.connect(bind(&SheetView::resizeRow, this, _1, _2));
}

SheetView::~SheetView()
{
    delete delegate;
}

void SheetView::setCurrentCell(QString str)
{
    updateContentLine();
}

void SheetView::updateContentLine()
{
    QModelIndex i = ui->cells->currentIndex();

    if (i.isValid()) {
        std::string str;

        sheet->getCell(i.row(), i.column())->getStringContent(str);
        ui->cellContent->setText( QString::fromUtf8(str.c_str()) );
        ui->cellContent->setEnabled( true );
    }
}

void SheetView::columnResized(int col, int oldSize, int newSize)
{
    sheet->setColumnWidth(col, newSize);
}

void SheetView::rowResized(int col, int oldSize, int newSize)
{
    sheet->setRowHeight(col, newSize);
}

void SheetView::resizeColumn(int col, int newSize)
{
    if (ui->cells->horizontalHeader()->sectionSize(col) != newSize)
        ui->cells->setColumnWidth(col, newSize);
}

void SheetView::resizeRow(int col, int newSize)
{
    if (ui->cells->verticalHeader()->sectionSize(col) != newSize)
        ui->cells->setRowHeight(col, newSize);
}

void SheetView::editingFinished()
{
    QModelIndex i = ui->cells->currentIndex();

    // Update data in cell
    ui->cells->model()->setData(i, QVariant(ui->cellContent->text()), Qt::EditRole);

    // Focus on cell below
    i = ui->cells->model()->index(i.row() + 1, i.column());
    ui->cells->setCurrentIndex( i );
}

void SheetView::currentChanged ( const QModelIndex & current, const QModelIndex & previous  )
{
    updateContentLine();
}

void SheetView::updateCell(const App::Property *prop)
{
    int row, col;

    try {
        sheet->getCellAddress(prop, row, col);
        sheet->cellUpdated(row, col);
        updateContentLine();
    }
    catch (...) {
        // Property is not a cell
        return;
    }
}

std::vector<Spreadsheet::Sheet::Range> SheetView::selectedRanges() const
{
    return ui->cells->selectedRanges();
}

QModelIndexList SheetView::selectedIndexes() const
{
    return ui->cells->selectionModel()->selectedIndexes();
}

QModelIndex SheetView::currentIndex() const
{
    return ui->cells->currentIndex();
}

void SheetView::keyPressEvent ( QKeyEvent * event )
{
    if (event->key() == Qt::Key_Delete) {
        if (event->modifiers() == 0) {
            //model()->setData(currentIndex(), QVariant(), Qt::EditRole);
        }
        else if (event->modifiers() == Qt::ControlModifier) {
            //model()->setData(currentIndex(), QVariant(), Qt::EditRole);
        }
    }
    else
       Gui::MDIView::keyPressEvent(event);
}

#include "moc_SpreadsheetView.cpp"
