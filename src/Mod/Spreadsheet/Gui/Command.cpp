/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2015 Eivind Kvedalen <eivind@kvedalen.name>             *
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
#include <sstream>
#endif

#if defined(FC_OS_WIN32)
#include <sys/timeb.h>
#endif

#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/FileDialog.h>
#include <Gui/MainWindow.h>
#include <Mod/Spreadsheet/App/Sheet.h>

#include "PropertiesDialog.h"
#include "SpreadsheetView.h"


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

using namespace SpreadsheetGui;
using namespace Spreadsheet;
using namespace Base;
using namespace App;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DEF_STD_CMD_A(CmdSpreadsheetMergeCells)

CmdSpreadsheetMergeCells::CmdSpreadsheetMergeCells()
    : Command("Spreadsheet_MergeCells")
{
    sAppModule = "Spreadsheet";
    sGroup = QT_TR_NOOP("Spreadsheet");
    sMenuText = QT_TR_NOOP("Merge cells");
    sToolTipText = QT_TR_NOOP("Merge selected cells");
    sWhatsThis = "Spreadsheet_MergeCells";
    sStatusTip = sToolTipText;
    sPixmap = "SpreadsheetMergeCells";
}

void CmdSpreadsheetMergeCells::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        SpreadsheetGui::SheetView* sheetView =
            freecad_dynamic_cast<SpreadsheetGui::SheetView>(activeWindow);

        if (sheetView) {
            Sheet* sheet = sheetView->getSheet();
            std::vector<Range> ranges = sheetView->selectedRanges();

            // Execute mergeCells commands
            if (!ranges.empty()) {
                Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Merge cells"));
                std::vector<Range>::const_iterator i = ranges.begin();
                for (; i != ranges.end(); ++i) {
                    if (i->size() > 1) {
                        Gui::Command::doCommand(Gui::Command::Doc,
                                                "App.ActiveDocument.%s.mergeCells('%s')",
                                                sheet->getNameInDocument(),
                                                i->rangeString().c_str());
                    }
                }
                Gui::Command::commitCommand();
                Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
            }
        }
    }
}

bool CmdSpreadsheetMergeCells::isActive()
{
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        SpreadsheetGui::SheetView* sheetView =
            freecad_dynamic_cast<SpreadsheetGui::SheetView>(activeWindow);

        if (sheetView) {
            return (sheetView->selectedIndexesRaw().size() > 1);
        }
    }
    return false;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DEF_STD_CMD_A(CmdSpreadsheetSplitCell)

CmdSpreadsheetSplitCell::CmdSpreadsheetSplitCell()
    : Command("Spreadsheet_SplitCell")
{
    sAppModule = "Spreadsheet";
    sGroup = QT_TR_NOOP("Spreadsheet");
    sMenuText = QT_TR_NOOP("Split cell");
    sToolTipText = QT_TR_NOOP("Split previously merged cells");
    sWhatsThis = "Spreadsheet_SplitCell";
    sStatusTip = sToolTipText;
    sPixmap = "SpreadsheetSplitCell";
}

void CmdSpreadsheetSplitCell::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        SpreadsheetGui::SheetView* sheetView =
            freecad_dynamic_cast<SpreadsheetGui::SheetView>(activeWindow);

        if (sheetView) {
            Sheet* sheet = sheetView->getSheet();
            QModelIndex current = sheetView->currentIndex();

            if (current.isValid()) {
                std::string address = CellAddress(current.row(), current.column()).toString();
                Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Split cell"));
                Gui::Command::doCommand(Gui::Command::Doc,
                                        "App.ActiveDocument.%s.splitCell('%s')",
                                        sheet->getNameInDocument(),
                                        address.c_str());
                Gui::Command::commitCommand();
                Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
            }
        }
    }
}

bool CmdSpreadsheetSplitCell::isActive()
{
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        SpreadsheetGui::SheetView* sheetView =
            freecad_dynamic_cast<SpreadsheetGui::SheetView>(activeWindow);

        if (sheetView) {
            QModelIndex current = sheetView->currentIndex();
            Sheet* sheet = sheetView->getSheet();

            if (current.isValid()) {
                return (sheetView->selectedIndexesRaw().size() == 1
                        && sheet->isMergedCell(CellAddress(current.row(), current.column())));
            }
        }
    }
    return false;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DEF_STD_CMD_A(CmdSpreadsheetImport)

CmdSpreadsheetImport::CmdSpreadsheetImport()
    : Command("Spreadsheet_Import")
{
    sAppModule = "Spreadsheet";
    sGroup = QT_TR_NOOP("Spreadsheet");
    sMenuText = QT_TR_NOOP("Import spreadsheet");
    sToolTipText = QT_TR_NOOP("Import CSV file into spreadsheet");
    sWhatsThis = "Spreadsheet_Import";
    sStatusTip = sToolTipText;
    sPixmap = "SpreadsheetImport";
}

void CmdSpreadsheetImport::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    QString selectedFilter;
    QString formatList = QObject::tr("CSV (*.csv *.CSV);;All (*)");
    QString fileName = Gui::FileDialog::getOpenFileName(Gui::getMainWindow(),
                                                        QObject::tr("Import file"),
                                                        QString(),
                                                        formatList,
                                                        &selectedFilter);
    if (!fileName.isEmpty()) {
        std::string FeatName = getUniqueObjectName("Spreadsheet");
        Sheet* sheet = freecad_dynamic_cast<Sheet>(
            App::GetApplication().getActiveDocument()->addObject("Spreadsheet::Sheet",
                                                                 FeatName.c_str()));
        if (sheet) {
            char delim, quote, escape;
            std::string errMsg = "Import";
            bool isValid = sheet->getCharsFromPrefs(delim, quote, escape, errMsg);

            if (isValid) {
                sheet->importFromFile(fileName.toStdString(), delim, quote, escape);
                sheet->execute();
            }
            else {
                Base::Console().Error(errMsg.c_str());
                return;
            }
        }
    }
}

bool CmdSpreadsheetImport::isActive()
{
    return getActiveGuiDocument() ? true : false;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DEF_STD_CMD_A(CmdSpreadsheetExport)

CmdSpreadsheetExport::CmdSpreadsheetExport()
    : Command("Spreadsheet_Export")
{
    sAppModule = "Spreadsheet";
    sGroup = QT_TR_NOOP("Spreadsheet");
    sMenuText = QT_TR_NOOP("Export spreadsheet");
    sToolTipText = QT_TR_NOOP("Export spreadsheet to CSV file");
    sWhatsThis = "Spreadsheet_Export";
    sStatusTip = sToolTipText;
    sPixmap = "SpreadsheetExport";
}

void CmdSpreadsheetExport::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        SpreadsheetGui::SheetView* sheetView =
            freecad_dynamic_cast<SpreadsheetGui::SheetView>(activeWindow);

        if (sheetView) {
            Sheet* sheet = sheetView->getSheet();
            QString selectedFilter;
            QString formatList = QObject::tr("CSV (*.csv *.CSV);;All (*)");
            QString fileName = Gui::FileDialog::getSaveFileName(Gui::getMainWindow(),
                                                                QObject::tr("Export file"),
                                                                QString(),
                                                                formatList,
                                                                &selectedFilter);
            if (!fileName.isEmpty()) {
                if (sheet) {
                    char delim, quote, escape;
                    std::string errMsg = "Export";
                    bool isValid = sheet->getCharsFromPrefs(delim, quote, escape, errMsg);

                    if (isValid) {
                        sheet->exportToFile(fileName.toStdString(), delim, quote, escape);
                    }
                    else {
                        Base::Console().Error(errMsg.c_str());
                        return;
                    }
                }
            }
        }
    }
}

bool CmdSpreadsheetExport::isActive()
{
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        if (activeWindow && freecad_dynamic_cast<SpreadsheetGui::SheetView>(activeWindow)) {
            return true;
        }
    }
    return false;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DEF_STD_CMD_A(CmdSpreadsheetAlignLeft)

CmdSpreadsheetAlignLeft::CmdSpreadsheetAlignLeft()
    : Command("Spreadsheet_AlignLeft")
{
    sAppModule = "Spreadsheet";
    sGroup = QT_TR_NOOP("Spreadsheet");
    sMenuText = QT_TR_NOOP("Align left");
    sToolTipText = QT_TR_NOOP("Left-align contents of selected cells");
    sWhatsThis = "Spreadsheet_AlignLeft";
    sStatusTip = sToolTipText;
    sPixmap = "SpreadsheetAlignLeft";
}

void CmdSpreadsheetAlignLeft::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        SpreadsheetGui::SheetView* sheetView =
            freecad_dynamic_cast<SpreadsheetGui::SheetView>(activeWindow);

        if (sheetView) {
            Sheet* sheet = sheetView->getSheet();
            std::vector<Range> ranges = sheetView->selectedRanges();

            if (!ranges.empty()) {
                std::vector<Range>::const_iterator i = ranges.begin();

                Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Left-align cell"));
                for (; i != ranges.end(); ++i) {
                    Gui::Command::doCommand(
                        Gui::Command::Doc,
                        "App.ActiveDocument.%s.setAlignment('%s', 'left', 'keep')",
                        sheet->getNameInDocument(),
                        i->rangeString().c_str());
                }
                Gui::Command::commitCommand();
                Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
            }
        }
    }
}

bool CmdSpreadsheetAlignLeft::isActive()
{
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        if (activeWindow && freecad_dynamic_cast<SpreadsheetGui::SheetView>(activeWindow)) {
            return true;
        }
    }
    return false;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DEF_STD_CMD_A(CmdSpreadsheetAlignCenter)

CmdSpreadsheetAlignCenter::CmdSpreadsheetAlignCenter()
    : Command("Spreadsheet_AlignCenter")
{
    sAppModule = "Spreadsheet";
    sGroup = QT_TR_NOOP("Spreadsheet");
    sMenuText = QT_TR_NOOP("Align center");
    sToolTipText = QT_TR_NOOP("Center-align contents of selected cells");
    sWhatsThis = "Spreadsheet_AlignCenter";
    sStatusTip = sToolTipText;
    sPixmap = "SpreadsheetAlignCenter";
}

void CmdSpreadsheetAlignCenter::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        SpreadsheetGui::SheetView* sheetView =
            freecad_dynamic_cast<SpreadsheetGui::SheetView>(activeWindow);

        if (sheetView) {
            Sheet* sheet = sheetView->getSheet();
            std::vector<Range> ranges = sheetView->selectedRanges();

            if (!ranges.empty()) {
                std::vector<Range>::const_iterator i = ranges.begin();

                Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Center cell"));
                for (; i != ranges.end(); ++i) {
                    Gui::Command::doCommand(
                        Gui::Command::Doc,
                        "App.ActiveDocument.%s.setAlignment('%s', 'center', 'keep')",
                        sheet->getNameInDocument(),
                        i->rangeString().c_str());
                }
                Gui::Command::commitCommand();
                Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
            }
        }
    }
}

bool CmdSpreadsheetAlignCenter::isActive()
{
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        if (activeWindow && freecad_dynamic_cast<SpreadsheetGui::SheetView>(activeWindow)) {
            return true;
        }
    }
    return false;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DEF_STD_CMD_A(CmdSpreadsheetAlignRight)

CmdSpreadsheetAlignRight::CmdSpreadsheetAlignRight()
    : Command("Spreadsheet_AlignRight")
{
    sAppModule = "Spreadsheet";
    sGroup = QT_TR_NOOP("Spreadsheet");
    sMenuText = QT_TR_NOOP("Align right");
    sToolTipText = QT_TR_NOOP("Right-align contents of selected cells");
    sWhatsThis = "Spreadsheet_AlignRight";
    sStatusTip = sToolTipText;
    sPixmap = "SpreadsheetAlignRight";
}

void CmdSpreadsheetAlignRight::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        SpreadsheetGui::SheetView* sheetView =
            freecad_dynamic_cast<SpreadsheetGui::SheetView>(activeWindow);

        if (sheetView) {
            Sheet* sheet = sheetView->getSheet();
            std::vector<Range> ranges = sheetView->selectedRanges();

            if (!ranges.empty()) {
                std::vector<Range>::const_iterator i = ranges.begin();

                Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Right-align cell"));
                for (; i != ranges.end(); ++i) {
                    Gui::Command::doCommand(
                        Gui::Command::Doc,
                        "App.ActiveDocument.%s.setAlignment('%s', 'right', 'keep')",
                        sheet->getNameInDocument(),
                        i->rangeString().c_str());
                }
                Gui::Command::commitCommand();
                Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
            }
        }
    }
}

bool CmdSpreadsheetAlignRight::isActive()
{
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        if (activeWindow && freecad_dynamic_cast<SpreadsheetGui::SheetView>(activeWindow)) {
            return true;
        }
    }
    return false;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DEF_STD_CMD_A(CmdSpreadsheetAlignTop)

CmdSpreadsheetAlignTop::CmdSpreadsheetAlignTop()
    : Command("Spreadsheet_AlignTop")
{
    sAppModule = "Spreadsheet";
    sGroup = QT_TR_NOOP("Spreadsheet");
    sMenuText = QT_TR_NOOP("Align top");
    sToolTipText = QT_TR_NOOP("Top-align contents of selected cells");
    sWhatsThis = "Spreadsheet_AlignTop";
    sStatusTip = sToolTipText;
    sPixmap = "SpreadsheetAlignTop";
}

void CmdSpreadsheetAlignTop::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        SpreadsheetGui::SheetView* sheetView =
            freecad_dynamic_cast<SpreadsheetGui::SheetView>(activeWindow);

        if (sheetView) {
            Sheet* sheet = sheetView->getSheet();
            std::vector<Range> ranges = sheetView->selectedRanges();

            if (!ranges.empty()) {
                std::vector<Range>::const_iterator i = ranges.begin();

                Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Top-align cell"));
                for (; i != ranges.end(); ++i) {
                    Gui::Command::doCommand(
                        Gui::Command::Doc,
                        "App.ActiveDocument.%s.setAlignment('%s', 'top', 'keep')",
                        sheet->getNameInDocument(),
                        i->rangeString().c_str());
                }
                Gui::Command::commitCommand();
                Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
            }
        }
    }
}

bool CmdSpreadsheetAlignTop::isActive()
{
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        if (activeWindow && freecad_dynamic_cast<SpreadsheetGui::SheetView>(activeWindow)) {
            return true;
        }
    }
    return false;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DEF_STD_CMD_A(CmdSpreadsheetAlignBottom)

CmdSpreadsheetAlignBottom::CmdSpreadsheetAlignBottom()
    : Command("Spreadsheet_AlignBottom")
{
    sAppModule = "Spreadsheet";
    sGroup = QT_TR_NOOP("Spreadsheet");
    sMenuText = QT_TR_NOOP("Align bottom");
    sToolTipText = QT_TR_NOOP("Bottom-align contents of selected cells");
    sWhatsThis = "Spreadsheet_AlignBottom";
    sStatusTip = sToolTipText;
    sPixmap = "SpreadsheetAlignBottom";
}

void CmdSpreadsheetAlignBottom::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        SpreadsheetGui::SheetView* sheetView =
            freecad_dynamic_cast<SpreadsheetGui::SheetView>(activeWindow);

        if (sheetView) {
            Sheet* sheet = sheetView->getSheet();
            std::vector<Range> ranges = sheetView->selectedRanges();

            if (!ranges.empty()) {
                std::vector<Range>::const_iterator i = ranges.begin();

                Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Bottom-align cell"));
                for (; i != ranges.end(); ++i) {
                    Gui::Command::doCommand(
                        Gui::Command::Doc,
                        "App.ActiveDocument.%s.setAlignment('%s', 'bottom', 'keep')",
                        sheet->getNameInDocument(),
                        i->rangeString().c_str());
                }
                Gui::Command::commitCommand();
                Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
            }
        }
    }
}

bool CmdSpreadsheetAlignBottom::isActive()
{
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        if (activeWindow && freecad_dynamic_cast<SpreadsheetGui::SheetView>(activeWindow)) {
            return true;
        }
    }
    return false;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DEF_STD_CMD_A(CmdSpreadsheetAlignVCenter)

CmdSpreadsheetAlignVCenter::CmdSpreadsheetAlignVCenter()
    : Command("Spreadsheet_AlignVCenter")
{
    sAppModule = "Spreadsheet";
    sGroup = QT_TR_NOOP("Spreadsheet");
    sMenuText = QT_TR_NOOP("Vertically center-align");
    sToolTipText = QT_TR_NOOP("Vertically center-align contents of selected cells");
    sWhatsThis = "Spreadsheet_AlignVCenter";
    sStatusTip = sToolTipText;
    sPixmap = "SpreadsheetAlignVCenter";
}

void CmdSpreadsheetAlignVCenter::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        SpreadsheetGui::SheetView* sheetView =
            freecad_dynamic_cast<SpreadsheetGui::SheetView>(activeWindow);

        if (sheetView) {
            Sheet* sheet = sheetView->getSheet();
            std::vector<Range> ranges = sheetView->selectedRanges();

            if (!ranges.empty()) {
                std::vector<Range>::const_iterator i = ranges.begin();

                Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Vertically center cells"));
                for (; i != ranges.end(); ++i) {
                    Gui::Command::doCommand(
                        Gui::Command::Doc,
                        "App.ActiveDocument.%s.setAlignment('%s', 'vcenter', 'keep')",
                        sheet->getNameInDocument(),
                        i->rangeString().c_str());
                }
                Gui::Command::commitCommand();
                Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
            }
        }
    }
}

bool CmdSpreadsheetAlignVCenter::isActive()
{
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        if (activeWindow && freecad_dynamic_cast<SpreadsheetGui::SheetView>(activeWindow)) {
            return true;
        }
    }
    return false;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DEF_STD_CMD_A(CmdSpreadsheetStyleBold)

CmdSpreadsheetStyleBold::CmdSpreadsheetStyleBold()
    : Command("Spreadsheet_StyleBold")
{
    sAppModule = "Spreadsheet";
    sGroup = QT_TR_NOOP("Spreadsheet");
    sMenuText = QT_TR_NOOP("Bold text");
    sToolTipText = QT_TR_NOOP("Set text in selected cells bold");
    sWhatsThis = "Spreadsheet_StyleBold";
    sStatusTip = sToolTipText;
    sPixmap = "SpreadsheetStyleBold";
}

void CmdSpreadsheetStyleBold::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        SpreadsheetGui::SheetView* sheetView =
            freecad_dynamic_cast<SpreadsheetGui::SheetView>(activeWindow);

        if (sheetView) {
            Sheet* sheet = sheetView->getSheet();
            QModelIndexList selection = sheetView->selectedIndexes();

            if (!selection.empty()) {
                bool allBold = true;

                for (const auto& it : selection) {
                    const Cell* cell = sheet->getCell(CellAddress(it.row(), it.column()));

                    if (cell) {
                        std::set<std::string> style;

                        cell->getStyle(style);
                        if (style.find("bold") == style.end()) {
                            allBold = false;
                            break;
                        }
                    }
                }

                std::vector<Range> ranges = sheetView->selectedRanges();
                std::vector<Range>::const_iterator i = ranges.begin();

                Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Set bold text"));
                for (; i != ranges.end(); ++i) {
                    if (!allBold) {
                        Gui::Command::doCommand(
                            Gui::Command::Doc,
                            "App.ActiveDocument.%s.setStyle('%s', 'bold', 'add')",
                            sheet->getNameInDocument(),
                            i->rangeString().c_str());
                    }
                    else {
                        Gui::Command::doCommand(
                            Gui::Command::Doc,
                            "App.ActiveDocument.%s.setStyle('%s', 'bold', 'remove')",
                            sheet->getNameInDocument(),
                            i->rangeString().c_str());
                    }
                }
                Gui::Command::commitCommand();
                Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
            }
        }
    }
}

bool CmdSpreadsheetStyleBold::isActive()
{
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        if (activeWindow && freecad_dynamic_cast<SpreadsheetGui::SheetView>(activeWindow)) {
            return true;
        }
    }
    return false;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DEF_STD_CMD_A(CmdSpreadsheetStyleItalic)

CmdSpreadsheetStyleItalic::CmdSpreadsheetStyleItalic()
    : Command("Spreadsheet_StyleItalic")
{
    sAppModule = "Spreadsheet";
    sGroup = QT_TR_NOOP("Spreadsheet");
    sMenuText = QT_TR_NOOP("Italic text");
    sToolTipText = QT_TR_NOOP("Set text in selected cells italic");
    sWhatsThis = "Spreadsheet_StyleItalic";
    sStatusTip = sToolTipText;
    sPixmap = "SpreadsheetStyleItalic";
}

void CmdSpreadsheetStyleItalic::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        SpreadsheetGui::SheetView* sheetView =
            freecad_dynamic_cast<SpreadsheetGui::SheetView>(activeWindow);

        if (sheetView) {
            Sheet* sheet = sheetView->getSheet();
            QModelIndexList selection = sheetView->selectedIndexes();

            if (!selection.empty()) {
                bool allItalic = true;

                for (const auto& it : selection) {
                    const Cell* cell = sheet->getCell(CellAddress(it.row(), it.column()));

                    if (cell) {
                        std::set<std::string> style;

                        cell->getStyle(style);
                        if (style.find("italic") == style.end()) {
                            allItalic = false;
                            break;
                        }
                    }
                }

                std::vector<Range> ranges = sheetView->selectedRanges();
                std::vector<Range>::const_iterator i = ranges.begin();

                Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Set italic text"));
                for (; i != ranges.end(); ++i) {
                    if (!allItalic) {
                        Gui::Command::doCommand(
                            Gui::Command::Doc,
                            "App.ActiveDocument.%s.setStyle('%s', 'italic', 'add')",
                            sheet->getNameInDocument(),
                            i->rangeString().c_str());
                    }
                    else {
                        Gui::Command::doCommand(
                            Gui::Command::Doc,
                            "App.ActiveDocument.%s.setStyle('%s', 'italic', 'remove')",
                            sheet->getNameInDocument(),
                            i->rangeString().c_str());
                    }
                }
                Gui::Command::commitCommand();
                Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
            }
        }
    }
}

bool CmdSpreadsheetStyleItalic::isActive()
{
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        if (activeWindow && freecad_dynamic_cast<SpreadsheetGui::SheetView>(activeWindow)) {
            return true;
        }
    }
    return false;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DEF_STD_CMD_A(CmdSpreadsheetStyleUnderline)

CmdSpreadsheetStyleUnderline::CmdSpreadsheetStyleUnderline()
    : Command("Spreadsheet_StyleUnderline")
{
    sAppModule = "Spreadsheet";
    sGroup = QT_TR_NOOP("Spreadsheet");
    sMenuText = QT_TR_NOOP("Underline text");
    sToolTipText = QT_TR_NOOP("Underline text in selected cells");
    sWhatsThis = "Spreadsheet_StyleUnderline";
    sStatusTip = sToolTipText;
    sPixmap = "SpreadsheetStyleUnderline";
}

void CmdSpreadsheetStyleUnderline::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        SpreadsheetGui::SheetView* sheetView =
            freecad_dynamic_cast<SpreadsheetGui::SheetView>(activeWindow);

        if (sheetView) {
            Sheet* sheet = sheetView->getSheet();
            QModelIndexList selection = sheetView->selectedIndexes();

            if (!selection.empty()) {
                bool allUnderline = true;

                for (const auto& it : selection) {
                    const Cell* cell = sheet->getCell(CellAddress(it.row(), it.column()));

                    if (cell) {
                        std::set<std::string> style;

                        cell->getStyle(style);
                        if (style.find("underline") == style.end()) {
                            allUnderline = false;
                            break;
                        }
                    }
                }

                std::vector<Range> ranges = sheetView->selectedRanges();
                std::vector<Range>::const_iterator i = ranges.begin();

                Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Set underline text"));
                for (; i != ranges.end(); ++i) {
                    if (!allUnderline) {
                        Gui::Command::doCommand(
                            Gui::Command::Doc,
                            "App.ActiveDocument.%s.setStyle('%s', 'underline', 'add')",
                            sheet->getNameInDocument(),
                            i->rangeString().c_str());
                    }
                    else {
                        Gui::Command::doCommand(
                            Gui::Command::Doc,
                            "App.ActiveDocument.%s.setStyle('%s', 'underline', 'remove')",
                            sheet->getNameInDocument(),
                            i->rangeString().c_str());
                    }
                }
                Gui::Command::commitCommand();
                Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
            }
        }
    }
}

bool CmdSpreadsheetStyleUnderline::isActive()
{
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        if (activeWindow && freecad_dynamic_cast<SpreadsheetGui::SheetView>(activeWindow)) {
            return true;
        }
    }
    return false;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DEF_STD_CMD_A(CmdSpreadsheetSetAlias)

CmdSpreadsheetSetAlias::CmdSpreadsheetSetAlias()
    : Command("Spreadsheet_SetAlias")
{
    sAppModule = "Spreadsheet";
    sGroup = QT_TR_NOOP("Spreadsheet");
    sMenuText = QT_TR_NOOP("Set alias");
    sToolTipText = QT_TR_NOOP("Set alias for selected cell");
    sWhatsThis = "Spreadsheet_SetAlias";
    sStatusTip = sToolTipText;
    sAccel = "Ctrl+Shift+A";
    sPixmap = "SpreadsheetAlias";
}

void CmdSpreadsheetSetAlias::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        SpreadsheetGui::SheetView* sheetView =
            freecad_dynamic_cast<SpreadsheetGui::SheetView>(activeWindow);

        if (sheetView) {
            Sheet* sheet = sheetView->getSheet();
            QModelIndexList selection = sheetView->selectedIndexes();

            if (selection.size() == 1) {
                std::vector<Range> range;

                range.emplace_back(selection[0].row(),
                                   selection[0].column(),
                                   selection[0].row(),
                                   selection[0].column());

                std::unique_ptr<PropertiesDialog> dialog(
                    new PropertiesDialog(sheet, range, sheetView));

                dialog->selectAlias();

                if (dialog->exec() == QDialog::Accepted) {
                    dialog->apply();
                }
            }
        }
    }
}

bool CmdSpreadsheetSetAlias::isActive()
{
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();

        if (activeWindow) {
            SpreadsheetGui::SheetView* sheetView =
                freecad_dynamic_cast<SpreadsheetGui::SheetView>(activeWindow);

            if (sheetView) {
                QModelIndexList selection = sheetView->selectedIndexes();

                if (selection.size() == 1) {
                    return true;
                }
            }
        }
    }
    return false;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DEF_STD_CMD_A(CmdCreateSpreadsheet)

CmdCreateSpreadsheet::CmdCreateSpreadsheet()
    : Command("Spreadsheet_CreateSheet")
{
    sAppModule = "Spreadsheet";
    sGroup = QT_TR_NOOP("Spreadsheet");
    sMenuText = QT_TR_NOOP("Create spreadsheet");
    sToolTipText = QT_TR_NOOP("Create a new spreadsheet");
    sWhatsThis = "Spreadsheet_CreateSheet";
    sStatusTip = sToolTipText;
    sPixmap = "Spreadsheet";
}

void CmdCreateSpreadsheet::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::string FeatName = getUniqueObjectName("Spreadsheet");

    openCommand(QT_TRANSLATE_NOOP("Command", "Create Spreadsheet"));
    doCommand(Doc, "App.activeDocument().addObject('Spreadsheet::Sheet','%s\')", FeatName.c_str());
    doCommand(Gui, "Gui.Selection.clearSelection()\n");
    doCommand(Gui, "Gui.Selection.addSelection(App.activeDocument().Name,'%s\')", FeatName.c_str());
    commitCommand();
}

bool CmdCreateSpreadsheet::isActive()
{
    return App::GetApplication().getActiveDocument();
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void CreateSpreadsheetCommands()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdCreateSpreadsheet());

    rcCmdMgr.addCommand(new CmdSpreadsheetMergeCells());
    rcCmdMgr.addCommand(new CmdSpreadsheetSplitCell());

    rcCmdMgr.addCommand(new CmdSpreadsheetImport());
    rcCmdMgr.addCommand(new CmdSpreadsheetExport());

    rcCmdMgr.addCommand(new CmdSpreadsheetAlignLeft());
    rcCmdMgr.addCommand(new CmdSpreadsheetAlignCenter());
    rcCmdMgr.addCommand(new CmdSpreadsheetAlignRight());

    rcCmdMgr.addCommand(new CmdSpreadsheetAlignTop());
    rcCmdMgr.addCommand(new CmdSpreadsheetAlignVCenter());
    rcCmdMgr.addCommand(new CmdSpreadsheetAlignBottom());

    rcCmdMgr.addCommand(new CmdSpreadsheetStyleBold());
    rcCmdMgr.addCommand(new CmdSpreadsheetStyleItalic());
    rcCmdMgr.addCommand(new CmdSpreadsheetStyleUnderline());

    rcCmdMgr.addCommand(new CmdSpreadsheetSetAlias());
}
