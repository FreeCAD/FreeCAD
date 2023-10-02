/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
 *   Copyright (c) 2014 WandererFan <wandererfan@gmail.com>                *
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
#include <regex>
#include <sstream>
#include <string>

#include <QDialog>
#include <QDialogButtonBox>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QString>
#include <QVBoxLayout>
#endif

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Tools.h>
#include <Gui/MainWindow.h>
#include <Gui/Widgets.h>
#include <Mod/TechDraw/App/DrawViewAnnotation.h>
#include <Mod/TechDraw/App/Preferences.h>

#include "DlgStringListEditor.h"
#include "QGCustomText.h"
#include "QGIViewAnnotation.h"
#include "Rez.h"

using namespace TechDrawGui;

QGIViewAnnotation::QGIViewAnnotation()
{
    setCacheMode(QGraphicsItem::NoCache);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setAcceptHoverEvents(true);

    m_textItem = new QGCustomText();
    m_textItem->setTextInteractionFlags(Qt::NoTextInteraction);
    //To allow on screen editing of text:
    //m_textItem->setTextInteractionFlags(Qt::TextEditorInteraction);   //this works
    //QObject::connect(QGraphicsTextItem::document(), SIGNAL(contentsChanged()), m_textItem, SLOT(updateText()));  //not tested
    addToGroup(m_textItem);
    m_textItem->setPos(0., 0.);
}

void QGIViewAnnotation::setViewAnnoFeature(TechDraw::DrawViewAnnotation* obj)
{
    // called from QGVPage. (once)
    setViewFeature(static_cast<TechDraw::DrawView*>(obj));
}

void QGIViewAnnotation::updateView(bool update)
{
    auto viewAnno(dynamic_cast<TechDraw::DrawViewAnnotation*>(getViewObject()));
    if (!viewAnno) {
        return;
    }

    if (update || viewAnno->isTouched() || viewAnno->Text.isTouched() || viewAnno->Font.isTouched()
        || viewAnno->TextColor.isTouched() || viewAnno->TextSize.isTouched()) {

        draw();
    }

    QGIView::updateView(update);
}

void QGIViewAnnotation::draw()
{
    //    Base::Console().Message("QGIVA::draw()\n");
    if (!isVisible()) {
        return;
    }

    drawAnnotation();
    QGIView::draw();
    rotateView();
}

//TODO: text is positioned slightly high (and left??) on page save to SVG file

void QGIViewAnnotation::drawAnnotation()
{
    //    Base::Console().Message("QGIVA::drawAnnotation()\n");
    auto viewAnno(dynamic_cast<TechDraw::DrawViewAnnotation*>(getViewObject()));
    if (!viewAnno) {
        return;
    }

    const std::vector<std::string>& annoRawText = viewAnno->Text.getValues();
    std::vector<std::string> annoText;
    // v0.19- stored text as escapedUnicode
    // v0.20+ stores text as utf8
    for (auto& line : annoRawText) {
        if (line.find("\\x") == std::string::npos) {
            // not escaped
            annoText.push_back(line);
        } else {
            // is escaped
            std::string newLine = Base::Tools::escapedUnicodeToUtf8(line);
            annoText.push_back(newLine);
        }
    }

    int scaledSize = exactFontSize(viewAnno->Font.getValue(), viewAnno->TextSize.getValue());

    //build HTML/CSS formatting around Text lines
    std::stringstream ss;
    ss << "<html>\n<head>\n<style>\n";
    ss << "p {";
    ss << "font-family:" << viewAnno->Font.getValue() << "; ";
    ss << "font-size:" << scaledSize << "px; ";
    if (viewAnno->TextStyle.isValue("Normal")) {
        ss << "font-weight:normal; font-style:normal; ";
    } else if (viewAnno->TextStyle.isValue("Bold")) {
        ss << "font-weight:bold; font-style:normal; ";
    } else if (viewAnno->TextStyle.isValue("Italic")) {
        ss << "font-weight:normal; font-style:italic; ";
    } else if (viewAnno->TextStyle.isValue("Bold-Italic")) {
        ss << "font-weight:bold; font-style:italic; ";
    } else {
        Base::Console().Warning("%s has invalid TextStyle\n", viewAnno->getNameInDocument());
        ss << "font-weight:normal; font-style:normal; ";
    }
    ss << "line-height:" << viewAnno->LineSpace.getValue() << "%; ";
    App::Color c = viewAnno->TextColor.getValue();
    c = TechDraw::Preferences::getAccessibleColor(c);
    ss << "color:" << c.asHexString() << "; ";
    ss << "}\n</style>\n</head>\n<body>\n<p>";
    for (std::vector<std::string>::const_iterator it = annoText.begin(); it != annoText.end();
         it++) {
        if (it != annoText.begin()) {
            ss << "<br>";
        }

        //"less than" symbol chops off line.  need to use html sub.
        std::string lt = std::regex_replace((*it), std::regex("<"), "&lt;");
        ss << lt;
    }
    ss << "</p>\n</body>\n</html> ";

    prepareGeometryChange();
    m_textItem->setTextWidth(Rez::guiX(viewAnno->MaxWidth.getValue()));
    QString qs = QString::fromUtf8(ss.str().c_str());
    m_textItem->setHtml(qs);
    m_textItem->centerAt(0., 0.);
}

void QGIViewAnnotation::rotateView()
{
    QRectF r = m_textItem->boundingRect();
    m_textItem->setTransformOriginPoint(r.center());
    double rot = getViewObject()->Rotation.getValue();
    m_textItem->setRotation(-rot);
}

void QGIViewAnnotation::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    Q_UNUSED(event);

    TechDraw::DrawViewAnnotation* annotation =
        dynamic_cast<TechDraw::DrawViewAnnotation*>(getViewObject());
    if (!annotation) {
        return;
    }

    const std::vector<std::string>& values = annotation->Text.getValues();
    DlgStringListEditor dlg(values, Gui::getMainWindow());
    dlg.setWindowTitle(QString::fromUtf8("Annotation Text Editor"));
    if (dlg.exec() == QDialog::Accepted) {
        App::GetApplication().setActiveTransaction("Set Annotation Text");
        annotation->Text.setValues(dlg.getTexts());
        App::GetApplication().closeActiveTransaction();
    }
}
