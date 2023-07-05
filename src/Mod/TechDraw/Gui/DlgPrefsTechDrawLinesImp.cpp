/***************************************************************************
 *   Copyright (c) 2023 Benjamin Bræstrup Sayoc <benj5378@outlook.com>     *
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
# include <vector>
#endif

#include <csignal>

#include <QIcon>
#include <QPushButton>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QTableWidgetItem>
#include <Base/Console.h>
#include <Mod/TechDraw/App/LineGroup.h>

#include "DlgPrefsTechDrawLinesImp.h"
#include "ui_DlgPrefsTechDrawLines.h"
#include "DrawGuiUtil.h"
#include "PreferencesGui.h"


using namespace TechDrawGui;
using namespace TechDraw;


DlgPrefsTechDrawLinesImp::DlgPrefsTechDrawLinesImp( QWidget* parent )
  : PreferencePage( parent )
  , ui(new Ui_DlgPrefsTechDrawLinesImp)
{
    ui->setupUi(this);
    connect(ui->pushButton, &QPushButton::pressed, this, &DlgPrefsTechDrawLinesImp::addRow);
    connect(ui->tableWidget, &QTableWidget::itemChanged, this, &DlgPrefsTechDrawLinesImp::cellEdited);

    ui->tableWidget->setColumnWidth(0, 120);
    ui->tableWidget->setColumnWidth(1, 150);
    populateCellsWithPreview();
}

DlgPrefsTechDrawLinesImp::~DlgPrefsTechDrawLinesImp()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgPrefsTechDrawLinesImp::saveSettings()
{
    Base::Console().Message("save function\n");
    ui->tableWidget->onSave();
}

void DlgPrefsTechDrawLinesImp::loadSettings()
{
    //set defaults f>pcbBalloonArrow->setCurrentIndex(prefBalloonArrow());
}

void DlgPrefsTechDrawLinesImp::cellEdited(QTableWidgetItem* item)
{
    if(ui->tableWidget->column(item) != 1) {
        return;  // Icon updates also triggers this function and can cause a loop
    }
    Base::Console().Message("is here----\n");
    populateCellsWithPreview();  // Hmmm, populates ALL cells again
}

void DlgPrefsTechDrawLinesImp::changeEvent(QEvent* e)
{
    return;
}

void DlgPrefsTechDrawLinesImp::addRow() const
{
    int insertionLocation = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(insertionLocation);
}

/**
 * Updates the tooltip of the LineGroup combobox
 */
void DlgPrefsTechDrawLinesImp::onLineGroupChanged(int index)
{
}

void DlgPrefsTechDrawLinesImp::populateCellsWithPreview()
{
    ui->tableWidget->setIconSize(QSize(250, 30));
    // int columnCount = ui->tableWidget->columnCount();
    for (int row = 0; row < ui->tableWidget->rowCount(); row++) {
        QString dashArray = ui->tableWidget->item(row, 1)->text();
        if (dashArraw.compare(QString("continuous")) == 0) {
            dashArray = "50 0";  // hax for rendering a continous line
        }

        if (!ui->tableWidget->item(row, 2)) {  // nullptr if cell has no contents
            // In this case, we have to add an empty item to the cell
            QTableWidgetItem* item = new QTableWidgetItem;
            ui->tableWidget->setItem(row, 2, item);
        }
        ui->tableWidget->item(row, 2)->setIcon(iconOfLineStyle(dashArray));
    }
}

QIcon DlgPrefsTechDrawLinesImp::iconOfLineStyle(QString dashArray)
{
    QStringList list = dashArray.split(QString::fromUtf8(" "), Qt::SkipEmptyParts);
    QVector<double> result;
    for (const QString& value : list) {
        result.push_back(value.toDouble());
    }
    return iconOfLineStyle(result);
}

//! Creates a QIcon containing a preview of the LineStyle defined by dashArray parameter
QIcon DlgPrefsTechDrawLinesImp::iconOfLineStyle(QVector<double> dashArray)
{
    QPainterPath path;
    path.moveTo(0.0, 2.5);
    path.lineTo(250.0, 2.5);

    QPen pen;
    pen.setWidth(2);
    pen.setBrush(QBrush(QColor(0, 0, 0)));
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    pen.setDashPattern(dashArray);

    QPixmap pixmap(250, 4);
    pixmap.fill(QColor(0, 0, 0, 0));

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(pen);
    painter.drawPath(path);

    return QIcon(pixmap);
}

#include <Mod/TechDraw/Gui/moc_DlgPrefsTechDrawLinesImp.cpp>
