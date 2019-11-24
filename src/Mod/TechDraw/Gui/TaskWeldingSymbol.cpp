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
#include "SymbolChooser.h"
#include "Rez.h"

#include "TaskWeldingSymbol.h"

using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;

//ctor for creation
TaskWeldingSymbol::TaskWeldingSymbol(TechDraw::DrawLeaderLine* leader) :
    ui(new Ui_TaskWeldingSymbol),
    m_leadFeat(leader),
    m_weldFeat(nullptr),
    m_arrowIn(nullptr),
    m_otherIn(nullptr),
    m_createMode(true),
    m_arrowDirty(false),
    m_otherDirty(false)
{
//    Base::Console().Message("TWS::TWS() - create mode\n");
    if  (m_leadFeat == nullptr)  {
        //should be caught in CMD caller
        Base::Console().Error("TaskWeldingSymbol - bad parameters.  Can not proceed.\n");
        return;
    }
    ui->setupUi(this);

    connect(ui->pbArrowSymbol, SIGNAL(clicked(bool)),
            this, SLOT(onArrowSymbolClicked(bool)));
    connect(ui->pbOtherSymbol, SIGNAL(clicked(bool)),
            this, SLOT(onOtherSymbolClicked(bool)));
    connect(ui->pbOtherErase, SIGNAL(clicked(bool)),
            this, SLOT(onOtherEraseClicked(bool)));

    connect(ui->fcSymbolDir, SIGNAL(fileNameSelected(const QString&)),
            this, SLOT(onDirectorySelected(const QString&)));

    connect(ui->leArrowTextL, SIGNAL(textEdited(const QString&)),
            this, SLOT(onArrowTextChanged(const QString&)));
    connect(ui->leArrowTextR, SIGNAL(textEdited(const QString&)),
            this, SLOT(onArrowTextChanged(const QString&)));
    connect(ui->leArrowTextC, SIGNAL(textEdited(const QString&)),
            this, SLOT(onArrowTextChanged(const QString&)));

    connect(ui->leOtherTextL, SIGNAL(textEdited(const QString&)),
            this, SLOT(onOtherTextChanged(const QString&)));
    connect(ui->leOtherTextR, SIGNAL(textEdited(const QString&)),
            this, SLOT(onOtherTextChanged(const QString&)));
    connect(ui->leOtherTextC, SIGNAL(textEdited(const QString&)),
            this, SLOT(onOtherTextChanged(const QString&)));


    setUiPrimary();
}

//ctor for edit
TaskWeldingSymbol::TaskWeldingSymbol(TechDraw::DrawWeldSymbol* weld) :
    ui(new Ui_TaskWeldingSymbol),
    m_leadFeat(nullptr),
    m_weldFeat(weld),
    m_arrowIn(nullptr),
    m_otherIn(nullptr),
    m_createMode(false),
    m_arrowDirty(false),
    m_otherDirty(false)
{
//    Base::Console().Message("TWS::TWS() - edit mode\n");
    if  (m_weldFeat == nullptr)  {
        //should be caught in CMD caller
        Base::Console().Error("TaskWeldingSymbol - bad parameters.  Can not proceed.\n");
        return;
    }
    
    App::DocumentObject* obj = m_weldFeat->Leader.getValue();
    if ( (obj != nullptr) &&
         (obj->isDerivedFrom(TechDraw::DrawLeaderLine::getClassTypeId())) )  {
        m_leadFeat = static_cast<TechDraw::DrawLeaderLine*>(obj);
    } else {
        Base::Console().Error("TaskWeldingSymbol - no leader for welding symbol.  Can not proceed.\n");
        return;
    }

    ui->setupUi(this);

    connect(ui->pbArrowSymbol, SIGNAL(clicked(bool)),
            this, SLOT(onArrowSymbolClicked(bool)));

    connect(ui->pbOtherSymbol, SIGNAL(clicked(bool)),
            this, SLOT(onOtherSymbolClicked(bool)));
    connect(ui->pbOtherErase, SIGNAL(clicked(bool)),
            this, SLOT(onOtherEraseClicked(bool)));

    connect(ui->fcSymbolDir, SIGNAL(fileNameSelected(const QString&)),
            this, SLOT(onDirectorySelected(const QString&)));

    connect(ui->leArrowTextL, SIGNAL(textEdited(const QString&)),
            this, SLOT(onArrowTextChanged(const QString&)));
    connect(ui->leArrowTextR, SIGNAL(textEdited(const QString&)),
            this, SLOT(onArrowTextChanged(const QString&)));
    connect(ui->leArrowTextC, SIGNAL(textEdited(const QString&)),
            this, SLOT(onArrowTextChanged(const QString&)));

    connect(ui->leOtherTextL, SIGNAL(textEdited(const QString&)),
            this, SLOT(onOtherTextChanged(const QString&)));
    connect(ui->leOtherTextR, SIGNAL(textEdited(const QString&)),
            this, SLOT(onOtherTextChanged(const QString&)));
    connect(ui->leOtherTextC, SIGNAL(textEdited(const QString&)),
            this, SLOT(onOtherTextChanged(const QString&)));

    saveState();
    setUiEdit();
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

    ui->pbArrowSymbol->setFocus();
    m_arrowOut.init();
    m_arrowPath = QString();
    m_otherOut.init();
    m_otherPath = QString();
}

void TaskWeldingSymbol::setUiEdit()
{
//    Base::Console().Message("TWS::setUiEdit()\n");
    setWindowTitle(QObject::tr("Edit Welding Symbol"));

    m_currDir = QString::fromUtf8(prefSymbolDir().c_str());  //sb path part of 1st symbol file??
    ui->fcSymbolDir->setFileName(m_currDir);

    ui->cbAllAround->setChecked(m_weldFeat->AllAround.getValue());
    ui->cbFieldWeld->setChecked(m_weldFeat->FieldWeld.getValue());
    ui->cbAltWeld->setChecked(m_weldFeat->AlternatingWeld.getValue());
    ui->leTailText->setText(QString::fromUtf8(m_weldFeat->TailText.getValue()));

    //save existing tiles done in saveState
    if (m_arrowIn != nullptr) {
        QString qTemp = QString::fromUtf8(m_arrowIn->LeftText.getValue());
        ui->leArrowTextL->setText(qTemp);
        qTemp = QString::fromUtf8(m_arrowIn->RightText.getValue());
        ui->leArrowTextR->setText(qTemp);
        qTemp = QString::fromUtf8(m_arrowIn->CenterText.getValue());
        ui->leArrowTextC->setText(qTemp);

        std::string inFile = m_arrowIn->SymbolFile.getValue();
        auto fi = Base::FileInfo(inFile);
        if (fi.isReadable()) {
            qTemp = QString::fromUtf8(m_arrowIn->SymbolFile.getValue());
            QIcon targetIcon(qTemp);
            QSize iconSize(32,32);
            ui->pbArrowSymbol->setIcon(targetIcon);
            ui->pbArrowSymbol->setIconSize(iconSize);
            ui->pbArrowSymbol->setText(QString());
        }
    }

    if (m_otherIn != nullptr) {
        QString qTemp = QString::fromUtf8(m_otherIn->LeftText.getValue());
        ui->leOtherTextL->setText(qTemp);
        qTemp = QString::fromUtf8(m_otherIn->RightText.getValue());
        ui->leOtherTextR->setText(qTemp);
        qTemp = QString::fromUtf8(m_otherIn->CenterText.getValue());
        ui->leOtherTextC->setText(qTemp);

        std::string inFile = m_otherIn->SymbolFile.getValue();
        auto fi = Base::FileInfo(inFile);
        if (fi.isReadable()) {
            qTemp = QString::fromUtf8(m_otherIn->SymbolFile.getValue());
            QIcon targetIcon(qTemp);
            QSize iconSize(32,32);
            ui->pbOtherSymbol->setIcon(targetIcon);
            ui->pbOtherSymbol->setIconSize(iconSize);
            ui->pbOtherSymbol->setText(QString());
        }
    }

    ui->pbArrowSymbol->setFocus();
}

void TaskWeldingSymbol::onArrowSymbolClicked(bool b)
{
//    Base::Console().Message("TWS::OnArrowSymbolClicked()\n");
    Q_UNUSED(b);

    QString source = QString::fromUtf8("arrow");
    SymbolChooser* dlg = new SymbolChooser(this, m_currDir, source);
    connect(dlg, SIGNAL(symbolSelected(QString, QString)),
            this, SLOT(onSymbolSelected(QString, QString)));
    dlg->setAttribute(Qt::WA_DeleteOnClose);

    //int rc = 
    dlg->exec();
}

void TaskWeldingSymbol::onOtherSymbolClicked(bool b)
{
//    Base::Console().Message("TWS::OnOtherSymbolClicked()\n");
    Q_UNUSED(b);

    QString source = QString::fromUtf8("other");
    SymbolChooser* dlg = new SymbolChooser(this, m_currDir, source);
    connect(dlg, SIGNAL(symbolSelected(QString, QString)),
            this, SLOT(onSymbolSelected(QString, QString)));
    dlg->setAttribute(Qt::WA_DeleteOnClose);

//    int rc = 
    dlg->exec();
}

void TaskWeldingSymbol::onOtherEraseClicked(bool b)
{
//    Base::Console().Message("TWS::onOtherEraseClicked()\n");
    Q_UNUSED(b);
    m_otherOut.init();

    ui->leOtherTextL->setText(QString());
    ui->leOtherTextC->setText(QString());
    ui->leOtherTextR->setText(QString());
    ui->pbOtherSymbol->setIcon(QIcon());
    ui->pbOtherSymbol->setText(QString::fromUtf8("Symbol"));

    if ( (!m_createMode) &&
         (m_otherIn != nullptr) )  { 
        m_toRemove.push_back(m_otherIn->getNameInDocument());
    }
    m_otherIn = nullptr;
}

void TaskWeldingSymbol::onArrowTextChanged(const QString& qs)
{
//    Base::Console().Message("TWS::onArrowTextChanged(%s)\n", qPrintable(qs));
    Q_UNUSED(qs);
    m_arrowDirty = true;
}

void TaskWeldingSymbol::onOtherTextChanged(const QString& qs)
{
//    Base::Console().Message("TWS::onOtherTextChanged(%s)\n", qPrintable(qs));
    Q_UNUSED(qs);
    m_otherDirty = true;
}


void TaskWeldingSymbol::onDirectorySelected(const QString& newDir)
{
//    Base::Console().Message("TWS::onDirectorySelected(%s)\n", qPrintable(newDir));
    m_currDir = newDir + QString::fromUtf8("/");
}

void TaskWeldingSymbol::onSymbolSelected(QString symbolPath,
                                         QString source)
{
//    Base::Console().Message("TWS::onSymbolSelected(%s) - source: %s\n", 
//                            qPrintable(symbolPath), qPrintable(source));
    QIcon targetIcon(symbolPath);
    QSize iconSize(32,32);
    QString arrow = QString::fromUtf8("arrow");
    QString other = QString::fromUtf8("other");
    if (source == arrow) {
        m_arrowDirty = true;
        ui->pbArrowSymbol->setIcon(targetIcon);
        ui->pbArrowSymbol->setIconSize(iconSize);
        ui->pbArrowSymbol->setText(QString());
        m_arrowPath = symbolPath;
    } else if (source == other) {
        m_otherDirty = true;
        ui->pbOtherSymbol->setIcon(targetIcon);
        ui->pbOtherSymbol->setIconSize(iconSize);
        ui->pbOtherSymbol->setText(QString());
        m_otherPath = symbolPath;
    }
}

void TaskWeldingSymbol::blockButtons(bool b)
{
    Q_UNUSED(b);
}

void TaskWeldingSymbol::saveState(void)
{
    std::vector<DrawTileWeld*> tiles = m_weldFeat->getTiles();
    for (auto t: tiles) {
        if (t->TileRow.getValue() == 0) {
            m_arrowIn = t;
        } else if (t->TileRow.getValue() == -1) {
            m_otherIn = t;
        } else {
            Base::Console().Message("TWS::saveState - bad row: %d\n", t->TileRow.getValue());
        }
    }
}

void TaskWeldingSymbol::collectArrowData(void)
{
//    Base::Console().Message("TWS::collectArrowData()\n");
    m_arrowOut.toBeSaved = true;
    m_arrowOut.arrowSide = false;
    m_arrowOut.row = 0;
    m_arrowOut.col = 0;
    m_arrowOut.leftText = Base::Tools::toStdString(ui->leArrowTextL->text());
    m_arrowOut.centerText = Base::Tools::toStdString(ui->leArrowTextC->text());
    m_arrowOut.rightText = Base::Tools::toStdString(ui->leArrowTextR->text());
    m_arrowOut.symbolPath= Base::Tools::toStdString(m_arrowPath);
    m_arrowOut.tileName = "";
}

void TaskWeldingSymbol::collectOtherData(void)
{
//    Base::Console().Message("TWS::collectOtherData()\n");
    m_otherOut.toBeSaved = true;
    m_otherOut.arrowSide = false;
    m_otherOut.row = -1;
    m_otherOut.col = 0;
    m_otherOut.leftText = Base::Tools::toStdString(ui->leOtherTextL->text());
    m_otherOut.centerText = Base::Tools::toStdString(ui->leOtherTextC->text());
    m_otherOut.rightText = Base::Tools::toStdString(ui->leOtherTextR->text());
    m_otherOut.symbolPath= Base::Tools::toStdString(m_otherPath);
    m_otherOut.tileName = "";
}

//******************************************************************************
TechDraw::DrawWeldSymbol* TaskWeldingSymbol::createWeldingSymbol(void)
{
//    Base::Console().Message("TWS::createWeldingSymbol()\n");
    
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

    bool allAround = ui->cbAllAround->isChecked();
    std::string allAroundText = allAround ? "True" : "False";
    Command::doCommand(Command::Doc,"App.activeDocument().%s.AllAround = %s",
                           symbolName.c_str(), allAroundText.c_str());

    bool fieldWeld = ui->cbFieldWeld->isChecked();
    std::string fieldWeldText = fieldWeld ? "True" : "False";
    Command::doCommand(Command::Doc,"App.activeDocument().%s.FieldWeld = %s",
                           symbolName.c_str(), fieldWeldText.c_str());

    bool altWeld = ui->cbAltWeld->isChecked();
    std::string altWeldText = altWeld ? "True" : "False";
    Command::doCommand(Command::Doc,"App.activeDocument().%s.AlternatingWeld = %s",
                           symbolName.c_str(), altWeldText.c_str());

    std::string tailText = Base::Tools::toStdString(ui->leTailText->text());
    tailText = Base::Tools::escapeEncodeString(tailText);
    Command::doCommand(Command::Doc,"App.activeDocument().%s.TailText = '%s'",
                           symbolName.c_str(), tailText.c_str());

    App::DocumentObject* newObj = m_leadFeat->getDocument()->getObject(symbolName.c_str());
    TechDraw::DrawWeldSymbol* newSym = dynamic_cast<TechDraw::DrawWeldSymbol*>(newObj);
    if ( (newObj == nullptr) ||
         (newSym == nullptr) ) {
        throw Base::RuntimeError("TaskWeldingSymbol - new symbol object not found");
    }

    return newSym;
}

void TaskWeldingSymbol::updateWeldingSymbol(void)
{
//    Base::Console().Message("TWS::updateWeldingSymbol()\n");
    std::string symbolName = m_weldFeat->getNameInDocument();

    bool allAround = ui->cbAllAround->isChecked();
    std::string allAroundText = allAround ? "True" : "False";
    Command::doCommand(Command::Doc,"App.activeDocument().%s.AllAround = %s",
                           symbolName.c_str(), allAroundText.c_str());

    bool fieldWeld = ui->cbFieldWeld->isChecked();
    std::string fieldWeldText = fieldWeld ? "True" : "False";
    Command::doCommand(Command::Doc,"App.activeDocument().%s.FieldWeld = %s",
                           symbolName.c_str(), fieldWeldText.c_str());

    bool altWeld = ui->cbAltWeld->isChecked();
    std::string altWeldText = altWeld ? "True" : "False";
    Command::doCommand(Command::Doc,"App.activeDocument().%s.AlternatingWeld = %s",
                           symbolName.c_str(), altWeldText.c_str());

    std::string tailText = Base::Tools::toStdString(ui->leTailText->text());
    tailText = Base::Tools::escapeEncodeString(tailText);
    Command::doCommand(Command::Doc,"App.activeDocument().%s.TailText = '%s'",
                           symbolName.c_str(), tailText.c_str());
}

std::vector<App::DocumentObject*> TaskWeldingSymbol::createTiles(void)
{
//    Base::Console().Message("TWS::createTiles()\n");
    std::vector<App::DocumentObject*> tileFeats;
    std::string tileType("TechDraw::DrawTileWeld");

    collectArrowData();
    if (m_arrowOut.toBeSaved) {
        std::string tileName = m_leadFeat->getDocument()->getUniqueObjectName("DrawTileWeld");
        std::string symbolPath = Base::Tools::escapeEncodeString(m_arrowOut.symbolPath);
        std::string leftText = Base::Tools::escapeEncodeString(m_arrowOut.leftText);
        std::string rightText = Base::Tools::escapeEncodeString(m_arrowOut.rightText);
        std::string centerText = Base::Tools::escapeEncodeString(m_arrowOut.centerText);
        Command::doCommand(Command::Doc,"App.activeDocument().addObject('%s','%s')",
                   tileType.c_str(),tileName.c_str());
        Command::doCommand(Command::Doc,"App.activeDocument().%s.TileRow = %d",
                       tileName.c_str(), m_arrowOut.row);
        Command::doCommand(Command::Doc,"App.activeDocument().%s.TileColumn = %d",
                       tileName.c_str(), m_arrowOut.col);
        Command::doCommand(Command::Doc,"App.activeDocument().%s.SymbolFile = '%s'",
                       tileName.c_str(), symbolPath.c_str());
        Command::doCommand(Command::Doc,"App.activeDocument().%s.LeftText = '%s'",
                       tileName.c_str(), leftText.c_str());
        Command::doCommand(Command::Doc,"App.activeDocument().%s.RightText = '%s'",
                       tileName.c_str(), rightText.c_str());
        Command::doCommand(Command::Doc,"App.activeDocument().%s.CenterText = '%s'",
                       tileName.c_str(), centerText.c_str());
        App::DocumentObject* newTile = m_leadFeat->getDocument()->getObject(tileName.c_str());
        if (newTile == nullptr) {
            throw Base::RuntimeError("TaskWeldingSymbol - new tile object not found");
        }
        tileFeats.push_back(newTile);
    }

    if (m_otherDirty) {
        collectOtherData();
        if (m_otherOut.toBeSaved) {
            std::string tileName = m_leadFeat->getDocument()->getUniqueObjectName("DrawTileWeld");
            Command::doCommand(Command::Doc,"App.activeDocument().addObject('%s','%s')",
                       tileType.c_str(),tileName.c_str());
            Command::doCommand(Command::Doc,"App.activeDocument().%s.TileRow = %d",
                           tileName.c_str(), m_otherOut.row);
            Command::doCommand(Command::Doc,"App.activeDocument().%s.TileColumn = %d",
                           tileName.c_str(), m_otherOut.col);

            if (m_otherOut.symbolPath.empty()) {
                Command::doCommand(Command::Doc,"App.activeDocument().%s.SymbolFile = ''",
                           tileName.c_str());
            } else {
                std::string symbolPath = Base::Tools::escapeEncodeString(m_otherOut.symbolPath);
                Command::doCommand(Command::Doc,"App.activeDocument().%s.SymbolFile = '%s'",
                           tileName.c_str(), symbolPath.c_str());
            }

            std::string leftText = Base::Tools::escapeEncodeString(m_otherOut.leftText);
            std::string rightText = Base::Tools::escapeEncodeString(m_otherOut.rightText);
            std::string centerText = Base::Tools::escapeEncodeString(m_otherOut.centerText);
            Command::doCommand(Command::Doc,"App.activeDocument().%s.LeftText = '%s'",
                           tileName.c_str(), leftText.c_str());
            Command::doCommand(Command::Doc,"App.activeDocument().%s.RightText = '%s'",
                           tileName.c_str(), rightText.c_str());
            Command::doCommand(Command::Doc,"App.activeDocument().%s.CenterText = '%s'",
                           tileName.c_str(), centerText.c_str());
            App::DocumentObject* newTile = m_leadFeat->getDocument()->getObject(tileName.c_str());
            if (newTile == nullptr) {
                throw Base::RuntimeError("TaskWeldingSymbol - new tile object not found");
            }
            tileFeats.push_back(newTile);
        }
    }

    return tileFeats;
}

std::vector<App::DocumentObject*> TaskWeldingSymbol::updateTiles(void)
{
//    Base::Console().Message("TWS::updateTiles()\n");
    std::vector<App::DocumentObject*> tileFeats;
    std::string tileType("TechDraw::DrawTileWeld");
    std::string tileName;

    collectArrowData();

    if (m_arrowIn != nullptr) {
        tileName = m_arrowIn->getNameInDocument();
    }
    if (m_arrowIn == nullptr) {   // this should never happen on an update!
        tileName = m_leadFeat->getDocument()->getUniqueObjectName("DrawTileWeld");
        Command::doCommand(Command::Doc,"App.activeDocument().addObject('%s','%s')",
                   tileType.c_str(),tileName.c_str());
        App::DocumentObject* newTile = m_leadFeat->getDocument()->getObject(tileName.c_str());
        if (newTile == nullptr) {
            throw Base::RuntimeError("TaskWeldingSymbol - new tile object not found");
        }
        tileFeats.push_back(newTile);
    }

    if (m_arrowOut.toBeSaved) {
        Command::doCommand(Command::Doc,"App.activeDocument().%s.TileRow = %d",
                       tileName.c_str(), m_arrowOut.row);
        Command::doCommand(Command::Doc,"App.activeDocument().%s.TileColumn = %d",
                       tileName.c_str(), m_arrowOut.col);

        if (m_otherOut.symbolPath.empty()) {
            Command::doCommand(Command::Doc,"App.activeDocument().%s.SymbolFile = ''",
                       tileName.c_str());
        } else {
            std::string symbolPath = Base::Tools::escapeEncodeString(m_arrowOut.symbolPath);
            Command::doCommand(Command::Doc,"App.activeDocument().%s.SymbolFile = '%s'",
                       tileName.c_str(), symbolPath.c_str());
        }

        std::string leftText = Base::Tools::escapeEncodeString(m_arrowOut.leftText);
        std::string rightText = Base::Tools::escapeEncodeString(m_arrowOut.rightText);
        std::string centerText = Base::Tools::escapeEncodeString(m_arrowOut.centerText);
        Command::doCommand(Command::Doc,"App.activeDocument().%s.LeftText = '%s'",
                       tileName.c_str(), leftText.c_str());
        Command::doCommand(Command::Doc,"App.activeDocument().%s.RightText = '%s'",
                       tileName.c_str(), rightText.c_str());
        Command::doCommand(Command::Doc,"App.activeDocument().%s.CenterText = '%s'",
                       tileName.c_str(), centerText.c_str());
    }

    if (m_otherDirty) {
        collectOtherData();

        if (m_otherIn != nullptr) {
            tileName = m_otherIn->getNameInDocument();
        }

        if ( (m_otherIn == nullptr) &&    //m_otherIn can be nullptr if otherside added in edit session.
             (m_otherOut.toBeSaved) ) {
            tileName = m_leadFeat->getDocument()->getUniqueObjectName("DrawTileWeld");
            Command::doCommand(Command::Doc,"App.activeDocument().addObject('%s','%s')",
                       tileType.c_str(),tileName.c_str());
            App::DocumentObject* newTile = m_leadFeat->getDocument()->getObject(tileName.c_str());
            if (newTile == nullptr) {
                throw Base::RuntimeError("TaskWeldingSymbol - new tile object not found");
            }
            tileFeats.push_back(newTile);
        }

        if (m_otherOut.toBeSaved) {
            Command::doCommand(Command::Doc,"App.activeDocument().addObject('%s','%s')",
                       tileType.c_str(),tileName.c_str());
            Command::doCommand(Command::Doc,"App.activeDocument().%s.TileRow = %d",
                           tileName.c_str(), m_otherOut.row);
            Command::doCommand(Command::Doc,"App.activeDocument().%s.TileColumn = %d",
                           tileName.c_str(), m_otherOut.col);

            if (m_otherOut.symbolPath.empty()) {
                Command::doCommand(Command::Doc,"App.activeDocument().%s.SymbolFile = ''",
                           tileName.c_str());
            } else {
                std::string symbolPath = Base::Tools::escapeEncodeString(m_otherOut.symbolPath);
                Command::doCommand(Command::Doc,"App.activeDocument().%s.SymbolFile = '%s'",
                           tileName.c_str(), symbolPath.c_str());
            }

            std::string leftText = Base::Tools::escapeEncodeString(m_otherOut.leftText);
            std::string rightText = Base::Tools::escapeEncodeString(m_otherOut.rightText);
            std::string centerText = Base::Tools::escapeEncodeString(m_otherOut.centerText);
            Command::doCommand(Command::Doc,"App.activeDocument().%s.LeftText = '%s'",
                           tileName.c_str(), leftText.c_str());
            Command::doCommand(Command::Doc,"App.activeDocument().%s.RightText = '%s'",
                           tileName.c_str(), rightText.c_str());
            Command::doCommand(Command::Doc,"App.activeDocument().%s.CenterText = '%s'",
                           tileName.c_str(), centerText.c_str());
        }
    }

    return tileFeats;
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
    if (m_createMode) {
        Gui::Command::openCommand("Create WeldSymbol");
        m_weldFeat = createWeldingSymbol();
        std::vector<App::DocumentObject*> tileFeats = createTiles();
        for (auto& obj: tileFeats) {
            TechDraw::DrawTileWeld* tile = dynamic_cast<TechDraw::DrawTileWeld*>(obj);
            tile->TileParent.setValue(m_weldFeat);
        }
        Gui::Command::updateActive();
        Gui::Command::commitCommand();
        m_weldFeat->recomputeFeature();
    //    m_weldFeat->requestPaint();    //not a dv!
    } else {
        Gui::Command::openCommand("Edit WeldSymbol");
        try {
            updateWeldingSymbol();
            std::vector<App::DocumentObject*> tileFeats = updateTiles();
            for (auto& obj: tileFeats) {  //new tiles only
                TechDraw::DrawTileWeld* tile = dynamic_cast<TechDraw::DrawTileWeld*>(obj);
                tile->TileParent.setValue(m_weldFeat);
            }

            for (auto name: m_toRemove) {
                //QGIV is removed from scene by MDIVP/QGVP on objectDelete
                Command::doCommand(Command::Doc,
                     "App.activeDocument().removeObject('%s')", name.c_str());
            }
        }

        catch (...) {
            Base::Console().Error("TWS::accept - failed to update symbol\n");
        }

        Gui::Command::updateActive();
        Gui::Command::commitCommand();
        m_weldFeat->recomputeFeature();
    //    m_weldFeat->requestPaint();    //not a dv!
    }
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

TaskDlgWeldingSymbol::TaskDlgWeldingSymbol(TechDraw::DrawWeldSymbol* weld)
    : TaskDialog()
{
    widget  = new TaskWeldingSymbol(weld);
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
