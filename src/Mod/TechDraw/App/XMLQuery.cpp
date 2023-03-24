/***************************************************************************
 *   Copyright (c) 2023 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QDomDocument>
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
# include "QDomNodeModel.h"
# include <QXmlQuery>
# include <QXmlResultItems>
#endif
#endif

#include "XMLQuery.h"


using namespace TechDraw;

XMLQuery::XMLQuery(QDomDocument& dom)
    : domDocument(dom)
{

}

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
bool XMLQuery::processItems(const QString& queryStr, const std::function<bool(QDomElement&)>& process)
{
    QXmlQuery query(QXmlQuery::XQuery10);
    QDomNodeModel model(query.namePool(), domDocument);
    QDomElement symbolDocElem = domDocument.documentElement();
    query.setFocus(QXmlItem(model.fromDomNode(symbolDocElem)));

    query.setQuery(queryStr);
    QXmlResultItems queryResult;
    query.evaluateTo(&queryResult);

    while (!queryResult.next().isNull()) {
        QDomElement tspanElement =
            model.toDomNode(queryResult.current().toNodeModelIndex()).toElement();
        if (!process(tspanElement)) {
            return false;
        }
    }

    return true;
}
#else
bool XMLQuery::processItems(const QString& queryStr, const std::function<bool(QDomElement&)>& process)
{
    //TODO: Port to Qt6
    Q_UNUSED(queryStr)
    Q_UNUSED(process)
    return false;
}
#endif
