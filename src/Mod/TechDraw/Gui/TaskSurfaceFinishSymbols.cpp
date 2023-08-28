/***************************************************************************
 *   Copyright (c) 2022 edi                                                *
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
# include <QComboBox>
# include <QGraphicsProxyWidget>
# include <QLineEdit>
#endif

#include <App/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawViewSymbol.h>

#include "ui_TaskSurfaceFinishSymbols.h"
#include "TaskSurfaceFinishSymbols.h"


using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;

SvgString::SvgString(int width, int height)
{
    svgStream << "<?xml version='1.0'?>\n";
    svgStream << "<svg width='" << width << "' height='" << height << "'>\n";
}

void SvgString::addLine(int xStart, int yStart, int xEnd, int yEnd)
{
    svgStream << "<path stroke='#000' stroke-width='1' d='";
    svgStream << " M" << xStart << ", " << yStart; // startpoint
    svgStream << " L" << xEnd << ", " << yEnd; // endpoint
    svgStream << "' />\n";
}

void SvgString::addCircle(int xCenter, int yCenter, int radius)
{
    svgStream << "<circle cx='" << xCenter <<"' cy='" << yCenter;
    svgStream << "' r='" << radius;
    svgStream << "' fill='none' stroke='#000' stroke-width='1'/>\n";
}

void SvgString::addText(int xText, int yText, std::string text)
{
    svgStream << "<text x='" << xText << "' y='" << yText;
    svgStream << "' style='font-size:18px'>" << text << "</text>\n";
}

std::string SvgString::finish()
{
    svgStream << "</svg>\n";
    return svgStream.str();
}

//===========================================================================
// TaskSurfaceFinishSymbols
//===========================================================================

TaskSurfaceFinishSymbols::TaskSurfaceFinishSymbols(TechDraw::DrawViewPart* view) :
    selectedView(view),
    ui(new Ui_TaskSurfaceFinishSymbols)
{
    raValues = {"Ra50", "Ra25", "Ra12, 5", "Ra6, 3",
                "Ra3, 2", "Ra1, 6", "Ra0, 8", "Ra0, 4",
                "Ra0, 2", "Ra0, 1", "Ra0, 05", "Ra0, 025"};
    laySymbols = {"", "=", "⟂", "X", "M", "C", "R"};
    roughGrades = {"", "N1", "N2", "N3", "N4", "N5",
                   "N6", "N7", "N8", "N9", "N10", "N11"};
    ui->setupUi(this);
    setUiEdit();
}

QPixmap TaskSurfaceFinishSymbols::baseSymbol(symbolType type)
// return QPixmap showing a base symbol
{
    QImage img (50, 64, QImage::Format_ARGB32_Premultiplied);
    img.fill(QColor(240, 240, 240));
    QPainter painter;
    painter.begin(&img);
    painter.setPen(QPen(Qt::black, 2, Qt::SolidLine,
                        Qt::RoundCap, Qt::RoundJoin));
    painter.setRenderHints(QPainter::Antialiasing |
                           QPainter::SmoothPixmapTransform |
                           QPainter::TextAntialiasing);
    painter.drawLine(QLine(0, 44, 12, 64));
    painter.drawLine(QLine(12, 64, 42, 14));
    if (type == removeProhibit || type == removeProhibitAll)
        painter.drawEllipse(QPoint(12, 46), 9,9);
    if (type == removeRequired || type == removeRequiredAll)
        painter.drawLine(QLine(0, 44, 24, 44));
    if (type > removeRequired)
        painter.drawEllipse(QPoint(42, 14), 6,6);
    painter.end();
    return QPixmap::fromImage(img);
}

std::string TaskSurfaceFinishSymbols::completeSymbol()
// return string showing the complete symbol
{
    SvgString symbol(150, 64);
    symbol.addLine(0, 44, 12, 64);
    symbol.addLine(12, 64, 42, 14);
    int moveLeft(0), maxTextLength(0);
    if (activeIcon == removeProhibit || activeIcon == removeProhibitAll)
        symbol.addCircle(12, 46, 9);
    if (activeIcon == removeRequired || activeIcon == removeRequiredAll)
        symbol.addLine(0, 44, 24, 44);
    if (activeIcon > removeRequired)
    {
        symbol.addCircle(42, 14, 6);
        moveLeft = 5 ;
    }
    std::string methodText = leMethod->text().toStdString();
    symbol.addText(42+moveLeft, 11, methodText);
    int methodTextLength = methodText.length();
    if (isISO)
    {
        std::string raText = cbRA->itemText(cbRA->currentIndex()).toStdString();
        symbol.addText(42+moveLeft, 30, raText);
        int raTextLength = raText.length();
        maxTextLength = std::max(methodTextLength, raTextLength)*9.25+moveLeft;
    }
    else
    {
        std::string sampleText = leSamLength->text().toStdString();
        symbol.addText(42+moveLeft, 30, sampleText);
        int sampleTextLength = sampleText.length();
        maxTextLength = std::max(methodTextLength, sampleTextLength)*9.25+moveLeft;
        std::string minRoughtText = cbMinRought->itemText(cbMinRought->currentIndex()).toStdString();
        symbol.addText(-10, 35, minRoughtText);
        std::string maxRoughtText = cbMaxRought->itemText(cbMaxRought->currentIndex()).toStdString();
        symbol.addText(-10, 20, maxRoughtText);
    }
    symbol.addLine(42, 14, 42+maxTextLength, 14);
    symbol.addText(20, 60, cbLay->itemText(cbLay->currentIndex()).toStdString());
    symbol.addText(-25, 60, leAddition->text().toStdString());
    std::string symbolAsString = symbol.finish();
    return symbolAsString;
}

void TaskSurfaceFinishSymbols::updateTask()
{
//    blockUpdate = true;

//    blockUpdate = false;
}


void TaskSurfaceFinishSymbols::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

void TaskSurfaceFinishSymbols::setUiEdit()
// arrange widget at tool start
{
    setWindowTitle(tr("Surface Finish Symbols"));
    // create icon pixmaps of QPushButtons
    ui->pbIcon01->setIcon(baseSymbol(anyMethod));
    ui->pbIcon02->setIcon(baseSymbol(removeProhibit));
    ui->pbIcon03->setIcon(baseSymbol(removeRequired));
    ui->pbIcon04->setIcon(baseSymbol(anyMethodAll));
    ui->pbIcon05->setIcon(baseSymbol(removeProhibitAll));
    ui->pbIcon06->setIcon(baseSymbol(removeRequiredAll));
    activeIcon = anyMethod ;
    isISO = true;

    // Create scene and all items used in the scene
    symbolScene = new(QGraphicsScene);
    ui->graphicsView->setScene(symbolScene);
    // QLineEdit showing method
    leMethod = new(QLineEdit);
    leMethod->resize(90, 20);
    leMethod->setToolTip(QObject::tr("Method"));
    QGraphicsProxyWidget* proxyMethod = symbolScene->addWidget(leMethod);
    proxyMethod->setPos(2, -142);
    // QLineEdit showing addition
    leAddition = new(QLineEdit);
    leAddition->resize(25, 20);
    leAddition->setToolTip(QObject::tr("Addition"));
    QGraphicsProxyWidget* proxyAddition = symbolScene->addWidget(leAddition);
    proxyAddition->setPos(-80, -85);
    proxyAddition->setZValue(-2);
    // QComboBox showing RA values
    cbRA = new(QComboBox);
    cbRA->resize(90, 20);
    for (const std::string& nextValue : raValues)
        cbRA->addItem(QString::fromStdString(nextValue));
    cbRA->setToolTip(QObject::tr("Average roughness"));
    proxyRA = symbolScene->addWidget(cbRA);
    proxyRA->setPos(2, -113);
    // QLineEdit showing sampling length value
    leSamLength = new(QLineEdit);
    leSamLength->resize(90, 20);
    leSamLength->setToolTip(QObject::tr("Roughness sampling length"));
    proxySamLength = symbolScene->addWidget(leSamLength);
    proxySamLength->setPos(2, -113);
    proxySamLength->hide();
    // QComboBox showing lay symbol
    cbLay = new(QComboBox);
    cbLay->resize(40, 20);
    for (const std::string& nextLay : laySymbols)
        cbLay->addItem(QString::fromStdString(nextLay));
    cbLay->setToolTip(QObject::tr("Lay symbol"));
    QGraphicsProxyWidget* proxyLay = symbolScene->addWidget(cbLay);
    proxyLay->setPos(-23, -85);
    // QComboBox showing minimal roughness grade
    cbMinRought = new(QComboBox);
    cbMinRought->resize(55, 20);
    for (const std::string& nextGrade : roughGrades)
        cbMinRought->addItem(QString::fromStdString(nextGrade));
    cbMinRought->setToolTip(QObject::tr("Minimum roughness grade number"));
    proxyMinRough = symbolScene->addWidget(cbMinRought);
    proxyMinRough->setPos(-80, -118);
    proxyMinRough-> setZValue(1);
    proxyMinRough->hide();
    // QComboBox showing maximal roughness grade
    cbMaxRought = new(QComboBox);
    cbMaxRought->resize(55, 20);
    for (const std::string& nextGrade : roughGrades)
        cbMaxRought->addItem(QString::fromStdString(nextGrade));
    cbMaxRought->setToolTip(QObject::tr("Maximum roughness grade number"));
    proxyMaxRough = symbolScene->addWidget(cbMaxRought);
    proxyMaxRough->setPos(-80, -143);
    proxyMaxRough->setZValue(1);
    proxyMaxRough->hide();
    // add horizontal line
    symbolScene->addLine(QLine(-8, -116, 90, -116),
                         QPen(Qt::black, 2,Qt::SolidLine,
                         Qt::RoundCap, Qt::RoundJoin));
    // add pixmap of the surface finish symbol
    QIcon symbolIcon = ui->pbIcon01->icon();
    QGraphicsPixmapItem* pixmapItem = new(QGraphicsPixmapItem);
    pixmapItem->setPixmap(symbolIcon.pixmap(50, 64));
    pixmapItem->setPos(-50, -130);
    pixmapItem->setZValue(-1);
    symbolScene->addItem(pixmapItem);

    connect(ui->pbIcon01, &QPushButton::clicked, this, &TaskSurfaceFinishSymbols::onIconChanged);
    connect(ui->pbIcon02, &QPushButton::clicked, this, &TaskSurfaceFinishSymbols::onIconChanged);
    connect(ui->pbIcon03, &QPushButton::clicked, this, &TaskSurfaceFinishSymbols::onIconChanged);
    connect(ui->pbIcon04, &QPushButton::clicked, this, &TaskSurfaceFinishSymbols::onIconChanged);
    connect(ui->pbIcon05, &QPushButton::clicked, this, &TaskSurfaceFinishSymbols::onIconChanged);
    connect(ui->pbIcon06, &QPushButton::clicked, this, &TaskSurfaceFinishSymbols::onIconChanged);
    connect(ui->rbISO, &QPushButton::clicked, this, &TaskSurfaceFinishSymbols::onISO);
    connect(ui->rbASME, &QPushButton::clicked, this, &TaskSurfaceFinishSymbols::onASME);
}

void TaskSurfaceFinishSymbols::onIconChanged()
// Slot: change icon of surface finish symbol
{
    QObject* senderObj(this->sender());
    QPushButton* pressedButton = qobject_cast<QPushButton*>(senderObj);
    if (!pressedButton) {
        return;
    }

    if (ui->pbIcon01 == pressedButton) activeIcon = anyMethod;
    if (ui->pbIcon02 == pressedButton) activeIcon = removeProhibit;
    if (ui->pbIcon03 == pressedButton) activeIcon = removeRequired;
    if (ui->pbIcon04 == pressedButton) activeIcon = anyMethodAll;
    if (ui->pbIcon05 == pressedButton) activeIcon = removeProhibitAll;
    if (ui->pbIcon06 == pressedButton) activeIcon = removeRequiredAll;

    QIcon symbolIcon = pressedButton->icon();
    QGraphicsPixmapItem* pixmapItem = new(QGraphicsPixmapItem);
    pixmapItem->setPixmap(symbolIcon.pixmap(50, 64));
    pixmapItem->setPos(-50, -130);
    pixmapItem->setZValue(-1);
    symbolScene->addItem(pixmapItem);
}

void TaskSurfaceFinishSymbols::onISO()
// Slot: show ISO template in scene
{
    isISO = true;
    proxySamLength->hide();
    proxyMinRough->hide();
    proxyMaxRough->hide();
    proxyRA->show();
}

void TaskSurfaceFinishSymbols::onASME()
// Slot: show ASME template in scene
{
    isISO = false;
    proxySamLength->show();
    proxyMinRough->show();
    proxyMaxRough->show();
    proxyRA->hide();
}

bool TaskSurfaceFinishSymbols::accept()
// Slot: dialog finished using OK
{
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Surface Finish Symbols"));
    App::Document *doc = Application::Instance->activeDocument()->getDocument();
    App::DocumentObject *docObject = doc->addObject("TechDraw::DrawViewSymbol", "SurfaceSymbol");
    TechDraw::DrawViewSymbol *surfaceSymbol = dynamic_cast<TechDraw::DrawViewSymbol*>(docObject);
    surfaceSymbol->Symbol.setValue(completeSymbol());
    surfaceSymbol->Rotation.setValue(ui->leAngle->text().toDouble());
    TechDraw::DrawPage* page = selectedView->findParentPage();
    page->addView(surfaceSymbol);
    Gui::Command::commitCommand();
    return true;
}

bool TaskSurfaceFinishSymbols::reject()
{
    return true;
}

//===========================================================================
// TaskDlgSurfaceFinishSymbols//
//===========================================================================

TaskDlgSurfaceFinishSymbols::TaskDlgSurfaceFinishSymbols(TechDraw::DrawViewPart* view)
    : TaskDialog()
{
    widget  = new TaskSurfaceFinishSymbols(view);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_SurfaceFinishSymbols"),
                                             widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgSurfaceFinishSymbols::~TaskDlgSurfaceFinishSymbols()
{
}

void TaskDlgSurfaceFinishSymbols::update()
{
//    widget->updateTask();
}

//==== calls from the TaskView ===============================================================
void TaskDlgSurfaceFinishSymbols::open()
{
}

void TaskDlgSurfaceFinishSymbols::clicked(int)
{
}

bool TaskDlgSurfaceFinishSymbols::accept()
{
    widget->accept();
    return true;
}

bool TaskDlgSurfaceFinishSymbols::reject()
{
    widget->reject();
    return true;
}

#include <Mod/TechDraw/Gui/moc_TaskSurfaceFinishSymbols.cpp>
