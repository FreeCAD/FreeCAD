// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2026 David Carter <dcarter@david.carter.ca>             *
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

#include <QFile>

#include <Base/Console.h>

#include "XMLEventSource.h"

using namespace QtTesting;

XMLEventSource::XMLEventSource(QObject* p)
    : pqEventSource(p)
{
    xmlStream = NULL;
}

XMLEventSource::~XMLEventSource()
{
    delete xmlStream;
}

void XMLEventSource::setContent(const QString& xmlfilename)
{
    delete xmlStream;
    xmlStream = NULL;

    QFile xml(xmlfilename);
    if (!xml.open(QIODevice::ReadOnly)) {
        Base::Console().log("Failed to load %s\n", xmlfilename.toStdString().c_str());
        return;
    }
    QByteArray data = xml.readAll();
    xmlStream = new QXmlStreamReader(data);
    /* This checked for valid event objects, but also caused the first event
        * to get dropped. Commenting this out in the example. If you wish to report
        * empty XML test files a flag indicating whether valid events were found is
        * probably the best way to go.
    while (!xmlStream->atEnd())
        {
        QXmlStreamReader::TokenType token = xmlStream->readNext();
        if (token == QXmlStreamReader::StartElement)
        {
        if (xmlStream->name() == "event")
            {
            break;
            }
        }
        } */
    if (xmlStream->atEnd()) {
        Base::Console().log("Invalid xml\n");
    }
    return;
}

int XMLEventSource::getNextEvent(QString& widget, QString& command, QString& arguments, int& eventType)
{
    if (xmlStream->atEnd()) {
        return DONE;
    }
    while (!xmlStream->atEnd()) {
        QXmlStreamReader::TokenType token = xmlStream->readNext();
        if (token == QXmlStreamReader::StartElement) {
            if (xmlStream->name() == QStringLiteral("event")) {
                break;
            }
        }
    }
    if (xmlStream->atEnd()) {
        return DONE;
    }

    // Check for the "property" attribute to determine if this is a CHECK_EVENT or an ACTION_EVENT
    auto property = xmlStream->attributes().value(QStringLiteral("property")).toString();
    if (!property.isEmpty()) {
        eventType = pqEventTypes::CHECK_EVENT;
        widget = xmlStream->attributes().value(QStringLiteral("widget")).toString();
        command = property;
        arguments = xmlStream->attributes().value(QStringLiteral("arguments")).toString();
        return SUCCESS;
    }

    eventType = pqEventTypes::ACTION_EVENT;
    widget = xmlStream->attributes().value(QStringLiteral("widget")).toString();
    command = xmlStream->attributes().value(QStringLiteral("command")).toString();
    arguments = xmlStream->attributes().value(QStringLiteral("arguments")).toString();
    return SUCCESS;
}
