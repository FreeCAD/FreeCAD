// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *   Copyright (c) 2015 Eivind Kvedalen (eivind@kvedalen.name)             *
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


#include <QToolBar>
#include <qobject.h>


#include "Mod/Spreadsheet/App/Sheet.h"
#include "Mod/Spreadsheet/Gui/SpreadsheetView.h"
#include <App/Document.h>
#include <App/Range.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Gui/MenuManager.h>
#include <Gui/ToolBarManager.h>

#include "Workbench.h"
#include "qtcolorpicker.h"


using namespace Base;
using namespace App;
using namespace SpreadsheetGui;
using namespace Spreadsheet;

#if 0  // needed for Qt's lupdate utility
    qApp->translate("Workbench", "Spreadsheet");
    qApp->translate("Workbench", "&Spreadsheet");
    qApp->translate("Workbench", "&Alignment");
    qApp->translate("Workbench", "&Styles");
#endif

/// @namespace ImageGui @class Workbench
TYPESYSTEM_SOURCE(SpreadsheetGui::Workbench, Gui::StdWorkbench)

Workbench::Workbench()
    : Gui::StdWorkbench()
    , initialized(false)
    , workbenchHelper(new WorkbenchHelper)
{}

Workbench::~Workbench() = default;

void Workbench::activated()
{
    if (!initialized) {
        QList<QToolBar*> bars = Gui::getMainWindow()->findChildren<QToolBar*>(
            QStringLiteral("Spreadsheet")
        );

        if (bars.size() == 1) {
            QToolBar* bar = bars[0];
            QtColorPicker* foregroundColor;
            QtColorPicker* backgroundColor;
            QPalette palette = Gui::getMainWindow()->palette();

            QList<QtColorPicker*> fgList = Gui::getMainWindow()->findChildren<QtColorPicker*>(
                QStringLiteral("Spreadsheet_ForegroundColor")
            );
            if (!fgList.empty()) {
                foregroundColor = fgList[0];
            }
            else {
                foregroundColor = new QtColorPicker(bar, palette.color(QPalette::WindowText));
                foregroundColor->setObjectName(QStringLiteral("Spreadsheet_ForegroundColor"));
                foregroundColor->setStandardColors();
                QObject::connect(
                    foregroundColor,
                    &QtColorPicker::colorSet,
                    workbenchHelper.get(),
                    &WorkbenchHelper::setForegroundColor
                );
                QObject::connect(
                    foregroundColor,
                    &QtColorPicker::colorCleared,
                    workbenchHelper.get(),
                    &WorkbenchHelper::clearForegroundColor
                );
            }
            foregroundColor->setToolTip(QObject::tr("Sets the text color of cells"));
            foregroundColor->setWhatsThis(QObject::tr("Sets the text color of spreadsheet cells"));
            foregroundColor->setStatusTip(QObject::tr("Sets the text color of spreadsheet cells"));
            bar->addWidget(foregroundColor);

            QList<QtColorPicker*> bgList = Gui::getMainWindow()->findChildren<QtColorPicker*>(
                QStringLiteral("Spreadsheet_BackgroundColor")
            );
            if (!bgList.empty()) {
                backgroundColor = bgList[0];
            }
            else {
                backgroundColor = new QtColorPicker(bar, palette.color(QPalette::Base));
                backgroundColor->setObjectName(QStringLiteral("Spreadsheet_BackgroundColor"));
                backgroundColor->setStandardColors();
                QObject::connect(
                    backgroundColor,
                    &QtColorPicker::colorSet,
                    workbenchHelper.get(),
                    &WorkbenchHelper::setBackgroundColor
                );
                QObject::connect(
                    backgroundColor,
                    &QtColorPicker::colorCleared,
                    workbenchHelper.get(),
                    &WorkbenchHelper::clearBackgroundColor
                );
            }
            backgroundColor->setToolTip(QObject::tr("Sets the background color of cells"));
            backgroundColor->setWhatsThis(QObject::tr("Sets the spreadsheet cells background color"));
            backgroundColor->setStatusTip(QObject::tr("Sets the background color of cells"));
            bar->addWidget(backgroundColor);

            initialized = false;
        }
    }
}

void WorkbenchHelper::setForegroundColor(const QColor& color)
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();

    if (!doc) {
        return;
    }

    Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
    SpreadsheetGui::SheetView* sheetView = freecad_cast<SpreadsheetGui::SheetView*>(activeWindow);

    if (!sheetView) {
        return;
    }

    Sheet* sheet = sheetView->getSheet();
    std::vector<Range> ranges = sheetView->selectedRanges();

    if (ranges.empty()) {
        return;
    }

    std::vector<Range>::const_iterator i = ranges.begin();

    sheet->getDocument()->openTransaction(QT_TRANSLATE_NOOP("Command", "Set text color"));
    for (; i != ranges.end(); ++i) {
        Gui::Command::doCommand(
            Gui::Command::Doc,
            "App.ActiveDocument.%s.setForeground('%s', (%f,%f,%f))",
            sheet->getNameInDocument(),
            i->rangeString().c_str(),
            color.redF(),
            color.greenF(),
            color.blueF()
        );
    }
    sheet->getDocument()->commitTransaction();
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
}

void SpreadsheetGui::WorkbenchHelper::clearForegroundColor()
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();

    if (!doc) {
        return;
    }

    Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
    SpreadsheetGui::SheetView* sheetView = freecad_cast<SpreadsheetGui::SheetView*>(activeWindow);

    if (!sheetView) {
        return;
    }

    Sheet* sheet = sheetView->getSheet();
    std::vector<Range> ranges = sheetView->selectedRanges();

    if (ranges.empty()) {
        return;
    }

    std::vector<Range>::const_iterator i = ranges.begin();

    sheet->getDocument()->openTransaction(QT_TRANSLATE_NOOP("Command", "Clear text color"));
    for (; i != ranges.end(); ++i) {
        Gui::Command::doCommand(
            Gui::Command::Doc,
            "App.ActiveDocument.%s.clearForeground('%s')",
            sheet->getNameInDocument(),
            i->rangeString().c_str()
        );
    }
    sheet->getDocument()->commitTransaction();
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
}

void WorkbenchHelper::setBackgroundColor(const QColor& color)
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();

    if (!doc) {
        return;
    }

    Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
    SpreadsheetGui::SheetView* sheetView = freecad_cast<SpreadsheetGui::SheetView*>(activeWindow);

    if (!sheetView) {
        return;
    }

    Sheet* sheet = sheetView->getSheet();
    std::vector<Range> ranges = sheetView->selectedRanges();

    if (ranges.empty()) {
        return;
    }

    std::vector<Range>::const_iterator i = ranges.begin();

    sheet->getDocument()->openTransaction(QT_TRANSLATE_NOOP("Command", "Set background color"));
    for (; i != ranges.end(); ++i) {
        Gui::Command::doCommand(
            Gui::Command::Doc,
            "App.ActiveDocument.%s.setBackground('%s', (%f,%f,%f))",
            sheet->getNameInDocument(),
            i->rangeString().c_str(),
            color.redF(),
            color.greenF(),
            color.blueF()
        );
    }
    sheet->getDocument()->commitTransaction();
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
}

void SpreadsheetGui::WorkbenchHelper::clearBackgroundColor()
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();

    if (!doc) {
        return;
    }

    Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
    SpreadsheetGui::SheetView* sheetView = freecad_cast<SpreadsheetGui::SheetView*>(activeWindow);

    if (!sheetView) {
        return;
    }

    Sheet* sheet = sheetView->getSheet();
    std::vector<Range> ranges = sheetView->selectedRanges();

    if (ranges.empty()) {
        return;
    }

    std::vector<Range>::const_iterator i = ranges.begin();

    sheet->getDocument()->openTransaction(QT_TRANSLATE_NOOP("Command", "Clear background color"));
    for (; i != ranges.end(); ++i) {
        Gui::Command::doCommand(
            Gui::Command::Doc,
            "App.ActiveDocument.%s.clearBackground('%s')",
            sheet->getNameInDocument(),
            i->rangeString().c_str()
        );
    }
    sheet->getDocument()->commitTransaction();
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
}

Gui::MenuItem* Workbench::setupMenuBar() const
{
    Gui::MenuItem* root = StdWorkbench::setupMenuBar();
    Gui::MenuItem* item = root->findItem("&Windows");

    Gui::MenuItem* spreadsheet = new Gui::MenuItem;
    root->insertItem(item, spreadsheet);

    // utilities
    Gui::MenuItem* alignments = new Gui::MenuItem;
    alignments->setCommand("&Alignment");
    *alignments << "Spreadsheet_AlignLeft"
                << "Spreadsheet_AlignCenter"
                << "Spreadsheet_AlignRight"
                << "Spreadsheet_AlignTop"
                << "Spreadsheet_AlignVCenter"
                << "Spreadsheet_AlignBottom";

    Gui::MenuItem* styles = new Gui::MenuItem;
    styles->setCommand("&Styles");
    *styles << "Spreadsheet_StyleBold"
            << "Spreadsheet_StyleItalic"
            << "Spreadsheet_StyleUnderline";

    spreadsheet->setCommand("&Spreadsheet");
    *spreadsheet << "Spreadsheet_CreateSheet"
                 << "Separator"
                 << "Spreadsheet_Import"
                 << "Spreadsheet_Export"
                 << "Separator"
                 << "Spreadsheet_MergeCells"
                 << "Spreadsheet_SplitCell"
                 << "Separator" << alignments << styles;

    return root;
}

Gui::ToolBarItem* Workbench::setupToolBars() const
{
    Gui::ToolBarItem* root = StdWorkbench::setupToolBars();
    Gui::ToolBarItem* part = new Gui::ToolBarItem(root);
    part->setCommand("Spreadsheet");
    *part << "Spreadsheet_CreateSheet"
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
          << "Spreadsheet_SetAlias"
          << "Separator";

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
