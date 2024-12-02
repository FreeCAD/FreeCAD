/***************************************************************************
 *   Copyright (c) 2018 WandererFan <wandererfan@gmail.com>                *
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
#endif // #ifndef _PreComp_

#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/CenterLine.h>
#include <Mod/TechDraw/App/Geometry.h>
#include <Mod/TechDraw/App/LineGenerator.h>


#include "TaskLineDecor.h"
#include "ui_TaskLineDecor.h"
#include "ui_TaskRestoreLines.h"
#include "QGIView.h"
#include "ViewProviderViewPart.h"
#include "DrawGuiUtil.h"


using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;

TaskLineDecor::TaskLineDecor(TechDraw::DrawViewPart* partFeat,
                             std::vector<std::string> edgeNames) :
    ui(new Ui_TaskLineDecor),
    m_partFeat(partFeat),
    m_edges(edgeNames),
    m_apply(true)
{
    initializeRejectArrays();
    m_lineGenerator = new TechDraw::LineGenerator;

    ui->setupUi(this);

    getDefaults();
    initUi();

    connect(ui->cb_Style, qOverload<int>(&QComboBox::currentIndexChanged), this, &TaskLineDecor::onStyleChanged);
    connect(ui->cc_Color, &ColorButton::changed, this, &TaskLineDecor::onColorChanged);
    connect(ui->dsb_Weight, qOverload<double>(&QuantitySpinBox::valueChanged), this, &TaskLineDecor::onWeightChanged);
    connect(ui->cb_Visible, qOverload<int>(&QComboBox::currentIndexChanged), this, &TaskLineDecor::onVisibleChanged);
}

TaskLineDecor::~TaskLineDecor()
{
    delete m_lineGenerator;
}

void TaskLineDecor::initUi()
{
    std::string viewName = m_partFeat->getNameInDocument();
    ui->le_View->setText(QString::fromStdString(viewName));

    std::stringstream ss;
    for (auto& e: m_edges) {
        int num = DrawUtil::getIndexFromName(e);
        ss << num << ", ";
    }
    std::string temp = ss.str();
    if (!temp.empty()) {
        temp.resize(temp.length() - 2);
    }
    ui->le_Lines->setText(QString::fromStdString(temp));

    ui->cc_Color->setColor(m_color.asValue<QColor>());
    ui->dsb_Weight->setValue(m_weight);
    ui->dsb_Weight->setSingleStep(0.1);
    ui->cb_Visible->setCurrentIndex(m_visible);

    // line numbering starts at 1, not 0
    DrawGuiUtil::loadLineStyleChoices(ui->cb_Style, m_lineGenerator);
    if (ui->cb_Style->count() >= m_lineNumber ) {
        ui->cb_Style->setCurrentIndex(m_lineNumber - 1);
    }
}

TechDraw::LineFormat *TaskLineDecor::getFormatAccessPtr(const std::string &edgeName, std::string *newFormatTag)
{
    BaseGeomPtr bg = m_partFeat->getEdge(edgeName);
    if (bg) {
        if (bg->getCosmetic()) {
            if (bg->source() == SourceType::COSEDGE) {
                TechDraw::CosmeticEdge *ce = m_partFeat->getCosmeticEdgeBySelection(edgeName);
                if (ce) {
                    return &ce->m_format;
                }
            }
            else if (bg->source() == SourceType::CENTERLINE) {
                TechDraw::CenterLine *cl = m_partFeat->getCenterLineBySelection(edgeName);
                if (cl) {
                    return &cl->m_format;
                }
            }
        }
        else {
            TechDraw::GeomFormat *gf = m_partFeat->getGeomFormatBySelection(edgeName);
            if (gf) {
                return &gf->m_format;
            }
            else {
                ViewProviderViewPart *viewPart = dynamic_cast<ViewProviderViewPart *>(QGIView::getViewProvider(m_partFeat));
                if (viewPart) {
                    TechDraw::LineFormat lineFormat(Qt::SolidLine, viewPart->LineWidth.getValue(), LineFormat::getDefEdgeColor(), true);
                    TechDraw::GeomFormat geomFormat(DrawUtil::getIndexFromName(edgeName), lineFormat);

                    std::string formatTag = m_partFeat->addGeomFormat(&geomFormat);
                    if (newFormatTag) {
                        *newFormatTag = formatTag;
                    }

                    return &m_partFeat->getGeomFormat(formatTag)->m_format;
                }
            }
        }
    }
    return {};
}

void TaskLineDecor::initializeRejectArrays()
{
    m_originalFormats.resize(m_edges.size());
    m_createdFormatTags.resize(m_edges.size());

    for (size_t i = 0; i < m_edges.size(); ++i) {
        std::string newTag;
        TechDraw::LineFormat *accessPtr = getFormatAccessPtr(m_edges[i], &newTag);

        if (accessPtr) {
            m_originalFormats[i] = *accessPtr;
            if (!newTag.empty()) {
                m_createdFormatTags[i] = newTag;
            }
        }
    }
}

// get the current line tool appearance default
void TaskLineDecor::getDefaults()
{
//    Base::Console().Message("TLD::getDefaults()\n");
    m_color = LineFormat::getCurrentLineFormat().getColor();
    m_weight = LineFormat::getCurrentLineFormat().getWidth();
    m_visible = LineFormat::getCurrentLineFormat().getVisible();
    m_lineNumber = LineFormat::getCurrentLineFormat().getLineNumber();

    //set defaults to format of 1st edge
    if (!m_originalFormats.empty()) {
        LineFormat &lf = m_originalFormats.front();
        m_style = lf.getStyle();
        m_color = lf.getColor();
        m_weight = lf.getWidth();
        m_visible = lf.getVisible();
        m_lineNumber = lf.getLineNumber();
    }
}

void TaskLineDecor::onStyleChanged()
{
    m_lineNumber = ui->cb_Style->currentIndex() + 1;
    applyDecorations();
    m_partFeat->requestPaint();
}

void TaskLineDecor::onColorChanged()
{
    m_color.setValue<QColor>(ui->cc_Color->color());
    applyDecorations();
    m_partFeat->requestPaint();
}

void TaskLineDecor::onWeightChanged()
{
    m_weight = ui->dsb_Weight->value().getValue();
    applyDecorations();
    m_partFeat->requestPaint();
}

void TaskLineDecor::onVisibleChanged()
{
    m_visible = ui->cb_Visible->currentIndex();
    applyDecorations();
    m_partFeat->requestPaint();
}

void TaskLineDecor::applyDecorations()
{
//    Base::Console().Message("TLD::applyDecorations()\n");
    for (auto& e: m_edges) {
        LineFormat *lf = getFormatAccessPtr(e);
        if (lf) {
            lf->setStyle(m_style);
            lf->setColor(m_color);
            lf->setWidth(m_weight);
            lf->setVisible(m_visible);
            lf->setLineNumber(m_lineNumber);
        }
    }
}

bool TaskLineDecor::accept()
{
//    Base::Console().Message("TLD::accept()\n");
    Gui::Document* doc = Gui::Application::Instance->getDocument(m_partFeat->getDocument());
    if (!doc)
        return false;

    if (apply()) {
        applyDecorations();
    }

    m_partFeat->requestPaint();

    //Gui::Command::updateActive();     //no chain of updates here
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");

    return true;
}

bool TaskLineDecor::reject()
{
//    Base::Console().Message("TLD::reject()\n");
    Gui::Document* doc = Gui::Application::Instance->getDocument(m_partFeat->getDocument());
    if (!doc)
        return false;

    for (size_t i = 0; i < m_originalFormats.size(); ++i) {
        std::string &formatTag = m_createdFormatTags[i];
        if (formatTag.empty()) {
            LineFormat *lf = getFormatAccessPtr(m_edges[i]);
            if (lf) {
                *lf = m_originalFormats[i];
            }
        }
        else {
            m_partFeat->removeGeomFormat(formatTag);
        }
    }

    m_partFeat->requestPaint();

    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");
    return false;
}

void TaskLineDecor::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TaskRestoreLines::TaskRestoreLines(TechDraw::DrawViewPart* partFeat,
                                   TaskLineDecor* parent) :
    ui(new Ui_TaskRestoreLines),
    m_partFeat(partFeat),
    m_parent(parent)
{
    ui->setupUi(this);

    connect(ui->pb_All, &QPushButton::clicked, this, &TaskRestoreLines::onAllPressed);
    connect(ui->pb_Geometry, &QPushButton::clicked, this, &TaskRestoreLines::onGeometryPressed);
    connect(ui->pb_Cosmetic, &QPushButton::clicked, this, &TaskRestoreLines::onCosmeticPressed);
    connect(ui->pb_Center, &QPushButton::clicked, this, &TaskRestoreLines::onCenterPressed);

    initUi();
}

TaskRestoreLines::~TaskRestoreLines()
{
}

void TaskRestoreLines::initUi()
{
    ui->l_All->setText(QString::number(countInvisibleLines()));
    ui->l_Geometry->setText(QString::number(countInvisibleGeoms()));
    ui->l_Cosmetic->setText(QString::number(countInvisibleCosmetics()));
    ui->l_Center->setText(QString::number(countInvisibleCenters()));
}

void TaskRestoreLines::onAllPressed()
{
//    Base::Console().Message("TRL::onAllPressed()\n");
    onGeometryPressed();
    onCosmeticPressed();
    onCenterPressed();
}

void TaskRestoreLines::onGeometryPressed()
{
//    Base::Console().Message("TRL::onGeometryPressed()\n");
    restoreInvisibleGeoms();
    ui->l_Geometry->setText(QString::number(0));
    ui->l_All->setText(QString::number(countInvisibleLines()));
}

void TaskRestoreLines::onCosmeticPressed()
{
//    Base::Console().Message("TRL::onCosmeticPressed()\n");
    restoreInvisibleCosmetics();
    ui->l_Cosmetic->setText(QString::number(0));
    ui->l_All->setText(QString::number(countInvisibleLines()));
}

void TaskRestoreLines::onCenterPressed()
{
//    Base::Console().Message("TRL::onCenterPressed()\n");
    restoreInvisibleCenters();
    ui->l_Center->setText(QString::number(0));
    ui->l_All->setText(QString::number(countInvisibleLines()));
}

int TaskRestoreLines::countInvisibleLines()
{
    int result = 0;
    result += countInvisibleGeoms();
    result += countInvisibleCosmetics();
    result += countInvisibleCenters();
    return result;
}

int TaskRestoreLines::countInvisibleGeoms()
{
    int iGeoms = 0;
    const std::vector<TechDraw::GeomFormat*> geoms = m_partFeat->GeomFormats.getValues();
    for (auto& g : geoms) {
        if (!g->m_format.getVisible()) {
            iGeoms++;
        }
    }
    return iGeoms;
}

int TaskRestoreLines::countInvisibleCosmetics()
{
    int iCosmos = 0;
    const std::vector<TechDraw::CosmeticEdge*> cosmos = m_partFeat->CosmeticEdges.getValues();
    for (auto& c : cosmos) {
        if (!c->m_format.getVisible()) {
            iCosmos++;
        }
    }
    return iCosmos++;
}

int TaskRestoreLines::countInvisibleCenters()
{
    int iCenter = 0;
    const std::vector<TechDraw::CenterLine*> centers = m_partFeat->CenterLines.getValues();
    for (auto& c : centers) {
        if (!c->m_format.getVisible()) {
            iCenter++;
        }
    }
    return iCenter++;
}

void TaskRestoreLines::restoreInvisibleLines()
{
    restoreInvisibleGeoms();
    restoreInvisibleCosmetics();
    restoreInvisibleCenters();
}

void TaskRestoreLines::restoreInvisibleGeoms()
{
    const std::vector<TechDraw::GeomFormat*> geoms = m_partFeat->GeomFormats.getValues();
    for (auto& g : geoms) {
        if (!g->m_format.getVisible()) {
            g->m_format.setVisible(true);
        }
    }
    m_partFeat->GeomFormats.setValues(geoms);
    m_parent->apply(false);                   //don't undo the work we just did
}

void TaskRestoreLines::restoreInvisibleCosmetics()
{
    const std::vector<TechDraw::CosmeticEdge*> cosmos = m_partFeat->CosmeticEdges.getValues();
    for (auto& c : cosmos) {
        if (!c->m_format.getVisible()) {
            c->m_format.setVisible(true);
        }
    }
    m_partFeat->CosmeticEdges.setValues(cosmos);
    m_parent->apply(false);                   //don't undo the work we just did
}

void TaskRestoreLines::restoreInvisibleCenters()
{
    const std::vector<TechDraw::CenterLine*> centers = m_partFeat->CenterLines.getValues();
    for (auto& c : centers) {
        if (!c->m_format.getVisible()) {
            c->m_format.setVisible(true);
        }
    }
    m_partFeat->CenterLines.setValues(centers);
    m_parent->apply(false);                   //don't undo the work we just did
}


bool TaskRestoreLines::accept()
{
//    Base::Console().Message("TRL::accept()\n");
    return true;
}

bool TaskRestoreLines::reject()
{
//    Base::Console().Message("TRL::reject()\n");
    return false;
}

void TaskRestoreLines::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TaskDlgLineDecor::TaskDlgLineDecor(TechDraw::DrawViewPart* partFeat,
                                   std::vector<std::string> edgeNames) :
    TaskDialog()
{
    widget  = new TaskLineDecor(partFeat, edgeNames);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_DecorateLine"),
                                         widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
    if (edgeNames.empty()) {
        taskbox->hideGroupBox();
    }

    TaskLineDecor* parent = dynamic_cast<TaskLineDecor*>(widget);
    if (parent) {
        restore = new TaskRestoreLines(partFeat, parent);
        restoreBox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_DecorateLine"),
                                             tr("Restore Invisible Lines"), true, nullptr);
        restoreBox->groupLayout()->addWidget(restore);
        Content.push_back(restoreBox);
    }
}

TaskDlgLineDecor::~TaskDlgLineDecor()
{
}

//==== calls from the TaskView ===============================================================
void TaskDlgLineDecor::open()
{

}

void TaskDlgLineDecor::clicked(int i)
{
    Q_UNUSED(i);
}

bool TaskDlgLineDecor::accept()
{
//    Base::Console().Message("TDLD::accept()\n");
    widget->accept();
    return true;
}

bool TaskDlgLineDecor::reject()
{
//    Base::Console().Message("TDLD::reject()\n");
    widget->reject();
    return true;
}

#include <Mod/TechDraw/Gui/moc_TaskLineDecor.cpp>
