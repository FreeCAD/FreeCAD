// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
#include <QDomDocument>
#endif

#include "XMLQuery.h"


using namespace TechDraw;

XMLQuery::XMLQuery(QDomDocument& dom)
    : domDocument(dom)
{

}

// A helper function that traverses all child elements recursively and check for
// elements of name "text" with an attribute "freecad:editable"
// If the query string contains "tspan" the first sub-element is used or the
// found text element otherwise.
static bool processElements(const QDomElement& element, const QString& queryStr,
                            const std::function<bool(QDomElement&)>& process)
{
    bool find_tspan = queryStr.contains(QLatin1String("tspan"));
    QDomNodeList editable = element.elementsByTagName(QString(QLatin1String("text")));
    if (editable.count() > 0) {
        for(int i = 0; i < editable.count(); i++) {
            QDomNode node = editable.item(i);
            QDomElement element = node.toElement();
            if (element.hasAttribute(QString(QLatin1String("freecad:editable")))) {
                if (find_tspan) {
                    element = element.firstChildElement();
                }

                if (!process(element)) {
                    return false;
                }
            }
        }
    }
    else {
        QDomElement child;
        child = element.firstChildElement();
        while (!child.isNull()) {
            if (!processElements(child, queryStr, process)) {
                return false;
            }
            child = child.nextSiblingElement();
        }
    }

    return true;
}

bool XMLQuery::processItems(const QString& queryStr, const std::function<bool(QDomElement&)>& process)
{
    // The actual query string is of the form "//text[@freecad:editable]"
    // or "//text[@freecad:editable]/tspan"
    QDomElement root = domDocument.documentElement();
    if (!root.isNull()) {
        processElements(root, queryStr, process);
    }

    return true;
}
