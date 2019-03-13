/***************************************************************************
 *   Copyright (c) 2012-2014 Luke Parry <l.parry@warwick.ac.uk>            *
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
#include <QFile>
#include <QFont>
#include <QPen>
#include <QSvgRenderer>
#include <QGraphicsSvgItem>
#include <boost/regex.hpp>
#endif // #ifndef _PreComp_

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Gui/Command.h>

#include <Mod/TechDraw/App/Geometry.h>
#include <Mod/TechDraw/App/DrawSVGTemplate.h>

#include "Rez.h"
#include "ZVALUE.h"
#include "TemplateTextField.h"
#include "QGISVGTemplate.h"

using namespace TechDrawGui;

QGISVGTemplate::QGISVGTemplate(QGraphicsScene *scene)
    : QGITemplate(scene),
      firstTime(true)
{

    m_svgItem = new QGraphicsSvgItem(this);
    m_svgRender = new QSvgRenderer();

    m_svgItem->setSharedRenderer(m_svgRender);

    m_svgItem->setFlags(QGraphicsItem::ItemClipsToShape);
    m_svgItem->setCacheMode(QGraphicsItem::NoCache);

    addToGroup(m_svgItem);

    m_svgItem->setZValue(ZVALUE::SVGTEMPLATE);
    setZValue(ZVALUE::SVGTEMPLATE);

}

QGISVGTemplate::~QGISVGTemplate()
{
    delete m_svgRender;
}

QVariant QGISVGTemplate::itemChange(GraphicsItemChange change,
                                    const QVariant &value)
{
    return QGraphicsItemGroup::itemChange(change, value);
}


void QGISVGTemplate::openFile(const QFile &file)
{
    Q_UNUSED(file);
}

void QGISVGTemplate::load(const QString &fileName)
{
    if (fileName.isEmpty()){
        return;
    }

    QFile file(fileName);
    if (!file.exists()) {
        return;
    }
    m_svgRender->load(file.fileName());

    QSize size = m_svgRender->defaultSize();
    m_svgItem->setSharedRenderer(m_svgRender);

    if (firstTime) {
        createClickHandles();
        firstTime = false;
    }

    //convert from pixels or mm or inches in svg file to mm page size
    TechDraw::DrawSVGTemplate *tmplte = getSVGTemplate();
    double xaspect, yaspect;
    xaspect = tmplte->getWidth() / (double) size.width();
    yaspect = tmplte->getHeight() / (double) size.height();

    QTransform qtrans;
    qtrans.translate(0.f, Rez::guiX(-tmplte->getHeight()));
    qtrans.scale(Rez::guiX(xaspect) , Rez::guiX(yaspect));
    m_svgItem->setTransform(qtrans);
}

TechDraw::DrawSVGTemplate * QGISVGTemplate::getSVGTemplate()
{
    if(pageTemplate && pageTemplate->isDerivedFrom(TechDraw::DrawSVGTemplate::getClassTypeId()))
        return static_cast<TechDraw::DrawSVGTemplate *>(pageTemplate);
    else
        return 0;
}

void QGISVGTemplate::draw()
{
    TechDraw::DrawSVGTemplate *tmplte = getSVGTemplate();
    if(!tmplte)
        throw Base::RuntimeError("Template Feature not set for QGISVGTemplate");
    load(QString::fromUtf8(tmplte->PageResult.getValue()));
}

void QGISVGTemplate::updateView(bool update)
{
    Q_UNUSED(update);
    draw();
}

void QGISVGTemplate::createClickHandles(void)
{
    TechDraw::DrawSVGTemplate *tmplte = getSVGTemplate();
    std::string temp = tmplte->PageResult.getValue();

    if (temp.empty())
        return;

    Base::FileInfo fi(temp);

    std::ostringstream oStream;
    std::string tempendl = "--endOfLine--";
    std::string line;

    //read all of PageResult into oStream (except the DrawingContent marker comment - why??)
    std::ifstream ifile (fi.filePath().c_str());
    while (std::getline(ifile,line))
    {
        // check if the marker in the template is found
        if(line.find("<!-- DrawingContent -->") == std::string::npos) {
            // if not -  write line to oStream
            oStream << line << tempendl;
        }
    }

    std::string outfragment(oStream.str());

    // Find text tags with freecad:editable attribute and their matching tspans
    // keep tagRegex in sync with App/DrawSVGTemplate.cpp
    boost::regex tagRegex("<text([^>]*freecad:editable=[^>]*)>[^<]*<tspan[^>]*>([^<]*)</tspan>");

    // Smaller regexes for parsing matches to tagRegex
    boost::regex editableNameRegex("freecad:editable=\"(.*?)\"");
    boost::regex xRegex("x=\"([\\d.-]+)\"");
    boost::regex yRegex("y=\"([\\d.-]+)\"");
    //Note: some templates have fancy Transform clauses and don't use absolute x,y to position editableFields.
    //      editableFields will be in the wrong place in this case.

    std::string::const_iterator begin, end;
    begin = outfragment.begin();
    end = outfragment.end();
    boost::match_results<std::string::const_iterator> tagMatch, nameMatch, xMatch, yMatch;

    //TODO: Find location of special fields (first/third angle) and make graphics items for them

    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/General");
    double dotSize = hGrp->GetFloat("TemplateDotSize", 3.0);

    while (boost::regex_search(begin, end, tagMatch, tagRegex)) {
        if ( boost::regex_search(tagMatch[1].first, tagMatch[1].second, nameMatch, editableNameRegex) &&
             boost::regex_search(tagMatch[1].first, tagMatch[1].second, xMatch, xRegex) &&
             boost::regex_search(tagMatch[1].first, tagMatch[1].second, yMatch, yRegex) ) {

            QString xStr = QString::fromStdString(xMatch[1].str());
            QString yStr = QString::fromStdString(yMatch[1].str());

            double x = Rez::guiX(xStr.toDouble());
            double y = Rez::guiX(yStr.toDouble());

            double editClickBoxSize = Rez::guiX(dotSize);
            QColor editClickBoxColor = Qt::green;
            editClickBoxColor.setAlpha(128);              //semi-transparent

            double width = editClickBoxSize;
            double height = editClickBoxSize;

            auto item( new TemplateTextField(this, tmplte, nameMatch[1].str()) );
            float pad = 1;

            item->setRect(x - pad, Rez::guiX(-tmplte->getHeight()) + y - height - pad,
                          width + 2 * pad, height + 2 * pad);
            QPen myPen;
            QBrush myBrush(editClickBoxColor,Qt::SolidPattern);
            myPen.setStyle(Qt::SolidLine);
            myPen.setColor(editClickBoxColor);
            myPen.setWidth(0);  // 0 means "cosmetic pen" - always 1px
            item->setPen(myPen);
            item->setBrush(myBrush);

            item->setZValue(ZVALUE::SVGTEMPLATE + 1);
            addToGroup(item);
            textFields.push_back(item);
        }

        begin = tagMatch[0].second;
    }
}

#include <Mod/TechDraw/Gui/moc_QGISVGTemplate.cpp>
