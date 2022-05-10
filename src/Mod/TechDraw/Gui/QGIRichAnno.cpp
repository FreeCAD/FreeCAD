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
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <Precision.hxx>

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsItem>
#include <QPainter>
#include <QPainterPath>
#include <QPaintDevice>
#include <QSvgGenerator>
#include <QRegExp>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QTextFrame>
#include <QTextBlock>
#include <QTextCursor>
#include <QDialog>


# include <cmath>
#endif

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>
#include <Gui/Command.h>

#include <Mod/Part/App/PartFeature.h>

//#include <Mod/TechDraw/App/Preferences.h>
#include <Mod/TechDraw/App/DrawRichAnno.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/Geometry.h>

#include "Rez.h"
#include "ZVALUE.h"
#include "PreferencesGui.h"
#include "QGIArrow.h"
#include "ViewProviderRichAnno.h"
#include "MDIViewPage.h"
#include "DrawGuiUtil.h"
#include "QGVPage.h"
#include "QGIPrimPath.h"
#include "QGEPath.h"
#include "QGMText.h"
#include "QGIView.h"
#include "QGCustomText.h"
#include "QGCustomRect.h"

#include "QGIRichAnno.h"
#include "mrichtextedit.h"

using namespace TechDraw;
using namespace TechDrawGui;


//**************************************************************
QGIRichAnno::QGIRichAnno(QGraphicsItem* myParent,
                         TechDraw::DrawRichAnno* anno) :
    m_isExporting(false), m_hasHover(false)
{
    setHandlesChildEvents(false);
    setAcceptHoverEvents(false);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges,true);

    if (myParent != nullptr) {
        setParentItem(myParent);
    }

    setViewFeature(anno);

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

QVariant QGIRichAnno::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemSelectedHasChanged && scene()) {
        //There's nothing special for QGIRA to do when selection changes!
    } else if(change == ItemSceneChange && scene()) {
        // nothing special!
    }
    return QGIView::itemChange(change, value);
}

//void QGIRichAnno::select(bool state)
//{
//    setSelected(state);
//    draw();
//}

//void QGIRichAnno::hover(bool state)
//{
//    m_hasHover = state;
//    draw();
//}

void QGIRichAnno::updateView(bool update)
{
//    Base::Console().Message("QGIRA::updateView() - %s\n", getViewName());
    Q_UNUSED(update);
    auto annoFeat( dynamic_cast<TechDraw::DrawRichAnno*>(getViewObject()) );
    if ( annoFeat == nullptr ) {
        Base::Console().Log("QGIRA::updateView - no feature!\n");
        return;
    }

    auto vp = static_cast<ViewProviderRichAnno*>(getViewProvider(getViewObject()));
    if ( vp == nullptr ) {
        return;
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
//    Base::Console().Log("QGIRA::draw() - %s - parent: %X\n",getFeature()->getNameInDocument(), parentItem());
    if (!isVisible()) {
//        Base::Console().Message("QGIRA::draw - not visible\n");
        return;
    }

    TechDraw::DrawRichAnno* annoFeat = getFeature();
    if((!annoFeat) ) {
//        Base::Console().Message("QGIRA::draw - no feature\n");
        return;
    }

    auto vp = static_cast<ViewProviderRichAnno*>(getViewProvider(getFeature()));
    if ( vp == nullptr ) {
//        Base::Console().Message("QGIRA::draw - no viewprovider\n");
        return;
    }
//    double appX = Rez::guiX(annoFeat->X.getValue());
//    double appY = Rez::guiX(annoFeat->Y.getValue());

    QGIView::draw();

    setTextItem();
}

void QGIRichAnno::setTextItem()
{
//    Base::Console().Message("QGIRA::setTextItem() - %s\n",getViewName());
    TechDraw::DrawRichAnno* annoFeat = getFeature();
    QString inHtml = QString::fromUtf8(annoFeat->AnnoText.getValue());

    //don't do this multiplication if exporting to SVG as other apps interpret 
    //font sizes differently from QGraphicsTextItem (?)
    if (!getExporting()) {
        //convert point font sizes to (Rez,mm) font sizes
        QRegExp rxFontSize(QString::fromUtf8("font-size:([0-9]*)pt;"));
        QString match;
        double mmPerPoint = 0.353;
        double sizeConvert = Rez::getRezFactor() * mmPerPoint;
        int pos = 0;
        QStringList findList;
        QStringList replList;
        while ((pos = rxFontSize.indexIn(inHtml, pos)) != -1) {
            QString found = rxFontSize.cap(0);
            findList << found;
            QString qsOldSize = rxFontSize.cap(1); 

            QString repl = found;
            double newSize = qsOldSize.toDouble();
            newSize = newSize * sizeConvert;
            QString qsNewSize = QString::number(newSize, 'f', 2);
            repl.replace(qsOldSize,qsNewSize);
            replList << repl;
            pos += rxFontSize.matchedLength();
        }
        QString outHtml = inHtml;
        int iRepl = 0;
        //TODO: check list for duplicates?
        for ( ; iRepl < findList.size(); iRepl++) {
            outHtml = outHtml.replace(findList[iRepl], replList[iRepl]);
        }

        m_text->setTextWidth(Rez::guiX(annoFeat->MaxWidth.getValue()));
        m_text->setHtml(outHtml);
//        setLineSpacing(50);    //this has no effect on the display?!
//        m_text->update();

        if (annoFeat->ShowFrame.getValue()) {
            QRectF r = m_text->boundingRect().adjusted(1,1,-1,-1);
            m_rect->setPen(rectPen());
            m_rect->setBrush(Qt::NoBrush);
            m_rect->setRect(r);
            m_rect->show();
        } else {
            m_rect->hide();
        }
    } else {
        // don't force line wrap & strip formatting that doesn't export well!
        double realWidth = m_text->boundingRect().width();
        m_text->setTextWidth(realWidth);

        QFont f = prefFont();
        double ptSize = prefPointSize();
        f.setPointSizeF(ptSize);
        m_text->setFont(f);

        QString plainText = QTextDocumentFragment::fromHtml( inHtml ).toPlainText();
        m_text->setPlainText(plainText);
        setLineSpacing(100);       //this doesn't appear in the generated Svg, but does space the lines!
        m_rect->hide();
        m_rect->update();
    }

    m_text->centerAt(0.0, 0.0);
    m_rect->centerAt(0.0, 0.0);
}

void QGIRichAnno::setLineSpacing(int lineSpacing)
{
    //this line spacing should be px, but seems to be %? in any event, it does
    //space out the lines.
    QTextBlock block = m_text->document()->begin();
    for (; block.isValid(); block = block.next()) {
        QTextCursor tc = QTextCursor(block);
        QTextBlockFormat fmt = block.blockFormat();
//        fmt.setTopMargin(lineSpacing);            //no effect???
        fmt.setBottomMargin(lineSpacing);           //spaces out the lines!
        tc.setBlockFormat(fmt);
//        }
    }
}

//void QGIRichAnno::drawBorder()
//{
//////Leaders have no border!
////    QGIView::drawBorder();   //good for debugging
//}


TechDraw::DrawRichAnno* QGIRichAnno::getFeature(void)
{
    TechDraw::DrawRichAnno* result = 
         static_cast<TechDraw::DrawRichAnno*>(getViewObject());
    return result;
}

QRectF QGIRichAnno::boundingRect() const
{
    QRectF rect = mapFromItem(m_text,m_text->boundingRect()).boundingRect();
    return rect.adjusted(-10.,-10.,10.,10.);
}

QPainterPath QGIRichAnno::shape() const
{
    return QGraphicsItemGroup::shape();
}

void QGIRichAnno::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

//    painter->drawRect(boundingRect());          //good for debugging

    QGIView::paint (painter, &myOption, widget);
}

QPen QGIRichAnno::rectPen() const
{
    QPen pen;
    const auto sym( dynamic_cast<TechDraw::DrawRichAnno*>(getViewObject()) );
    if( sym == nullptr ) {
        return pen;
    }
    auto vp = static_cast<ViewProviderRichAnno*>(getViewProvider(getViewObject()));
    if ( vp == nullptr ) {
        return pen;
    }

    double rectWeight = Rez::guiX(vp->LineWidth.getValue());
    Qt::PenStyle rectStyle = (Qt::PenStyle) vp->LineStyle.getValue();
    App::Color temp = vp->LineColor.getValue();
    QColor rectColor = temp.asValue<QColor>(); 

    pen = QPen(rectStyle);
    pen.setWidthF(rectWeight);
    pen.setColor(rectColor);
    return pen;
}

QFont QGIRichAnno::prefFont(void)
{
    return PreferencesGui::labelFontQFont();
}

double QGIRichAnno::prefPointSize(void)
{
//    Base::Console().Message("QGIRA::prefPointSize()\n");
    double fontSize = Preferences::dimFontSizeMM();
    //this conversion is only approximate. the factor changes for different fonts.
//    double mmToPts = 2.83;  //theoretical value
    double mmToPts = 2.00;  //practical value. seems to be reasonable for common fonts.
    
    double ptsSize = round(fontSize * mmToPts);
    return ptsSize;
}

void QGIRichAnno::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) {
    Q_UNUSED(event);

    TechDraw::DrawRichAnno *annotation = dynamic_cast<TechDraw::DrawRichAnno *>(getViewObject());
    if (annotation == nullptr) {
        return;
    }

    QString text = QString::fromUtf8(annotation->AnnoText.getValue());

    QDialog dialog(nullptr);
    dialog.setWindowTitle(QObject::tr("Rich text editor"));
    dialog.setMinimumWidth(400);
    dialog.setMinimumHeight(400);

    MRichTextEdit richEdit(&dialog, text);
    QGridLayout gridLayout(&dialog);
    gridLayout.addWidget(&richEdit, 0, 0, 1, 1);

    connect(&richEdit, SIGNAL(saveText(QString)), &dialog, SLOT(accept()));
    connect(&richEdit, SIGNAL(editorFinished(void)), &dialog, SLOT(reject()));

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
