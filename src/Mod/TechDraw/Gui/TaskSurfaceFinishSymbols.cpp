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
# include <boost/algorithm/string/predicate.hpp>
# include <QComboBox>
# include <QGraphicsProxyWidget>
# include <QLineEdit>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawViewSymbol.h>

#include "QGIView.h"
#include "ui_TaskSurfaceFinishSymbols.h"
#include "TaskSurfaceFinishSymbols.h"
#include "ViewProviderSymbol.h"
#include "ZVALUE.h"


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

TaskSurfaceFinishSymbols::TaskSurfaceFinishSymbols(const std::string &ownerName) :
    currentIcon(nullptr),
    ui(new Ui_TaskSurfaceFinishSymbols)
{
    App::Document *doc = App::GetApplication().getActiveDocument();
    if (doc) {
        owner = doc->getObject(ownerName.c_str());
        std::string subName;
        if (!owner) {
            size_t dot = ownerName.rfind('.');
            if (dot != std::string::npos) {
                subName = ownerName.substr(dot + 1);
                owner = doc->getObject(ownerName.substr(0, dot).c_str());
            }
        }

        auto page = dynamic_cast<TechDraw::DrawPage *>(owner);
        if (page) {
            placement.x = page->getPageWidth()/2.0;
            placement.y = page->getPageHeight()/2.0;
        }

        auto viewPart = dynamic_cast<TechDraw::DrawViewPart *>(owner);
        if (viewPart && !subName.empty()) {
            std::string subType = DrawUtil::getGeomTypeFromName(subName);
            if (subType == "Vertex") {
                TechDraw::VertexPtr vertex = viewPart->getVertex(subName);
                if (vertex) {
                    placement = vertex->point();
                }
            }
            else if (subType == "Edge") {
                TechDraw::BaseGeomPtr edge = viewPart->getEdge(subName);
                if (edge) {
                    placement = edge->getMidPoint();
                }
            }
            else if (subType == "Face") {
                TechDraw::FacePtr face = viewPart->getFace(subName);
                if (face) {
                    placement = face->getCenter();
                }
            }

            placement = DrawUtil::invertY(placement);
        }
    }

    raValues = {"Ra50", "Ra25", "Ra12, 5", "Ra6, 3",
                "Ra3, 2", "Ra1, 6", "Ra0, 8", "Ra0, 4",
                "Ra0, 2", "Ra0, 1", "Ra0, 05", "Ra0, 025"};
    laySymbols = {"", "=", "⟂", "X", "M", "C", "R"};
    roughGrades = {"", "N1", "N2", "N3", "N4", "N5",
                   "N6", "N7", "N8", "N9", "N10", "N11"};

    ui->setupUi(this);
    setUiEdit();
}

QColor TaskSurfaceFinishSymbols::getPenColor()
{
    // TODO: should be dependent on global API giving pen color - not from hacking stylesheet name
    const std::string stylesheetName = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/MainWindow")->GetASCII("StyleSheet");
    if(boost::icontains(stylesheetName, "dark")) {
        return Qt::white;
    }
    return Qt::black;
}

QPixmap TaskSurfaceFinishSymbols::baseSymbol(SymbolType type)
// return QPixmap showing a base symbol
{
    QImage img (50, 64, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);

    // TODO: color should depend on theme/background
    QPainter painter;
    painter.begin(&img);
    painter.setPen(QPen(getPenColor(), 2, Qt::SolidLine,
                        Qt::RoundCap, Qt::RoundJoin));
    painter.setRenderHints(QPainter::Antialiasing |
                           QPainter::SmoothPixmapTransform |
                           QPainter::TextAntialiasing);
    painter.drawLine(QLine(0, 40, 12, 60));
    painter.drawLine(QLine(12, 60, 42, 10));
    if (type == SymbolType::RemoveProhibit || type == SymbolType::RemoveProhibitAll)
        painter.drawEllipse(QPoint(12, 42), 9,9);
    if (type == SymbolType::RemoveRequired || type == SymbolType::RemoveRequiredAll)
        painter.drawLine(QLine(0, 40, 24, 40));
    if (type == SymbolType::AnyMethodAll ||
        type == SymbolType::RemoveProhibitAll ||
        type == SymbolType::RemoveRequiredAll)
        painter.drawEllipse(QPoint(42, 10), 6,6);
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
    if (activeIcon == SymbolType::RemoveProhibit || activeIcon == SymbolType::RemoveProhibitAll)
        symbol.addCircle(12, 46, 9);
    if (activeIcon == SymbolType::RemoveRequired || activeIcon == SymbolType::RemoveRequiredAll)
        symbol.addLine(0, 44, 24, 44);
    if (activeIcon == SymbolType::AnyMethodAll ||
        activeIcon == SymbolType::RemoveProhibitAll ||
        activeIcon == SymbolType::RemoveRequiredAll)
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
    ui->pbIcon01->setIcon(baseSymbol(SymbolType::AnyMethod));
    ui->pbIcon02->setIcon(baseSymbol(SymbolType::RemoveProhibit));
    ui->pbIcon03->setIcon(baseSymbol(SymbolType::RemoveRequired));
    ui->pbIcon04->setIcon(baseSymbol(SymbolType::AnyMethodAll));
    ui->pbIcon05->setIcon(baseSymbol(SymbolType::RemoveProhibitAll));
    ui->pbIcon06->setIcon(baseSymbol(SymbolType::RemoveRequiredAll));

    int w = ui->pbIcon01->width();
    int h = ui->pbIcon01->height();
    ui->pbIcon01->setIconSize(QSize(w, h));
    ui->pbIcon02->setIconSize(QSize(w, h));
    ui->pbIcon03->setIconSize(QSize(w, h));
    ui->pbIcon04->setIconSize(QSize(w, h));
    ui->pbIcon05->setIconSize(QSize(w, h));
    ui->pbIcon06->setIconSize(QSize(w, h));


    activeIcon = SymbolType::AnyMethod ;
    isISO = true;

    // Create scene and all items used in the scene
    symbolScene = new(QGraphicsScene);
    // symbolScene->setBackgroundBrush(Qt::blue);
    ui->graphicsView->setBackgroundBrush(Qt::NoBrush);
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
    proxyAddition->setPos(-110, -85);
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
    proxyMinRough->setPos(-100, -118);
    proxyMinRough-> setZValue(1);
    proxyMinRough->hide();
    // QComboBox showing maximal roughness grade
    cbMaxRought = new(QComboBox);
    cbMaxRought->resize(55, 20);
    for (const std::string& nextGrade : roughGrades)
        cbMaxRought->addItem(QString::fromStdString(nextGrade));
    cbMaxRought->setToolTip(QObject::tr("Maximum roughness grade number"));
    proxyMaxRough = symbolScene->addWidget(cbMaxRought);
    proxyMaxRough->setPos(-100, -143);
    proxyMaxRough->setZValue(1);
    proxyMaxRough->hide();
    // add horizontal line
    symbolScene->addLine(QLine(-8, -116, 90, -116),
                         QPen(getPenColor(), 2,Qt::SolidLine,
                         Qt::RoundCap, Qt::RoundJoin));

    connect(ui->pbIcon01, &QPushButton::clicked, this, &TaskSurfaceFinishSymbols::onIconChanged);
    connect(ui->pbIcon02, &QPushButton::clicked, this, &TaskSurfaceFinishSymbols::onIconChanged);
    connect(ui->pbIcon03, &QPushButton::clicked, this, &TaskSurfaceFinishSymbols::onIconChanged);
    connect(ui->pbIcon04, &QPushButton::clicked, this, &TaskSurfaceFinishSymbols::onIconChanged);
    connect(ui->pbIcon05, &QPushButton::clicked, this, &TaskSurfaceFinishSymbols::onIconChanged);
    connect(ui->pbIcon06, &QPushButton::clicked, this, &TaskSurfaceFinishSymbols::onIconChanged);
    connect(ui->rbISO, &QPushButton::clicked, this, &TaskSurfaceFinishSymbols::onISO);
    connect(ui->rbASME, &QPushButton::clicked, this, &TaskSurfaceFinishSymbols::onASME);

    // set initial icon
    ui->pbIcon01->click();
}

void TaskSurfaceFinishSymbols::onIconChanged()
// Slot: change icon of surface finish symbol
{
    QObject* senderObj(this->sender());
    QPushButton* pressedButton = qobject_cast<QPushButton*>(senderObj);
    if (!pressedButton) {
        return;
    }

    if (ui->pbIcon01 == pressedButton) activeIcon = SymbolType::AnyMethod;
    if (ui->pbIcon02 == pressedButton) activeIcon = SymbolType::RemoveProhibit;
    if (ui->pbIcon03 == pressedButton) activeIcon = SymbolType::RemoveRequired;
    if (ui->pbIcon04 == pressedButton) activeIcon = SymbolType::AnyMethodAll;
    if (ui->pbIcon05 == pressedButton) activeIcon = SymbolType::RemoveProhibitAll;
    if (ui->pbIcon06 == pressedButton) activeIcon = SymbolType::RemoveRequiredAll;

    QIcon symbolIcon = pressedButton->icon();
    if(currentIcon) {
        symbolScene->removeItem(currentIcon);
    }
    currentIcon = new(QGraphicsPixmapItem);
    currentIcon->setPixmap(symbolIcon.pixmap(50, 64));
    currentIcon->setPos(-50, -126);
    currentIcon->setZValue(-1);
    symbolScene->addItem(currentIcon);
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
    auto* surfaceSymbol = doc->addObject<TechDraw::DrawViewSymbol>("SurfaceSymbol");
    surfaceSymbol->Symbol.setValue(completeSymbol());
    surfaceSymbol->Rotation.setValue(ui->leAngle->text().toDouble());

    auto view = dynamic_cast<TechDraw::DrawView *>(owner);
    surfaceSymbol->Owner.setValue(view);
    surfaceSymbol->X.setValue(placement.x);
    surfaceSymbol->Y.setValue(placement.y);

    auto viewProvider = dynamic_cast<ViewProviderSymbol *>(QGIView::getViewProvider(surfaceSymbol));
    if (viewProvider) {
        viewProvider->StackOrder.setValue(ZVALUE::DIMENSION);
    }

    auto page = dynamic_cast<TechDraw::DrawPage *>(owner);
    if (!page && view) {
        page = view->findParentPage();
    }
    if (page) {
        page->addView(surfaceSymbol);
    }

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

TaskDlgSurfaceFinishSymbols::TaskDlgSurfaceFinishSymbols(const std::string &ownerName)
    : TaskDialog()
{
    widget  = new TaskSurfaceFinishSymbols(ownerName);
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
