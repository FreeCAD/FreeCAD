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
# include <cmath>

# include <QDialog>
# include <QGraphicsItem>
# include <QGraphicsSceneMouseEvent>
# include <QPainter>
# include <QRegularExpression>
# include <QRegularExpressionMatch>
# include <QTextBlock>
# include <QTextCursor>
# include <QTextDocumentFragment>
#endif

#include <App/Application.h>
#include <Mod/TechDraw/App/DrawRichAnno.h>

#include "QGIRichAnno.h"
#include "mrichtextedit.h"
#include "PreferencesGui.h"
#include "QGCustomRect.h"
#include "QGCustomText.h"
#include "Rez.h"
#include "ViewProviderRichAnno.h"
#include "ZVALUE.h"


using namespace TechDraw;
using namespace TechDrawGui;


//**************************************************************
QGIRichAnno::QGIRichAnno() :
    m_isExportingPdf(false), m_isExportingSvg(false), m_hasHover(false)
{
    setHandlesChildEvents(false);
    setAcceptHoverEvents(false);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    m_text = new QGCustomText();
    m_text->setTextInteractionFlags(Qt::NoTextInteraction);
    addToGroup(m_text);
    m_text->setZValue(ZVALUE::DIMENSION);
    m_text->centerAt(0.0, 0.0);

    m_rect = new QGCustomRect();
    addToGroup(m_rect);
    m_rect->setZValue(ZVALUE::DIMENSION - 1);
    m_rect->centerAt(0.0, 0.0);

    setZValue(ZVALUE::DIMENSION);

}

void QGIRichAnno::updateView(bool update)
{
//    Base::Console().Message("QGIRA::updateView() - %s\n", getViewName());
    Q_UNUSED(update);
    auto annoFeat( dynamic_cast<TechDraw::DrawRichAnno*>(getViewObject()) );
    if (!annoFeat) {
        return;
    }

    auto vp = static_cast<ViewProviderRichAnno*>(getViewProvider(getViewObject()));
    if (!vp) {
        return;
    }

    //allow/prevent dragging
    if (getViewObject()->isLocked()) {
        setFlag(QGraphicsItem::ItemIsMovable, false);
    } else {
        setFlag(QGraphicsItem::ItemIsMovable, true);
    }

    if (annoFeat->X.isTouched() ||
        annoFeat->Y.isTouched()) {
        float x = Rez::guiX(annoFeat->X.getValue());
        float y = Rez::guiX(annoFeat->Y.getValue());
        m_text->centerAt(x, -y);
        m_rect->centerAt(x, -y);
     }

    draw();
}

void QGIRichAnno::drawBorder()
{
////Leaders have no border!
//    QGIView::drawBorder();   //good for debugging
}


void QGIRichAnno::draw()
{
//    Base::Console().Log("QGIRA::draw() - %s - parent: %X\n", getFeature()->getNameInDocument(), parentItem());
    if (!isVisible())
//        Base::Console().Message("QGIRA::draw - not visible\n");
        return;

    TechDraw::DrawRichAnno* annoFeat = getFeature();
    if (!annoFeat)
//        Base::Console().Message("QGIRA::draw - no feature\n");
        return;

    auto vp = static_cast<ViewProviderRichAnno*>(getViewProvider(getFeature()));
    if (!vp) {
//        Base::Console().Message("QGIRA::draw - no viewprovider\n");
        return;
    }

    setTextItem();

    QGIView::draw();
}

void QGIRichAnno::setTextItem()
{
//    Base::Console().Message("QGIRA::setTextItem() - %s\n", getViewName());
    TechDraw::DrawRichAnno* annoFeat = getFeature();
    QString inHtml = QString::fromUtf8(annoFeat->AnnoText.getValue());
    QRectF inRect = m_text->boundingRect();
    double fontSize = m_text->font().pointSizeF();

    QRegularExpression rxFontSize(QString::fromUtf8("font-size:([0-9]*)pt;"));
    QRegularExpressionMatch match;
    double mmPerPoint = 0.353;                  // 25.4 mm/in / 72 points/inch
    double cssPxPerPoint = 16.0 / 12.0;         // CSS says 12 pt text is 16 px high
    double sceneUnitsPerPoint = Rez::getRezFactor() * mmPerPoint;      // scene units per point: 3.53
    int pos = 0;
    QStringList findList;
    QStringList replList;
    while ((pos = inHtml.indexOf(rxFontSize, pos, &match)) != -1) {
        QString found = match.captured(0);
        findList << found;
        QString qsOldSize = match.captured(1);

        QString repl = found;
        double newSize = qsOldSize.toDouble();      // in points
        // The font size in the QGraphicsTextItem html is interpreted differently
        // in svg rendering compared to the screen or pdf?
        if (getExportingSvg()) {
            // scale point size to CSS pixels
            newSize = newSize * cssPxPerPoint;
        } else {
            // scale point size to scene units
            newSize = newSize * sceneUnitsPerPoint;
        }
        QString qsNewSize = QString::number(newSize, 'f', 2);
        repl.replace(qsOldSize, qsNewSize);
        replList << repl;
        pos += match.capturedLength();
    }
    QString outHtml = inHtml;
    int iRepl = 0;
    //TODO: check list for duplicates?
    for ( ; iRepl < findList.size(); iRepl++) {
        outHtml = outHtml.replace(findList[iRepl], replList[iRepl]);
    }

    prepareGeometryChange();
    double actualWidth = m_text->textWidth();
    m_text->setTextWidth(Rez::guiX(annoFeat->MaxWidth.getValue()));
    m_text->setHtml(outHtml);
    if (getExportingSvg()) {
        m_text->setTextWidth(actualWidth);
        // lines are correctly spaced on screen or in pdf, but svg needs this
        setLineSpacing(100);
    }

    if (annoFeat->ShowFrame.getValue()) {
        QRectF outRect = m_text->boundingRect().adjusted(1, 1,-1, -1);
        m_rect->setPen(rectPen());
        m_rect->setBrush(Qt::NoBrush);
        m_rect->setRect(outRect);
        if (getExportingSvg()) {
            // the original text bounding rect is closer to the right size vs
            // the bounding rect after the text size is changed
            m_rect->setRect(inRect);
        }
        m_rect->centerAt(0.0, 0.0);
        m_rect->show();
    } else {
        m_rect->hide();
    }

    m_text->centerAt(0.0, 0.0);
    if (getExportingSvg()) {
        // m_text position will be wrong in svg, but m_rect will be correct, so
        // we adjust the position of m_text
        // double spaced default font size in px
        double verticalAdjust = cssPxPerPoint * fontSize * 2.0;
        QPointF textPos(m_rect->pos().x() + 1.0, m_rect->pos().y() + verticalAdjust);
        m_text->setPos(textPos);
    }
}

// attempt to space the lines correctly after the font sizes are changed to match
// the Svg rendering of the QGraphicsTextItem.
void QGIRichAnno::setLineSpacing(int lineSpacing)
{
    // left to itself, Qt appears to space the lines according to this formula
    // DeltaY(in mm) = (1 + 2*pointSize + margin) which vastly under spaces the
    // lines
    m_text->document()->setUseDesignMetrics(true);

    QTextBlock block = m_text->document()->begin();
    for (; block.isValid(); block = block.next()) {
        QTextCursor tc = QTextCursor(block);
        QTextBlockFormat fmt = tc.blockFormat();
        QTextCharFormat cFmt = tc.charFormat();
        // this is already converted to pixels in setTextItem
        //
        double cssPixelSize = cFmt.font().pointSizeF();
        // css Pixels are treated as if they were points in the conversion to Svg
        double textHeightSU = Rez::guiX(cssPixelSize * 25.4 / 72.0);  // actual height of text in scene units
        double pointSize = cssPixelSize * 72.0 / 96.0;
        double deltaYSU = 1.0 + 2.0 * pointSize;    // how far Qt will space lines (based on samples)
        double linegap = 0.4 * cssPixelSize;  // 20% gaps above and below
        // margins will be included in Qt's calculation of spacing

        double margin = linegap * pointSize / 10.0;
        QTextBlockFormat spacerFmt = QTextBlockFormat();
        if (block.previous().isValid()) {
            // there is a block before this one, so add a top margin
            spacerFmt.setTopMargin(margin);
        }
        if (block.next().isValid()) {
            // there is another block after this, so add a bottom margin
            spacerFmt.setBottomMargin(margin);
        }
        double requiredSpacing = (textHeightSU / (deltaYSU - 1.0)) * lineSpacing;
        spacerFmt.setLineHeight(requiredSpacing, QTextBlockFormat::ProportionalHeight);
        tc.mergeBlockFormat(spacerFmt);
    }
}

TechDraw::DrawRichAnno* QGIRichAnno::getFeature()
{
    TechDraw::DrawRichAnno* result =
         static_cast<TechDraw::DrawRichAnno*>(getViewObject());
    return result;
}

QRectF QGIRichAnno::boundingRect() const
{
    QRectF rect = mapFromItem(m_text, m_text->boundingRect()).boundingRect();
    return rect.adjusted(-10., -10., 10., 10.);
}

void QGIRichAnno::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

//    painter->drawRect(boundingRect());          //good for debugging

    QGIView::paint (painter, &myOption, widget);
}

QPen QGIRichAnno::rectPen() const
{
    const auto sym( dynamic_cast<TechDraw::DrawRichAnno*>(getViewObject()) );
    if (!sym) {
        return QPen();
    }
    auto vp = static_cast<ViewProviderRichAnno*>(getViewProvider(getViewObject()));
    if (!vp) {
        return QPen();
    }

    double rectWeight = Rez::guiX(vp->LineWidth.getValue());
    Qt::PenStyle rectStyle = static_cast<Qt::PenStyle>(vp->LineStyle.getValue());
    App::Color temp = vp->LineColor.getValue();
    QColor rectColor = temp.asValue<QColor>();

    QPen pen = QPen(rectStyle);
    pen.setWidthF(rectWeight);
    pen.setColor(rectColor);
    return pen;
}

QFont QGIRichAnno::prefFont()
{
    return PreferencesGui::labelFontQFont();
}

void QGIRichAnno::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) {
    Q_UNUSED(event);

    TechDraw::DrawRichAnno *annotation = dynamic_cast<TechDraw::DrawRichAnno *>(getViewObject());
    if (!annotation)
        return;

    QString text = QString::fromUtf8(annotation->AnnoText.getValue());

    QDialog dialog(nullptr);
    dialog.setWindowTitle(QObject::tr("Rich text editor"));
    dialog.setMinimumWidth(400);
    dialog.setMinimumHeight(400);

    MRichTextEdit richEdit(&dialog, text);
    QGridLayout gridLayout(&dialog);
    gridLayout.addWidget(&richEdit, 0, 0, 1, 1);

    connect(&richEdit, &MRichTextEdit::saveText, &dialog, &QDialog::accept);
    connect(&richEdit, &MRichTextEdit::editorFinished, &dialog, &QDialog::reject);

    if (dialog.exec()) {
        QString newText = richEdit.toHtml();
        if (newText != text) {
            App::GetApplication().setActiveTransaction("Set Rich Annotation Text");
            annotation->AnnoText.setValue(newText.toStdString());
            App::GetApplication().closeActiveTransaction();
        }
    }
}

#include <Mod/TechDraw/Gui/moc_QGIRichAnno.cpp>
