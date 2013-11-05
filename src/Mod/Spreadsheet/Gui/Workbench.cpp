/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *   Copyright (c) 2013 Eivind Kvedalen (eivind@kvedalen.name)             *
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
# include <qobject.h>
#endif

#include "Workbench.h"
#include <Gui/ToolBarManager.h>
#include <Gui/Application.h>
#include <Gui/MainWindow.h>
#include <Gui/Command.h>
#include <QToolBar>
#include "qtcolorpicker.h"
#include "Mod/Spreadsheet/App/Sheet.h"
#include "Mod/Spreadsheet/Gui/SpreadsheetView.h"

using namespace SpreadsheetGui;

#if 0 // needed for Qt's lupdate utility
    qApp->translate("Workbench", "Spreadsheet");
#endif

/// @namespace ImageGui @class Workbench
TYPESYSTEM_SOURCE(SpreadsheetGui::Workbench, Gui::StdWorkbench)

Workbench::Workbench()
    : Gui::StdWorkbench()
    , initialized(false)
    , workbenchHelper(new WorkbenchHelper)
{
}

Workbench::~Workbench()
{
}

void Workbench::fillPalette(QtColorPicker * picker)
{
    picker->insertColor(QColor(0x00,0x00,0x00));
    picker->insertColor(QColor(0x00,0x00,0xaa));
    picker->insertColor(QColor(0x00,0xaa,0x00));
    picker->insertColor(QColor(0x00,0xaa,0xaa));

    picker->insertColor(QColor(0xaa,0x00,0x00));
    picker->insertColor(QColor(0xaa,0x00,0xaa));
    picker->insertColor(QColor(0xaa,0x55,0x00));
    picker->insertColor(QColor(0xaa,0xaa,0xaa));

    picker->insertColor(QColor(0x55,0x55,0xff));
    picker->insertColor(QColor(0x55,0xff,0x55));
    picker->insertColor(QColor(0x55,0xff,0xff));
    picker->insertColor(QColor(0xff,0x55,0x55));

    picker->insertColor(QColor(0xaa,0x55,0x55));
    picker->insertColor(QColor(0xff,0x55,0xff));
    picker->insertColor(QColor(0xff,0xff,0x55));
    picker->insertColor(QColor(0xff,0xff,0xff));
}

void Workbench::activated()
{
    if (!initialized) {
        QList<QToolBar*> bars = Gui::getMainWindow()->findChildren<QToolBar*>(QString::fromAscii("Spreadsheet"));

        if (bars.size() == 1) {
            QToolBar * bar = bars[0];
            QtColorPicker * foregroundColor;
            QtColorPicker * backgroundColor;

            foregroundColor = new QtColorPicker();
            foregroundColor->setObjectName(QString::fromAscii("Spreadsheet_ForegroundColor"));
            fillPalette(foregroundColor);
            QObject::connect(foregroundColor, SIGNAL(colorChanged(QColor)), workbenchHelper.get(), SLOT(setForegroundColor(QColor)));
            bar->addWidget(foregroundColor);

            backgroundColor = new QtColorPicker();
            backgroundColor->setObjectName(QString::fromAscii("Spreadsheet_BackgroundColor"));
            fillPalette(backgroundColor);
            QObject::connect(backgroundColor, SIGNAL(colorChanged(QColor)), workbenchHelper.get(), SLOT(setBackgroundColor(QColor)));
            bar->addWidget(backgroundColor);

            initialized = true;
        }
    }
}

void WorkbenchHelper::setForegroundColor(const QColor & color)
{
    Gui::Document * doc = Gui::Application::Instance->activeDocument();

    if (doc) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        SpreadsheetGui::SheetView * sheetView = dynamic_cast<SpreadsheetGui::SheetView*>(activeWindow);

        if (sheetView) {
            Spreadsheet::Sheet * sheet = sheetView->getSheet();
            std::vector<Spreadsheet::Sheet::Range> ranges = sheetView->selectedRanges();

            // Execute mergeCells commands
            if (ranges.size() > 0) {
                std::vector<Spreadsheet::Sheet::Range>::const_iterator i = ranges.begin();

                Gui::Command::openCommand("Set foreground color");
                for (; i != ranges.end(); ++i)
                        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.setForeground('%s', (%f,%f,%f))", sheet->getNameInDocument(),
                                                i->rangeString().c_str(), color.redF(), color.greenF(), color.blueF());
                Gui::Command::commitCommand();
                Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
            }
        }
    }
}

void WorkbenchHelper::setBackgroundColor(const QColor & color)
{
    Gui::Document * doc = Gui::Application::Instance->activeDocument();

    if (doc) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        SpreadsheetGui::SheetView * sheetView = dynamic_cast<SpreadsheetGui::SheetView*>(activeWindow);

        if (sheetView) {
            Spreadsheet::Sheet * sheet = sheetView->getSheet();
            std::vector<Spreadsheet::Sheet::Range> ranges = sheetView->selectedRanges();

            // Execute mergeCells commands
            if (ranges.size() > 0) {
                std::vector<Spreadsheet::Sheet::Range>::const_iterator i = ranges.begin();

                Gui::Command::openCommand("Set background color");
                for (; i != ranges.end(); ++i)
                        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.setBackground('%s', (%f,%f,%f))", sheet->getNameInDocument(),
                                                i->rangeString().c_str(), color.redF(), color.greenF(), color.blueF());
                Gui::Command::commitCommand();
                Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
            }
        }
    }
}

Gui::ToolBarItem* Workbench::setupToolBars() const
{
    Gui::ToolBarItem* root = StdWorkbench::setupToolBars();
    Gui::ToolBarItem* part = new Gui::ToolBarItem(root);
    part->setCommand("Spreadsheet");
    *part << "Spreadsheet_CreateSheet"
          << "Spreadsheet_Controller"
          << "Separator"
          << "Spreadsheet_Import"
          << "Spreadsheet_Export"
          << "Separator"
          << "Spreadsheet_MergeCells"
          << "Spreadsheet_SplitCell"
          << "Separator"
          << "Spreadsheet_AlignLeft"
          << "Spreadsheet_AlignCenter"
          << "Spreadsheet_AlignRight"
          << "Spreadsheet_AlignTop"
          << "Spreadsheet_AlignVCenter"
          << "Spreadsheet_AlignBottom"
          << "Separator"
          << "Spreadsheet_StyleBold"
          << "Spreadsheet_StyleItalic"
          << "Spreadsheet_StyleUnderline"
          << "Separator"
             ;

    return root;
}

Gui::ToolBarItem* Workbench::setupCommandBars() const
{
    Gui::ToolBarItem* root = new Gui::ToolBarItem;
    Gui::ToolBarItem* ss = new Gui::ToolBarItem(root);
    ss->setCommand("Spreadsheet");
    *ss << "Spreadsheet_Open";
    return root;
}

#include "moc_Workbench.cpp"
