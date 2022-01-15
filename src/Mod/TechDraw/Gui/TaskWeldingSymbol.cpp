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
//#include <Mod/TechDraw/App/Preferences.h>

#include <Mod/TechDraw/Gui/ui_TaskWeldingSymbol.h>

#include "DrawGuiStd.h"
#include "PreferencesGui.h"
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
    m_arrowFeat(nullptr),
    m_otherFeat(nullptr),
    m_btnOK(nullptr),
    m_btnCancel(nullptr),
    m_createMode(true),
    m_otherDirty(false)
{
//TODO: why does DWS need DLL as parent?

    //existence of leader is guaranteed by CmdTechDrawWeldSymbol (CommandAnnotate.cpp)
    ui->setupUi(this);

    setUiPrimary();

    connect(ui->pbArrowSymbol, SIGNAL(clicked(bool)),
            this, SLOT(onArrowSymbolCreateClicked()));
    connect(ui->pbOtherSymbol, SIGNAL(clicked(bool)),
            this, SLOT(onOtherSymbolCreateClicked()));
    connect(ui->pbOtherErase, SIGNAL(clicked(bool)),
            this, SLOT(onOtherEraseCreateClicked()));
    connect(ui->pbFlipSides, SIGNAL(clicked(bool)),
            this, SLOT(onFlipSidesCreateClicked()));
    connect(ui->fcSymbolDir, SIGNAL(fileNameSelected(const QString&)),
            this, SLOT(onDirectorySelected(const QString&)));
}

//ctor for edit
TaskWeldingSymbol::TaskWeldingSymbol(TechDraw::DrawWeldSymbol* weld) :
    ui(new Ui_TaskWeldingSymbol),
    m_leadFeat(nullptr),
    m_weldFeat(weld),
    m_arrowFeat(nullptr),
    m_otherFeat(nullptr),
    m_btnOK(nullptr),
    m_btnCancel(nullptr),
    m_createMode(false),
    m_otherDirty(false)
{
    //existence of weld is guaranteed by CmdTechDrawWeldSymbol (CommandAnnotate.cpp)
    //                                or ViewProviderWeld.setEdit

    App::DocumentObject* obj = m_weldFeat->Leader.getValue();
    if ( (obj != nullptr) &&
         (obj->isDerivedFrom(TechDraw::DrawLeaderLine::getClassTypeId())) )  {
        m_leadFeat = static_cast<TechDraw::DrawLeaderLine*>(obj);
    } else {
        Base::Console().Error("TaskWeldingSymbol - no leader for welding symbol.  Can not proceed.\n");
        return;
    }

    ui->setupUi(this);

    setUiEdit();

    connect(ui->pbArrowSymbol, SIGNAL(clicked(bool)),
        this, SLOT(onArrowSymbolClicked()));
    connect(ui->pbOtherSymbol, SIGNAL(clicked(bool)),
        this, SLOT(onOtherSymbolClicked()));
    connect(ui->pbOtherErase, SIGNAL(clicked(bool)),
        this, SLOT(onOtherEraseClicked()));
    connect(ui->pbFlipSides, SIGNAL(clicked(bool)),
        this, SLOT(onFlipSidesClicked()));

    connect(ui->fcSymbolDir, SIGNAL(fileNameSelected(const QString&)),
        this, SLOT(onDirectorySelected(const QString&)));

    connect(ui->leArrowTextL, SIGNAL(textEdited(QString)),
        this, SLOT(onArrowTextChanged()));
    connect(ui->leArrowTextR, SIGNAL(textEdited(QString)),
        this, SLOT(onArrowTextChanged()));
    connect(ui->leArrowTextC, SIGNAL(textEdited(QString)),
        this, SLOT(onArrowTextChanged()));

    connect(ui->leOtherTextL, SIGNAL(textEdited(QString)),
        this, SLOT(onOtherTextChanged()));
    connect(ui->leOtherTextR, SIGNAL(textEdited(QString)),
        this, SLOT(onOtherTextChanged()));
    connect(ui->leOtherTextC, SIGNAL(textEdited(QString)),
        this, SLOT(onOtherTextChanged()));

    connect(ui->leTailText, SIGNAL(textEdited(QString)),
        this, SLOT(onWeldingChanged()));
    connect(ui->cbFieldWeld, SIGNAL(toggled(bool)),
        this, SLOT(onWeldingChanged()));
    connect(ui->cbAllAround, SIGNAL(toggled(bool)),
        this, SLOT(onWeldingChanged()));
    connect(ui->cbAltWeld, SIGNAL(toggled(bool)),
        this, SLOT(onWeldingChanged()));
}

TaskWeldingSymbol::~TaskWeldingSymbol()
{
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
    m_currDir = PreferencesGui::weldingDirectory();
    ui->fcSymbolDir->setFileName(m_currDir);

    ui->pbArrowSymbol->setFocus();
    m_arrowOut.init();
    m_arrowPath = QString();
    m_arrowSymbol = QString();
    m_otherOut.init();
    m_otherPath = QString();
    m_otherSymbol = QString();

    // we must mark the other side dirty to assure it gets created
    m_otherDirty = true;
}

void TaskWeldingSymbol::setUiEdit()
{
//    Base::Console().Message("TWS::setUiEdit()\n");
    setWindowTitle(QObject::tr("Edit Welding Symbol"));

    m_currDir = PreferencesGui::weldingDirectory();
    ui->fcSymbolDir->setFileName(m_currDir);

    ui->cbAllAround->setChecked(m_weldFeat->AllAround.getValue());
    ui->cbFieldWeld->setChecked(m_weldFeat->FieldWeld.getValue());
    ui->cbAltWeld->setChecked(m_weldFeat->AlternatingWeld.getValue());
    ui->leTailText->setText(QString::fromUtf8(m_weldFeat->TailText.getValue()));

    getTileFeats();
    if (m_arrowFeat != nullptr) {
        QString qTemp = QString::fromUtf8(m_arrowFeat->LeftText.getValue());
        ui->leArrowTextL->setText(qTemp);
        qTemp = QString::fromUtf8(m_arrowFeat->RightText.getValue());
        ui->leArrowTextR->setText(qTemp);
        qTemp = QString::fromUtf8(m_arrowFeat->CenterText.getValue());
        ui->leArrowTextC->setText(qTemp);

        std::string inFile = m_arrowFeat->SymbolFile.getValue();
        auto fi = Base::FileInfo(inFile);
        if (fi.isReadable()) {
            qTemp = QString::fromUtf8(m_arrowFeat->SymbolFile.getValue());
            QIcon targetIcon(qTemp);
            QSize iconSize(32, 32);
            ui->pbArrowSymbol->setIcon(targetIcon);
            ui->pbArrowSymbol->setIconSize(iconSize);
            ui->pbArrowSymbol->setText(QString());
        } else {
            ui->pbArrowSymbol->setText(tr("Symbol"));
        }
    }

    if (m_otherFeat != nullptr) {
        QString qTemp = QString::fromUtf8(m_otherFeat->LeftText.getValue());
        ui->leOtherTextL->setText(qTemp);
        qTemp = QString::fromUtf8(m_otherFeat->RightText.getValue());
        ui->leOtherTextR->setText(qTemp);
        qTemp = QString::fromUtf8(m_otherFeat->CenterText.getValue());
        ui->leOtherTextC->setText(qTemp);

        std::string inFile = m_otherFeat->SymbolFile.getValue();
        auto fi = Base::FileInfo(inFile);
        if (fi.isReadable()) {
            qTemp = QString::fromUtf8(m_otherFeat->SymbolFile.getValue());
            QIcon targetIcon(qTemp);
            QSize iconSize(32, 32);
            ui->pbOtherSymbol->setIcon(targetIcon);
            ui->pbOtherSymbol->setIconSize(iconSize);
            ui->pbOtherSymbol->setText(QString());
        } else {
            ui->pbOtherSymbol->setText(tr("Symbol"));
        }
    }

    ui->pbArrowSymbol->setFocus();
}

void TaskWeldingSymbol::onArrowSymbolCreateClicked()
{
    QString source = tr("arrow");
    SymbolChooser* dlg = new SymbolChooser(this, m_currDir, source);
    connect(dlg, SIGNAL(symbolSelected(QString, QString)),
        this, SLOT(onSymbolSelected(QString, QString)));
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->exec();
}

void TaskWeldingSymbol::onArrowSymbolClicked()
{
    QString source = tr("arrow");
    SymbolChooser* dlg = new SymbolChooser(this, m_currDir, source);
    connect(dlg, SIGNAL(symbolSelected(QString, QString)),
            this, SLOT(onSymbolSelected(QString, QString)));
    dlg->setAttribute(Qt::WA_DeleteOnClose);

    dlg->exec();
    updateTiles();
    m_weldFeat->requestPaint();
}

void TaskWeldingSymbol::onOtherSymbolCreateClicked()
{
    QString source = tr("other");
    SymbolChooser* dlg = new SymbolChooser(this, m_currDir, source);
    connect(dlg, SIGNAL(symbolSelected(QString, QString)),
        this, SLOT(onSymbolSelected(QString, QString)));
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->exec();
}

void TaskWeldingSymbol::onOtherSymbolClicked()
{
    QString source = tr("other");
    SymbolChooser* dlg = new SymbolChooser(this, m_currDir, source);
    connect(dlg, SIGNAL(symbolSelected(QString, QString)),
            this, SLOT(onSymbolSelected(QString, QString)));
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->exec();
    updateTiles();
    m_weldFeat->requestPaint();
}

void TaskWeldingSymbol::onOtherEraseCreateClicked()
{
    ui->leOtherTextL->setText(QString());
    ui->leOtherTextC->setText(QString());
    ui->leOtherTextR->setText(QString());
    ui->pbOtherSymbol->setIcon(QIcon());
    ui->pbOtherSymbol->setText(tr("Symbol"));
    m_otherOut.init();
    m_otherPath = QString();
}

void TaskWeldingSymbol::onOtherEraseClicked()
{
    m_otherDirty = true;
    ui->leOtherTextL->setText(QString());
    ui->leOtherTextC->setText(QString());
    ui->leOtherTextR->setText(QString());
    ui->pbOtherSymbol->setIcon(QIcon());
    ui->pbOtherSymbol->setText(tr("Symbol"));
    m_otherOut.init();
    m_otherPath = QString();
    updateTiles();
    m_weldFeat->requestPaint();
}

void TaskWeldingSymbol::onFlipSidesCreateClicked()
{
    QString tempText = ui->leOtherTextL->text();
    ui->leOtherTextL->setText(ui->leArrowTextL->text());
    ui->leArrowTextL->setText(tempText);
    tempText = ui->leOtherTextC->text();
    ui->leOtherTextC->setText(ui->leArrowTextC->text());
    ui->leArrowTextC->setText(tempText);
    tempText = ui->leOtherTextR->text();
    ui->leOtherTextR->setText(ui->leArrowTextR->text());
    ui->leArrowTextR->setText(tempText);

    QString tempPathArrow = m_otherPath;
    m_otherPath = m_arrowPath;
    m_arrowPath = tempPathArrow;
    tempText = ui->pbOtherSymbol->text();
    ui->pbOtherSymbol->setText(ui->pbArrowSymbol->text());
    ui->pbArrowSymbol->setText(tempText);
    QIcon tempIcon = ui->pbOtherSymbol->icon();
    ui->pbOtherSymbol->setIcon(ui->pbArrowSymbol->icon());
    ui->pbArrowSymbol->setIcon(tempIcon);
}

void TaskWeldingSymbol::onFlipSidesClicked()
{
    QString tempText = ui->leOtherTextL->text();
    ui->leOtherTextL->setText(ui->leArrowTextL->text());
    ui->leArrowTextL->setText(tempText);
    tempText = ui->leOtherTextC->text();
    ui->leOtherTextC->setText(ui->leArrowTextC->text());
    ui->leArrowTextC->setText(tempText);
    tempText = ui->leOtherTextR->text();
    ui->leOtherTextR->setText(ui->leArrowTextR->text());
    ui->leArrowTextR->setText(tempText);

    // one cannot get the path from the icon therefore read out
    // the path property
    auto tempPathArrow = m_arrowFeat->SymbolFile.getValue();
    auto tempPathOther = m_otherFeat->SymbolFile.getValue();
    m_otherPath = QString::fromLatin1(tempPathArrow);
    m_arrowPath = QString::fromLatin1(tempPathOther);
    QIcon tempIcon = ui->pbOtherSymbol->icon();
    ui->pbOtherSymbol->setIcon(ui->pbArrowSymbol->icon());
    ui->pbArrowSymbol->setIcon(tempIcon);

    m_otherDirty = true;
    updateTiles();
    m_weldFeat->requestPaint();
}

void TaskWeldingSymbol::onArrowTextChanged()
{
    updateTiles();
    m_weldFeat->requestPaint();
}

void TaskWeldingSymbol::onOtherTextChanged()
{
    m_otherDirty = true;
    updateTiles();
    m_weldFeat->requestPaint();
}

void TaskWeldingSymbol::onWeldingChanged()
{
    updateWeldingSymbol();
    m_weldFeat->requestPaint();
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
    QString arrow = tr("arrow");
    QString other = tr("other");
    if (source == arrow) {
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

void TaskWeldingSymbol::collectArrowData(void)
{
//    Base::Console().Message("TWS::collectArrowData()\n");
    m_arrowOut.toBeSaved = true;
    m_arrowOut.arrowSide = false;
    m_arrowOut.row = 0;
    m_arrowOut.col = 0;
    m_arrowOut.leftText = ui->leArrowTextL->text().toStdString();
    m_arrowOut.centerText = ui->leArrowTextC->text().toStdString();
    m_arrowOut.rightText = ui->leArrowTextR->text().toStdString();
    m_arrowOut.symbolPath= m_arrowPath.toStdString();
    m_arrowOut.tileName = "";
}

void TaskWeldingSymbol::collectOtherData(void)
{
//    Base::Console().Message("TWS::collectOtherData()\n");
    m_otherOut.toBeSaved = true;
    m_otherOut.arrowSide = false;
    m_otherOut.row = -1;
    m_otherOut.col = 0;
    m_otherOut.leftText = ui->leOtherTextL->text().toStdString();
    m_otherOut.centerText = ui->leOtherTextC->text().toStdString();
    m_otherOut.rightText = ui->leOtherTextR->text().toStdString();
    m_otherOut.symbolPath = m_otherPath.toStdString();
    m_otherOut.tileName = "";
}

void TaskWeldingSymbol::getTileFeats(void)
{
//    Base::Console().Message("TWS::getTileFeats()\n");
    std::vector<TechDraw::DrawTileWeld*> tiles = m_weldFeat->getTiles();
    m_arrowFeat = nullptr;
    m_otherFeat = nullptr;

    if (!tiles.empty()) {
        TechDraw::DrawTileWeld* tempTile = tiles.at(0);
        if (tempTile->TileRow.getValue() == 0) {
            m_arrowFeat = tempTile;
        } else {
            m_otherFeat = tempTile;
        }
    }
    if (tiles.size() > 1) {
        TechDraw::DrawTileWeld* tempTile = tiles.at(1);
        if (tempTile->TileRow.getValue() == 0) {
            m_arrowFeat = tempTile;
        } else {
            m_otherFeat = tempTile;
        }
    }
}

//******************************************************************************
TechDraw::DrawWeldSymbol* TaskWeldingSymbol::createWeldingSymbol(void)
{
//    Base::Console().Message("TWS::createWeldingSymbol()\n");

    std::string symbolName = m_leadFeat->getDocument()->getUniqueObjectName("WeldSymbol");
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

    std::string tailText = ui->leTailText->text().toStdString();
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

    std::string tailText = ui->leTailText->text().toStdString();
    tailText = Base::Tools::escapeEncodeString(tailText);
    Command::doCommand(Command::Doc,"App.activeDocument().%s.TailText = '%s'",
                           symbolName.c_str(), tailText.c_str());
}

void TaskWeldingSymbol::updateTiles(void)
{
//    Base::Console().Message("TWS::updateTiles()\n");
    getTileFeats();

    if (m_arrowFeat == nullptr) {
        Base::Console().Message("TWS::updateTiles - no arrow tile!\n");
    } else {
        collectArrowData();
        if (m_arrowOut.toBeSaved) {
            std::string tileName = m_arrowFeat->getNameInDocument();
            std::string leftText = Base::Tools::escapeEncodeString(m_arrowOut.leftText);
            std::string rightText = Base::Tools::escapeEncodeString(m_arrowOut.rightText);
            std::string centerText = Base::Tools::escapeEncodeString(m_arrowOut.centerText);
            Command::doCommand(Command::Doc,"App.activeDocument().%s.TileColumn = %d",
                           tileName.c_str(), m_arrowOut.col);
            Command::doCommand(Command::Doc,"App.activeDocument().%s.LeftText = '%s'",
                           tileName.c_str(), leftText.c_str());
            Command::doCommand(Command::Doc,"App.activeDocument().%s.RightText = '%s'",
                           tileName.c_str(), rightText.c_str());
            Command::doCommand(Command::Doc,"App.activeDocument().%s.CenterText = '%s'",
                           tileName.c_str(), centerText.c_str());
            if (!m_arrowOut.symbolPath.empty()) {
//                m_arrowFeat->replaceSymbol(m_arrowOut.symbolPath);
                m_arrowFeat->SymbolFile.setValue(m_arrowOut.symbolPath);
            }
        }
    }

    if (m_otherFeat == nullptr) {
//        Base::Console().Message("TWS::updateTiles - no other tile!\n");
    } else {
        if (m_otherDirty) {
            collectOtherData();
            if (m_otherOut.toBeSaved) {
                std::string tileName = m_otherFeat->getNameInDocument();
                std::string leftText = Base::Tools::escapeEncodeString(m_otherOut.leftText);
                std::string rightText = Base::Tools::escapeEncodeString(m_otherOut.rightText);
                std::string centerText = Base::Tools::escapeEncodeString(m_otherOut.centerText);
                Command::doCommand(Command::Doc,"App.activeDocument().%s.TileColumn = %d",
                               tileName.c_str(), m_otherOut.col);
                Command::doCommand(Command::Doc,"App.activeDocument().%s.LeftText = '%s'",
                               tileName.c_str(), leftText.c_str());
                Command::doCommand(Command::Doc,"App.activeDocument().%s.RightText = '%s'",
                               tileName.c_str(), rightText.c_str());
                Command::doCommand(Command::Doc,"App.activeDocument().%s.CenterText = '%s'",
                               tileName.c_str(), centerText.c_str());
//                m_otherFeat->replaceSymbol(m_otherOut.symbolPath);
                m_otherFeat->SymbolFile.setValue(m_otherOut.symbolPath);
            }
        }
    }
    return;
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

//******************************************************************************

bool TaskWeldingSymbol::accept()
{
//    Base::Console().Message("TWS::accept()\n");
    if (m_createMode) {
        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create WeldSymbol"));
        m_weldFeat = createWeldingSymbol();
        updateTiles();
        Gui::Command::updateActive();
        Gui::Command::commitCommand();
        m_weldFeat->recomputeFeature();
    //    m_weldFeat->requestPaint();    //not a dv!
    } else {
        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Edit WeldSymbol"));
        try {
            updateWeldingSymbol();
            updateTiles();
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
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_WeldSymbol"),
                                             widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgWeldingSymbol::TaskDlgWeldingSymbol(TechDraw::DrawWeldSymbol* weld)
    : TaskDialog()
{
    widget  = new TaskWeldingSymbol(weld);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_WeldSymbol"),
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
