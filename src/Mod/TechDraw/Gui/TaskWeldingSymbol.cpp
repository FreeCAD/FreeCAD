/***************************************************************************
 *   Copyright (c) 2019 Wandererfan <wandererfan@gmail.com                 *
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
#include <cmath>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>

#endif // #ifndef _PreComp_

#include <QApplication>
#include <QStatusBar>
#include <QGraphicsScene>

#include <Base/Console.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>

#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawLeaderLine.h>
#include <Mod/TechDraw/App/DrawWeldSymbol.h>
#include <Mod/TechDraw/App/DrawTile.h>
#include <Mod/TechDraw/App/DrawTileWeld.h>
#include <Mod/TechDraw/App/Geometry.h>
#include <Mod/TechDraw/App/Cosmetic.h>

#include <Mod/TechDraw/Gui/ui_TaskWeldingSymbol.h>

#include "DrawGuiStd.h"
#include "QGVPage.h"
#include "QGIView.h"
#include "QGIPrimPath.h"
#include "QGILeaderLine.h"
#include "MDIViewPage.h"
#include "ViewProviderPage.h"
#include "ViewProviderViewPart.h"
#include "Rez.h"

#include "TaskWeldingSymbol.h"

using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;

//ctor for creation
TaskWeldingSymbol::TaskWeldingSymbol(TechDraw::DrawLeaderLine* leader) :
    ui(new Ui_TaskWeldingSymbol),
    m_leadFeat(leader),
    m_arrowCount(0),
    m_otherCount(0)
{
//    Base::Console().Message("TWS::TWS() - create mode\n");
    if  (m_leadFeat == nullptr)  {
        //should be caught in CMD caller
        Base::Console().Error("TaskWeldingSymbol - bad parameters.  Can not proceed.\n");
        return;
    }

    ui->setupUi(this);
    connect(ui->pbArrow0, SIGNAL(clicked(bool)),
            this, SLOT(onArrow0Clicked(bool)));
    connect(ui->pbArrow1, SIGNAL(clicked(bool)),
            this, SLOT(onArrow1Clicked(bool)));
    connect(ui->pbOther0, SIGNAL(clicked(bool)),
            this, SLOT(onOther0Clicked(bool)));
    connect(ui->pbOther1, SIGNAL(clicked(bool)),
            this, SLOT(onOther1Clicked(bool)));
    connect(ui->fcSymbolDir, SIGNAL(fileNameSelected(const QString&)),
            this, SLOT(onDirectorySelected(const QString&)));

    setUiPrimary();
}

TaskWeldingSymbol::~TaskWeldingSymbol()
{
    delete ui;
}

void TaskWeldingSymbol::updateTask()
{
//    blockUpdate = true;

//    blockUpdate = false;
}

void TaskWeldingSymbol::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

void TaskWeldingSymbol::setUiPrimary()
{
//    Base::Console().Message("TWS::setUiPrimary()\n");
    setWindowTitle(QObject::tr("Create Welding Symbol"));
    m_currDir = QString::fromUtf8(prefSymbolDir().c_str());
    ui->fcSymbolDir->setFileName(m_currDir);
    loadSymbolNames(m_currDir);

    ui->lwSymbols->setViewMode(QListView::IconMode);
    ui->lwSymbols->setFlow(QListView::LeftToRight);
    ui->lwSymbols->setWrapping(true);
    ui->lwSymbols->setDragEnabled(true);
    ui->lwSymbols->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->lwSymbols->setAcceptDrops(false);
}

void TaskWeldingSymbol::setUiEdit()
{
//    Base::Console().Message("TWS::setUiEdit()\n");
    setWindowTitle(QObject::tr("Edit Welding Symbol"));
}

void TaskWeldingSymbol::onArrow0Clicked(bool b)
{
//    Base::Console().Message("TWS::OnArrow0Clicked()\n");
    Q_UNUSED(b);
    Qt::KeyboardModifiers km = QApplication::keyboardModifiers();
    if (km & Qt::ControlModifier) {
        ui->pbArrow0->setText(QString::fromUtf8("Add"));
        ui->pbArrow0->setIcon(QIcon());
        removePendingTile(0,0);
        return;
    }
    
    QListWidgetItem* sourceItem = ui->lwSymbols->currentItem();
    QString targetText = sourceItem->text();
    TechDrawGui::Tile2Add newTile;

    QString iconPath = m_currDir + 
                       targetText +
                       QString::fromUtf8(".svg") ;

    QIcon targetIcon(iconPath);
    QSize iconSize(32,32);
    ui->pbArrow0->setIcon(targetIcon);
    ui->pbArrow0->setIconSize(iconSize);
    ui->pbArrow0->setText(QString());
    
    newTile.arrowSide = true;
    newTile.symbolPath = Base::Tools::toStdString(iconPath);
    newTile.leftText = Base::Tools::toStdString(ui->leLeftText->text());
    newTile.centerText = Base::Tools::toStdString(ui->leCenterText->text());
    newTile.rightText = Base::Tools::toStdString(ui->leRightText->text());
    newTile.row = 0;
    newTile.col = 0;
    m_tiles2Add.push_back(newTile);
    m_arrowCount++;
}

void TaskWeldingSymbol::onArrow1Clicked(bool b)
{
//    Base::Console().Message("TWS::OnArrow1Clicked()\n");
    Q_UNUSED(b);
    Qt::KeyboardModifiers km = QApplication::keyboardModifiers();
    if (km & Qt::ControlModifier) {
        ui->pbArrow1->setText(QString::fromUtf8("Add"));
        ui->pbArrow1->setIcon(QIcon());
        removePendingTile(0,1);
        return;
    }

    QListWidgetItem* sourceItem = ui->lwSymbols->currentItem();
    QString targetText = sourceItem->text();
    TechDrawGui::Tile2Add newTile;

    QString iconPath = m_currDir + 
                       targetText +
                       QString::fromUtf8(".svg") ;

    QIcon targetIcon(iconPath);
    QSize iconSize(32,32);
    ui->pbArrow1->setIcon(targetIcon);
    ui->pbArrow1->setIconSize(iconSize);
    ui->pbArrow1->setText(QString());

    newTile.arrowSide = true;
    newTile.symbolPath = Base::Tools::toStdString(iconPath);
    newTile.leftText = Base::Tools::toStdString(ui->leLeftText->text());
    newTile.centerText = Base::Tools::toStdString(ui->leCenterText->text());
    newTile.rightText = Base::Tools::toStdString(ui->leRightText->text());
    newTile.row = 0;
    newTile.col = 1;
    m_tiles2Add.push_back(newTile);
    m_arrowCount++;
}

void TaskWeldingSymbol::onOther0Clicked(bool b)
{
//    Base::Console().Message("TWS::onOther0Clicked()\n");
    Q_UNUSED(b);
    Qt::KeyboardModifiers km = QApplication::keyboardModifiers();
    if (km & Qt::ControlModifier) {
        ui->pbOther0->setText(QString::fromUtf8("Add"));
        ui->pbOther0->setIcon(QIcon());
        removePendingTile(-1,0);
        return;
    }

    QListWidgetItem* sourceItem = ui->lwSymbols->currentItem();
    QString targetText = sourceItem->text();
    TechDrawGui::Tile2Add newTile;

    QString iconPath = m_currDir + 
                       targetText +
                       QString::fromUtf8(".svg") ;

    QIcon targetIcon(iconPath);
    QSize iconSize(32,32);
    ui->pbOther0->setIcon(targetIcon);
    ui->pbOther0->setIconSize(iconSize);
    ui->pbOther0->setText(QString());

    newTile.arrowSide = false;
    newTile.symbolPath = Base::Tools::toStdString(iconPath);
    newTile.leftText = Base::Tools::toStdString(ui->leLeftText->text());
    newTile.centerText = Base::Tools::toStdString(ui->leCenterText->text());
    newTile.rightText = Base::Tools::toStdString(ui->leRightText->text());
    newTile.row = -1;
    newTile.col = 0;
    m_tiles2Add.push_back(newTile);
    m_otherCount++;
}

void TaskWeldingSymbol::onOther1Clicked(bool b)
{
//    Base::Console().Message("TWS::onOther1Clicked()\n");
    Q_UNUSED(b);
    Qt::KeyboardModifiers km = QApplication::keyboardModifiers();
    if (km & Qt::ControlModifier) {
        ui->pbOther1->setText(QString::fromUtf8("Add"));
        ui->pbOther1->setIcon(QIcon());
        removePendingTile(-1,1);
        return;
    }
    
    QListWidgetItem* sourceItem = ui->lwSymbols->currentItem();
    QString targetText = sourceItem->text();
    TechDrawGui::Tile2Add newTile;

    QString iconPath = m_currDir + 
                       targetText +
                       QString::fromUtf8(".svg") ;

    QIcon targetIcon(iconPath);
    QSize iconSize(32,32);
    ui->pbOther1->setIcon(targetIcon);
    ui->pbOther1->setIconSize(iconSize);
    ui->pbOther1->setText(QString());

    newTile.arrowSide = false;
    newTile.symbolPath = Base::Tools::toStdString(iconPath);
    newTile.leftText = Base::Tools::toStdString(ui->leLeftText->text());
    newTile.centerText = Base::Tools::toStdString(ui->leCenterText->text());
    newTile.rightText = Base::Tools::toStdString(ui->leRightText->text());
    newTile.row = -1;
    newTile.col = 1;
    m_tiles2Add.push_back(newTile);
    m_otherCount++;
}

void TaskWeldingSymbol::onDirectorySelected(const QString& newDir)
{
//    Base::Console().Message("TWS::onDirectorySelected(%s)\n", qPrintable(newDir));
    m_currDir = newDir + QString::fromUtf8("/");
    loadSymbolNames(m_currDir);
}

void TaskWeldingSymbol::removePendingTile(int row, int col)
{
//    Base::Console().Message("TWS::removePendingIcon(%d, %d) - tiles in: %d\n",
//                            row, col, m_tiles2Add.size());
    std::vector<Tile2Add> newList;
    for (auto& t: m_tiles2Add) {
        if ((t.row == row) &&
            (t.col == col) ) {
            continue;
        } else {
            newList.push_back(t);
        }
    }
    m_tiles2Add = newList;
}


void TaskWeldingSymbol::blockButtons(bool b)
{
    Q_UNUSED(b);
}

void TaskWeldingSymbol::loadSymbolNames(QString pathToSymbols)
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

//******************************************************************************
App::DocumentObject* TaskWeldingSymbol::createWeldingSymbol(void)
{
//    Base::Console().Message("TWS::createWeldingSymbol()\n");
    Gui::Command::openCommand("Create WeldSymbol");
    
    std::string symbolName = m_leadFeat->getDocument()->getUniqueObjectName("DrawWeldSymbol");
    std::string symbolType = "TechDraw::DrawWeldSymbol";

    TechDraw::DrawPage* page = m_leadFeat->findParentPage();
    std::string pageName = page->getNameInDocument();

    Command::doCommand(Command::Doc,"App.activeDocument().addObject('%s','%s')",
                       symbolType.c_str(),symbolName.c_str());
    Command::doCommand(Command::Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",
                       pageName.c_str(), symbolName.c_str());
    Command::doCommand(Command::Doc,"App.activeDocument().%s.Leader = App.activeDocument().%s",
                           symbolName.c_str(),m_leadFeat->getNameInDocument());

    bool allAround = ui->rbAllAround->isChecked();
    std::string allAroundText = allAround ? "True" : "False";
    Command::doCommand(Command::Doc,"App.activeDocument().%s.AllAround = %s",
                           symbolName.c_str(), allAroundText.c_str());

    bool fieldWeld = ui->rbFieldWeld->isChecked();
    std::string fieldWeldText = fieldWeld ? "True" : "False";
    Command::doCommand(Command::Doc,"App.activeDocument().%s.FieldWeld = %s",
                           symbolName.c_str(), fieldWeldText.c_str());

    std::string tailText = Base::Tools::toStdString(ui->leProcessText->text());
    Command::doCommand(Command::Doc,"App.activeDocument().%s.TailText = '%s'",
                           symbolName.c_str(), tailText.c_str());

    App::DocumentObject* newObj = m_leadFeat->getDocument()->getObject(symbolName.c_str());
    if (newObj == nullptr) {
        throw Base::RuntimeError("TaskWeldingSymbol - new symbol object not found");
    }
    newObj->recomputeFeature();
    Gui::Command::updateActive();
    Gui::Command::commitCommand();
    return newObj;
}

std::vector<App::DocumentObject*> TaskWeldingSymbol::createTiles(void)
{
//    Base::Console().Message("TWS::createTiles()\n");
    Gui::Command::openCommand("Create Welding Tiles");
    std::vector<App::DocumentObject*> tileFeats;
    std::string tileType("TechDraw::DrawTileWeld");
    for (auto& t: m_tiles2Add) {
        std::string tileName = m_leadFeat->getDocument()->getUniqueObjectName("DrawTileWeld");
        Command::doCommand(Command::Doc,"App.activeDocument().addObject('%s','%s')",
                   tileType.c_str(),tileName.c_str());
        Command::doCommand(Command::Doc,"App.activeDocument().%s.TileRow = %d",
                       tileName.c_str(), t.row);
        Command::doCommand(Command::Doc,"App.activeDocument().%s.TileColumn = %d",
                       tileName.c_str(), t.col);
        Command::doCommand(Command::Doc,"App.activeDocument().%s.SymbolFile = '%s'",
                       tileName.c_str(), t.symbolPath.c_str());
        Command::doCommand(Command::Doc,"App.activeDocument().%s.LeftText = '%s'",
                       tileName.c_str(), t.leftText.c_str());
        Command::doCommand(Command::Doc,"App.activeDocument().%s.RightText = '%s'",
                       tileName.c_str(), t.rightText.c_str());
        Command::doCommand(Command::Doc,"App.activeDocument().%s.CenterText = '%s'",
                       tileName.c_str(), t.centerText.c_str());

        App::DocumentObject* newTile = m_leadFeat->getDocument()->getObject(tileName.c_str());
        if (newTile == nullptr) {
            throw Base::RuntimeError("TaskWeldingSymbol - new tile object not found");
        }
        tileFeats.push_back(newTile);
    }

    Gui::Command::updateActive();
    Gui::Command::commitCommand();
    return tileFeats;
}

void TaskWeldingSymbol::updateWeldingSymbol(void)
{
//    Base::Console().Message("TWS::updateWeldingSymbol()\n");
    Gui::Command::openCommand("Edit WeldingSymbol");
    m_weldFeat->requestPaint();

    Gui::Command::updateActive();
    Gui::Command::commitCommand();
}

void TaskWeldingSymbol::saveButtons(QPushButton* btnOK,
                             QPushButton* btnCancel)
{
    m_btnOK = btnOK;
    m_btnCancel = btnCancel;
}

void TaskWeldingSymbol::enableTaskButtons(bool b)
{
    m_btnOK->setEnabled(b);
    m_btnCancel->setEnabled(b);
}

std::string TaskWeldingSymbol::prefSymbolDir()
{
    std::string defaultDir = App::Application::getResourceDir() + "Mod/TechDraw/Symbols/Welding/AWS/";
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
                                         GetGroup("Preferences")->GetGroup("Mod/TechDraw/Files");
                                    
    std::string symbolDir = hGrp->GetASCII("WeldingDir", defaultDir.c_str());
    return symbolDir;
}

//******************************************************************************

bool TaskWeldingSymbol::accept()
{
//    Base::Console().Message("TWS::accept()\n");
    std::vector<App::DocumentObject*> tileFeats = createTiles();
    App::DocumentObject* weldFeat = createWeldingSymbol();
    for (auto& obj: tileFeats) {
        TechDraw::DrawTileWeld* tile = dynamic_cast<TechDraw::DrawTileWeld*>(obj);
        tile->TileParent.setValue(weldFeat);
    }
    weldFeat->recomputeFeature();
//    weldFeat->requestPaint();    //not a dv!
    
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");

    return true;
}

bool TaskWeldingSymbol::reject()
{
//    Base::Console().Message("TWS::reject()\n");
      //nothing to remove. 

    Gui::Command::doCommand(Gui::Command::Gui,"App.activeDocument().recompute()");
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");

    return false;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TaskDlgWeldingSymbol::TaskDlgWeldingSymbol(TechDraw::DrawLeaderLine* leader)
    : TaskDialog()
{
    widget  = new TaskWeldingSymbol(leader);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/techdraw-weldsymbol"),
                                             widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgWeldingSymbol::~TaskDlgWeldingSymbol()
{
}

void TaskDlgWeldingSymbol::update()
{
//    widget->updateTask();
}

void TaskDlgWeldingSymbol::modifyStandardButtons(QDialogButtonBox* box)
{
    QPushButton* btnOK = box->button(QDialogButtonBox::Ok);
    QPushButton* btnCancel = box->button(QDialogButtonBox::Cancel);
    widget->saveButtons(btnOK, btnCancel);
}

//==== calls from the TaskView ===============================================================
void TaskDlgWeldingSymbol::open()
{
}

void TaskDlgWeldingSymbol::clicked(int)
{
}

bool TaskDlgWeldingSymbol::accept()
{
    widget->accept();
    return true;
}

bool TaskDlgWeldingSymbol::reject()
{
    widget->reject();
    return true;
}

#include <Mod/TechDraw/Gui/moc_TaskWeldingSymbol.cpp>
