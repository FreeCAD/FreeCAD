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
    m_createMode(true),
    m_arrowDirty(false),
    m_otherDirty(false)
{
//TODO: why does DWS need DLL as parent?
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
    m_arrowSymbol = QString();
    m_otherOut.init();
    m_otherPath = QString();
    m_otherSymbol = QString();
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
            QSize iconSize(32,32);
            ui->pbArrowSymbol->setIcon(targetIcon);
            ui->pbArrowSymbol->setIconSize(iconSize);
            ui->pbArrowSymbol->setText(QString());
        } else {
            ui->pbArrowSymbol->setText(QString::fromUtf8("Symbol"));
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
            QSize iconSize(32,32);
            ui->pbOtherSymbol->setIcon(targetIcon);
            ui->pbOtherSymbol->setIconSize(iconSize);
            ui->pbOtherSymbol->setText(QString());
        } else {
            ui->pbOtherSymbol->setText(QString::fromUtf8("Symbol"));
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
    m_otherDirty = true;
    m_otherOut.init();

    ui->leOtherTextL->setText(QString());
    ui->leOtherTextC->setText(QString());
    ui->leOtherTextR->setText(QString());
    ui->pbOtherSymbol->setIcon(QIcon());
    ui->pbOtherSymbol->setText(QString::fromUtf8("Symbol"));
    m_otherOut.init();
    m_otherPath = QString();
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

//obsolete.  tiles are only updated on accept.
void TaskWeldingSymbol::saveState(void)
{
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
    m_otherOut.symbolPath = Base::Tools::toStdString(m_otherPath);
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
                m_arrowFeat->replaceSymbol(m_arrowOut.symbolPath);
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
                m_otherFeat->replaceSymbol(m_otherOut.symbolPath);
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
        updateTiles();
        Gui::Command::updateActive();
        Gui::Command::commitCommand();
        m_weldFeat->recomputeFeature();
    //    m_weldFeat->requestPaint();    //not a dv!
    } else {
        Gui::Command::openCommand("Edit WeldSymbol");
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
