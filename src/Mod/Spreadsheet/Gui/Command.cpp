/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *   for detail see the LICENCE text file.                                 *
 *   Jürgen Riegel 2002                                                    *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QAction>
# include <QFileDialog>
# include <QImage>
# include <QImageReader>
# include <QMessageBox>
# include <QTextStream>
#endif

#include <time.h>
#include <sys/timeb.h>

#include <Base/Exception.h>
#include <Base/Interpreter.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/MainWindow.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/ViewProvider.h>
#include <Gui/BitmapFactory.h>
# include <Gui/FileDialog.h>

#include "SpreadsheetView.h"
#include "../App/Sheet.h"
#include "ViewProviderSpreadsheet.h"


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

using namespace SpreadsheetGui;

DEF_STD_CMD_A(CmdSpreadsheetController);

CmdSpreadsheetController::CmdSpreadsheetController()
  : Command("Spreadsheet_Controller")
{
    sAppModule      = "Spreadsheet";
    sGroup          = QT_TR_NOOP("Spreadsheet");
    sMenuText       = QT_TR_NOOP("Spreadsheet controller...");
    sToolTipText    = QT_TR_NOOP("Add Spreadsheet controller");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "SpreadsheetController";
}

void CmdSpreadsheetController::activated(int iMsg)
{
}

bool CmdSpreadsheetController::isActive()
{
    return App::GetApplication().getActiveDocument();
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DEF_STD_CMD_A(CmdSpreadsheetMergeCells);

CmdSpreadsheetMergeCells::CmdSpreadsheetMergeCells()
  : Command("Spreadsheet_MergeCells")
{
    sAppModule      = "Spreadsheet";
    sGroup          = QT_TR_NOOP("Spreadsheet");
    sMenuText       = QT_TR_NOOP("Merge cells");
    sToolTipText    = QT_TR_NOOP("Merge selected cells in spreadsheet");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "SpreadsheetMergeCells";
}

static void createRectangles(std::set<std::pair<int, int> > & cells, std::map<std::pair<int, int>, std::pair<int, int> > & rectangles)
{
    while (cells.size() != 0) {
        int row, col;
        int orgRow;
        int rows = 1;
        int cols = 1;

        orgRow = row = (*cells.begin()).first;
        col = (*cells.begin()).second;

        // Expand right first
        while (cells.find(std::make_pair<int,int>(row, col + cols)) != cells.end())
            ++cols;

        // Expand left
        while (cells.find(std::make_pair<int,int>(row, col + cols)) != cells.end()) {
            col--;
            ++cols;
        }

        // Try to expand cell up (the complete row above from [col,col + cols> needs to be in the cells variable)
        bool ok = true;
        while (ok) {
            for (int i = col; i < col + cols; ++i) {
                if ( cells.find(std::make_pair<int,int>(row - 1, i)) == cells.end()) {
                    ok = false;
                    break;
                }
            }
            if (ok) {
                // Complete row
                row--;
                rows++;
            }
            else
                break;
        }

        // Try to expand down (the complete row below from [col,col + cols> needs to be in the cells variable)
        ok = true;
        while (ok) {
            for (int i = col; i < col + cols; ++i) {
                if ( cells.find(std::make_pair<int,int>(orgRow + 1, i)) == cells.end()) {
                   ok = false;
                   break;
                }
            }
            if (ok) {
                // Complete row
                orgRow++;
                rows++;
            }
            else
                break;
        }

        // Remove entries from cell set for this rectangle
        for (int r = row; r < row + rows; ++r)
            for (int c = col; c < col + cols; ++c)
                cells.erase(std::make_pair<int,int>(r, c));

        // Insert into output variable
        rectangles[std::make_pair<int,int>(row, col)] = std::make_pair<int,int>(rows, cols);
    }
}


void CmdSpreadsheetMergeCells::activated(int iMsg)
{
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        SpreadsheetGui::SheetView * sheetView = dynamic_cast<SpreadsheetGui::SheetView*>(activeWindow);

        if (sheetView) {
            Spreadsheet::Sheet * sheet = sheetView->getSheet();
            QModelIndexList list = sheetView->selectedIndexes();

            // Insert selected cells into set. This variable should ideally be a hash_set
            // but that is not part of standard stl.
            std::set<std::pair<int, int> > cells;
            for (QModelIndexList::const_iterator it = list.begin(); it != list.end(); ++it)
                cells.insert(std::make_pair<int,int>((*it).row(), (*it).column()));

            // Create rectangular cells from the unordered collection of selected cells
            std::map<std::pair<int, int>, std::pair<int, int> > rectangles;
            createRectangles(cells, rectangles);

            // Execute mergeCells commands
            if (rectangles.size() > 0) {
                Gui::Command::openCommand("Merge cells");
                std::map<std::pair<int, int>, std::pair<int, int> >::const_iterator i = rectangles.begin();
                for (; i != rectangles.end(); ++i) {
                    std::pair<int, int> ur = (*i).first;
                    std::pair<int, int> size = (*i).second;
                    std::string from = Spreadsheet::Sheet::toAddress(ur.first, ur.second);
                    std::string to = Spreadsheet::Sheet::toAddress(ur.first + size.first - 1, ur.second + size.second - 1);

                    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.mergeCells('%s:%s')", sheet->getNameInDocument(),
                                            from.c_str(), to.c_str());
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
        if (activeWindow && dynamic_cast<SpreadsheetGui::SheetView*>(activeWindow))
            return true;

    }
    return false;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DEF_STD_CMD_A(CmdSpreadsheetSplitCell);

CmdSpreadsheetSplitCell::CmdSpreadsheetSplitCell()
  : Command("Spreadsheet_SplitCell")
{
    sAppModule      = "Spreadsheet";
    sGroup          = QT_TR_NOOP("Spreadsheet");
    sMenuText       = QT_TR_NOOP("Split cell");
    sToolTipText    = QT_TR_NOOP("Split previously merged cells in spreadsheet");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "SpreadsheetSplitCell";
}

void CmdSpreadsheetSplitCell::activated(int iMsg)
{
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        SpreadsheetGui::SheetView * sheetView = dynamic_cast<SpreadsheetGui::SheetView*>(activeWindow);

        if (sheetView) {
            Spreadsheet::Sheet * sheet = sheetView->getSheet();
            QModelIndex current = sheetView->currentIndex();

            if (current.isValid()) {
                std::string address = Spreadsheet::Sheet::toAddress(current.row(), current.column());
                Gui::Command::openCommand("Split cell");
                Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.splitCell('%s')", sheet->getNameInDocument(),
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
        SpreadsheetGui::SheetView * sheetView = dynamic_cast<SpreadsheetGui::SheetView*>(activeWindow);

        if (sheetView) {
            QModelIndex current = sheetView->currentIndex();
            Spreadsheet::Sheet * sheet = sheetView->getSheet();

            if (current.isValid())
                return sheet->isMergedCell(current.row(), current.column());
        }
    }
    return false;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DEF_STD_CMD(CmdSpreadsheetImport);

CmdSpreadsheetImport::CmdSpreadsheetImport()
  : Command("Spreadsheet_Import")
{
    sAppModule      = "Spreadsheet";
    sGroup          = QT_TR_NOOP("Spreadsheet");
    sMenuText       = QT_TR_NOOP("Import spreadsheet");
    sToolTipText    = QT_TR_NOOP("Import CSV file into spreadsheet");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "SpreadsheetImport";
}

void CmdSpreadsheetImport::activated(int iMsg)
{
    std::string FeatName = getUniqueObjectName("Spreadsheet");
    Spreadsheet::Sheet * sheet = dynamic_cast<Spreadsheet::Sheet*>(App::GetApplication().getActiveDocument()->addObject("Spreadsheet::Sheet", FeatName.c_str()));
    SheetView* sView = new SheetView(sheet, Gui::getMainWindow());
    sView->setWindowIcon( Gui::BitmapFactory().pixmap("Spreadsheet") );
    sView->setWindowTitle(QString::fromStdString(FeatName));
    sView->resize( 400, 300 );
    Gui::getMainWindow()->addWindow( sView );

    QString selectedFilter;
    QString formatList = QObject::tr("All (*)");
    QString fileName = Gui::FileDialog::getOpenFileName(Gui::getMainWindow(),
                                                        QObject::tr("Import file"),
                                                        QString(),
                                                        formatList,
                                                        &selectedFilter);
    if (!fileName.isEmpty())
        sheet->importFromFile(fileName.toStdString(), '\t', '"', '\\');
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DEF_STD_CMD_A(CmdSpreadsheetExport);

CmdSpreadsheetExport::CmdSpreadsheetExport()
  : Command("Spreadsheet_Export")
{
    sAppModule      = "Spreadsheet";
    sGroup          = QT_TR_NOOP("Spreadsheet");
    sMenuText       = QT_TR_NOOP("Export spreadsheet");
    sToolTipText    = QT_TR_NOOP("Export spreadsheet to CSV file");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "SpreadsheetExport";
}

void CmdSpreadsheetExport::activated(int iMsg)
{
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        SpreadsheetGui::SheetView * sheetView = dynamic_cast<SpreadsheetGui::SheetView*>(activeWindow);

        if (sheetView) {
            Spreadsheet::Sheet * sheet = sheetView->getSheet();
            QString selectedFilter;
            QString formatList = QObject::tr("All (*)");
            QString fileName = Gui::FileDialog::getSaveFileName(Gui::getMainWindow(),
                                                                QObject::tr("Export file"),
                                                                QString(),
                                                                formatList,
                                                                &selectedFilter);
            if (!fileName.isEmpty())
                sheet->exportToFile(fileName.toStdString(), '\t', '"', '\\');
        }
    }
}

bool CmdSpreadsheetExport::isActive()
{
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        if (activeWindow && dynamic_cast<SpreadsheetGui::SheetView*>(activeWindow))
            return true;

    }
    return false;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DEF_STD_CMD_A(CmdSpreadsheetAlignLeft);

CmdSpreadsheetAlignLeft::CmdSpreadsheetAlignLeft()
  : Command("Spreadsheet_AlignLeft")
{
    sAppModule      = "Spreadsheet";
    sGroup          = QT_TR_NOOP("Spreadsheet");
    sMenuText       = QT_TR_NOOP("Align left");
    sToolTipText    = QT_TR_NOOP("Left-align contents of selected cells");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "SpreadsheetAlignLeft";
}

void CmdSpreadsheetAlignLeft::activated(int iMsg)
{
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        SpreadsheetGui::SheetView * sheetView = dynamic_cast<SpreadsheetGui::SheetView*>(activeWindow);

        if (sheetView) {
            Spreadsheet::Sheet * sheet = sheetView->getSheet();
            QModelIndexList selection = sheetView->selectedIndexes();

            if (selection.size() > 0) {
                Gui::Command::openCommand("Left-align cell");
                for (QModelIndexList::const_iterator it = selection.begin(); it != selection.end(); ++it) {
                    std::string address = Spreadsheet::Sheet::toAddress((*it).row(), (*it).column());
                    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.setAlignment('%s', 'left', 'keep')", sheet->getNameInDocument(),
                                            address.c_str());
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
        if (activeWindow && dynamic_cast<SpreadsheetGui::SheetView*>(activeWindow))
            return true;

    }
    return false;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DEF_STD_CMD_A(CmdSpreadsheetAlignCenter);

CmdSpreadsheetAlignCenter::CmdSpreadsheetAlignCenter()
  : Command("Spreadsheet_AlignCenter")
{
    sAppModule      = "Spreadsheet";
    sGroup          = QT_TR_NOOP("Spreadsheet");
    sMenuText       = QT_TR_NOOP("Align center");
    sToolTipText    = QT_TR_NOOP("Center-align contents of selected cells");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "SpreadsheetAlignCenter";
}

void CmdSpreadsheetAlignCenter::activated(int iMsg)
{
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        SpreadsheetGui::SheetView * sheetView = dynamic_cast<SpreadsheetGui::SheetView*>(activeWindow);

        if (sheetView) {
            Spreadsheet::Sheet * sheet = sheetView->getSheet();
            QModelIndexList selection = sheetView->selectedIndexes();

            if (selection.size() > 0) {
                Gui::Command::openCommand("Center cell");
                for (QModelIndexList::const_iterator it = selection.begin(); it != selection.end(); ++it) {
                    std::string address = Spreadsheet::Sheet::toAddress((*it).row(), (*it).column());
                    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.setAlignment('%s', 'center', 'keep')", sheet->getNameInDocument(),
                                            address.c_str());
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
        if (activeWindow && dynamic_cast<SpreadsheetGui::SheetView*>(activeWindow))
            return true;

    }
    return false;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DEF_STD_CMD_A(CmdSpreadsheetAlignRight);

CmdSpreadsheetAlignRight::CmdSpreadsheetAlignRight()
  : Command("Spreadsheet_AlignRight")
{
    sAppModule      = "Spreadsheet";
    sGroup          = QT_TR_NOOP("Spreadsheet");
    sMenuText       = QT_TR_NOOP("Align right");
    sToolTipText    = QT_TR_NOOP("Right-align contents of selected cells");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "SpreadsheetAlignRight";
}

void CmdSpreadsheetAlignRight::activated(int iMsg)
{
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        SpreadsheetGui::SheetView * sheetView = dynamic_cast<SpreadsheetGui::SheetView*>(activeWindow);

        if (sheetView) {
            Spreadsheet::Sheet * sheet = sheetView->getSheet();
            QModelIndexList selection = sheetView->selectedIndexes();

            if (selection.size() > 0) {
                Gui::Command::openCommand("Right-align cell");
                for (QModelIndexList::const_iterator it = selection.begin(); it != selection.end(); ++it) {
                    std::string address = Spreadsheet::Sheet::toAddress((*it).row(), (*it).column());
                    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.setAlignment('%s', 'right', 'keep')", sheet->getNameInDocument(),
                                            address.c_str());
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
        if (activeWindow && dynamic_cast<SpreadsheetGui::SheetView*>(activeWindow))
            return true;

    }
    return false;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DEF_STD_CMD_A(CmdSpreadsheetAlignTop);

CmdSpreadsheetAlignTop::CmdSpreadsheetAlignTop()
  : Command("Spreadsheet_AlignTop")
{
    sAppModule      = "Spreadsheet";
    sGroup          = QT_TR_NOOP("Spreadsheet");
    sMenuText       = QT_TR_NOOP("Align top");
    sToolTipText    = QT_TR_NOOP("Top-align contents of selected cells");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "SpreadsheetAlignTop";
}

void CmdSpreadsheetAlignTop::activated(int iMsg)
{
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        SpreadsheetGui::SheetView * sheetView = dynamic_cast<SpreadsheetGui::SheetView*>(activeWindow);

        if (sheetView) {
            Spreadsheet::Sheet * sheet = sheetView->getSheet();
            QModelIndexList selection = sheetView->selectedIndexes();

            if (selection.size() > 0) {
                Gui::Command::openCommand("Top-align cell");
                for (QModelIndexList::const_iterator it = selection.begin(); it != selection.end(); ++it) {
                    std::string address = Spreadsheet::Sheet::toAddress((*it).row(), (*it).column());
                    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.setAlignment('%s', 'top', 'keep')", sheet->getNameInDocument(),
                                            address.c_str());
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
        if (activeWindow && dynamic_cast<SpreadsheetGui::SheetView*>(activeWindow))
            return true;

    }
    return false;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DEF_STD_CMD_A(CmdSpreadsheetAlignBottom);

CmdSpreadsheetAlignBottom::CmdSpreadsheetAlignBottom()
  : Command("Spreadsheet_AlignBottom")
{
    sAppModule      = "Spreadsheet";
    sGroup          = QT_TR_NOOP("Spreadsheet");
    sMenuText       = QT_TR_NOOP("Align bottom");
    sToolTipText    = QT_TR_NOOP("Bottom-align contents of selected cells");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "SpreadsheetAlignBottom";
}

void CmdSpreadsheetAlignBottom::activated(int iMsg)
{
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        SpreadsheetGui::SheetView * sheetView = dynamic_cast<SpreadsheetGui::SheetView*>(activeWindow);

        if (sheetView) {
            Spreadsheet::Sheet * sheet = sheetView->getSheet();
            QModelIndexList selection = sheetView->selectedIndexes();

            if (selection.size() > 0) {
                Gui::Command::openCommand("Bottom-align cell");
                for (QModelIndexList::const_iterator it = selection.begin(); it != selection.end(); ++it) {
                    std::string address = Spreadsheet::Sheet::toAddress((*it).row(), (*it).column());
                    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.setAlignment('%s', 'bottom', 'keep')", sheet->getNameInDocument(),
                                            address.c_str());
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
        if (activeWindow && dynamic_cast<SpreadsheetGui::SheetView*>(activeWindow))
            return true;

    }
    return false;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DEF_STD_CMD_A(CmdSpreadsheetAlignVCenter);

CmdSpreadsheetAlignVCenter::CmdSpreadsheetAlignVCenter()
  : Command("Spreadsheet_AlignVCenter")
{
    sAppModule      = "Spreadsheet";
    sGroup          = QT_TR_NOOP("Spreadsheet");
    sMenuText       = QT_TR_NOOP("Vertically center-align");
    sToolTipText    = QT_TR_NOOP("Center-align contents vertically of selected cells");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "SpreadsheetAlignVCenter";
}

void CmdSpreadsheetAlignVCenter::activated(int iMsg)
{
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        SpreadsheetGui::SheetView * sheetView = dynamic_cast<SpreadsheetGui::SheetView*>(activeWindow);

        if (sheetView) {
            Spreadsheet::Sheet * sheet = sheetView->getSheet();
            QModelIndexList selection = sheetView->selectedIndexes();

            if (selection.size() > 0) {
                Gui::Command::openCommand("Vertically center cells");
                for (QModelIndexList::const_iterator it = selection.begin(); it != selection.end(); ++it) {
                    std::string address = Spreadsheet::Sheet::toAddress((*it).row(), (*it).column());
                    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.setAlignment('%s', 'vcenter', 'keep')", sheet->getNameInDocument(),
                                            address.c_str());
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
        if (activeWindow && dynamic_cast<SpreadsheetGui::SheetView*>(activeWindow))
            return true;

    }
    return false;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DEF_STD_CMD_A(CmdSpreadsheetStyleBold);

CmdSpreadsheetStyleBold::CmdSpreadsheetStyleBold()
  : Command("Spreadsheet_StyleBold")
{
    sAppModule      = "Spreadsheet";
    sGroup          = QT_TR_NOOP("Spreadsheet");
    sMenuText       = QT_TR_NOOP("Bold text");
    sToolTipText    = QT_TR_NOOP("Set bold text in selected cells");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "SpreadsheetStyleBold";
}

void CmdSpreadsheetStyleBold::activated(int iMsg)
{
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        SpreadsheetGui::SheetView * sheetView = dynamic_cast<SpreadsheetGui::SheetView*>(activeWindow);

        if (sheetView) {
            Spreadsheet::Sheet * sheet = sheetView->getSheet();
            QModelIndexList selection = sheetView->selectedIndexes();

            if (selection.size() > 0) {
                Gui::Command::openCommand("Set bold text");
                bool allBold = true;

                for (QModelIndexList::const_iterator it = selection.begin(); it != selection.end(); ++it) {
                    std::set<std::string> style;

                    sheet->getStyle((*it).row(), (*it).column(), style);
                    if (style.find("bold") == style.end()) {
                        allBold = false;
                        break;
                    }
                }

                for (QModelIndexList::const_iterator it = selection.begin(); it != selection.end(); ++it) {
                    std::string address = Spreadsheet::Sheet::toAddress((*it).row(), (*it).column());
                    if (!allBold)
                        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.setStyle('%s', 'bold', 'add')", sheet->getNameInDocument(),
                                                address.c_str());
                    else
                        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.setStyle('%s', 'bold', 'remove')", sheet->getNameInDocument(),
                                                address.c_str());
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
        if (activeWindow && dynamic_cast<SpreadsheetGui::SheetView*>(activeWindow))
            return true;

    }
    return false;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DEF_STD_CMD_A(CmdSpreadsheetStyleItalic);

CmdSpreadsheetStyleItalic::CmdSpreadsheetStyleItalic()
  : Command("Spreadsheet_StyleItalic")
{
    sAppModule      = "Spreadsheet";
    sGroup          = QT_TR_NOOP("Spreadsheet");
    sMenuText       = QT_TR_NOOP("Bold text");
    sToolTipText    = QT_TR_NOOP("Set italic text in selected cells");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "SpreadsheetStyleItalic";
}

void CmdSpreadsheetStyleItalic::activated(int iMsg)
{
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        SpreadsheetGui::SheetView * sheetView = dynamic_cast<SpreadsheetGui::SheetView*>(activeWindow);

        if (sheetView) {
            Spreadsheet::Sheet * sheet = sheetView->getSheet();
            QModelIndexList selection = sheetView->selectedIndexes();

            if (selection.size() > 0) {
                Gui::Command::openCommand("Set italic text");
                bool allItalic = true;

                for (QModelIndexList::const_iterator it = selection.begin(); it != selection.end(); ++it) {
                    std::set<std::string> style;

                    sheet->getStyle((*it).row(), (*it).column(), style);
                    if (style.find("italic") == style.end()) {
                        allItalic = false;
                        break;
                    }
                }

                for (QModelIndexList::const_iterator it = selection.begin(); it != selection.end(); ++it) {
                    std::string address = Spreadsheet::Sheet::toAddress((*it).row(), (*it).column());
                    if (!allItalic)
                        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.setStyle('%s', 'italic', 'add')", sheet->getNameInDocument(),
                                                address.c_str());
                    else
                        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.setStyle('%s', 'italic', 'remove')", sheet->getNameInDocument(),
                                                address.c_str());
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
        if (activeWindow && dynamic_cast<SpreadsheetGui::SheetView*>(activeWindow))
            return true;

    }
    return false;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DEF_STD_CMD_A(CmdSpreadsheetStyleUnderline);

CmdSpreadsheetStyleUnderline::CmdSpreadsheetStyleUnderline()
  : Command("Spreadsheet_StyleUnderline")
{
    sAppModule      = "Spreadsheet";
    sGroup          = QT_TR_NOOP("Spreadsheet");
    sMenuText       = QT_TR_NOOP("Bold text");
    sToolTipText    = QT_TR_NOOP("Set underline text in selected cells");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "SpreadsheetStyleUnderline";
}

void CmdSpreadsheetStyleUnderline::activated(int iMsg)
{
    if (getActiveGuiDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        SpreadsheetGui::SheetView * sheetView = dynamic_cast<SpreadsheetGui::SheetView*>(activeWindow);

        if (sheetView) {
            Spreadsheet::Sheet * sheet = sheetView->getSheet();
            QModelIndexList selection = sheetView->selectedIndexes();

            if (selection.size() > 0) {
                Gui::Command::openCommand("Set underline text");
                bool allUnderline = true;

                for (QModelIndexList::const_iterator it = selection.begin(); it != selection.end(); ++it) {
                    std::set<std::string> style;

                    sheet->getStyle((*it).row(), (*it).column(), style);
                    if (style.find("underline") == style.end()) {
                        allUnderline = false;
                        break;
                    }
                }

                for (QModelIndexList::const_iterator it = selection.begin(); it != selection.end(); ++it) {
                    std::string address = Spreadsheet::Sheet::toAddress((*it).row(), (*it).column());
                    if (!allUnderline)
                        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.setStyle('%s', 'underline', 'add')", sheet->getNameInDocument(),
                                                address.c_str());
                    else
                        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.setStyle('%s', 'underline', 'remove')", sheet->getNameInDocument(),
                                                address.c_str());
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
        if (activeWindow && dynamic_cast<SpreadsheetGui::SheetView*>(activeWindow))
            return true;

    }
    return false;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DEF_STD_CMD_A(CmdCreateSpreadsheet);

CmdCreateSpreadsheet::CmdCreateSpreadsheet()
    :Command("Spreadsheet_CreateSheet")
{
    sAppModule      = "Spreadsheet";
    sGroup          = QT_TR_NOOP("Spreadsheet");
    sMenuText       = QT_TR_NOOP("Create spreadsheet");
    sToolTipText    = QT_TR_NOOP("Create a new spreadsheet");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "Spreadsheet";
}

void CmdCreateSpreadsheet::activated(int iMsg)
{
    std::string FeatName = getUniqueObjectName("Spreadsheet");

    openCommand("Create Spreadsheet");
    doCommand(Doc,"App.activeDocument().addObject('Spreadsheet::Sheet','%s\')",FeatName.c_str());
}

bool CmdCreateSpreadsheet::isActive()
{
    return App::GetApplication().getActiveDocument();
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void CreateSpreadsheetCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdCreateSpreadsheet());
    rcCmdMgr.addCommand(new CmdSpreadsheetController());

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
}

