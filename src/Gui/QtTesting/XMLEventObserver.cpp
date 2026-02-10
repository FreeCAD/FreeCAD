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

#include <QTextStream>

#include <pqEventTypes.h>

#include <Base/Console.h>

#include "XMLEventObserver.h"

using namespace QtTesting;

XMLEventObserver::XMLEventObserver(QObject* p)
    : pqEventObserver(p)
{
    xmlStream = NULL;
}

XMLEventObserver::~XMLEventObserver()
{
    delete xmlStream;
}

void XMLEventObserver::setStream(QTextStream* stream)
{
    if (xmlStream) {
        xmlStream->writeEndElement();
        xmlStream->writeEndDocument();
        delete xmlStream;
        xmlStream = NULL;
    }
    if (this->Stream) {
        *this->Stream << xmlString;
    }
    xmlString = QString();
    pqEventObserver::setStream(stream);
    if (this->Stream) {
        xmlStream = new QXmlStreamWriter(&xmlString);
        xmlStream->setAutoFormatting(true);
        xmlStream->writeStartDocument();
        xmlStream->writeStartElement(QStringLiteral("events"));
    }
}

void XMLEventObserver::onRecordEvent(
    const QString& widget,
    const QString& command,
    const QString& arguments,
    const int& eventType
)
{
    if (xmlStream) {
        xmlStream->writeStartElement(QStringLiteral("event"));
        xmlStream->writeAttribute(QStringLiteral("widget"), widget);
        if (eventType == pqEventTypes::ACTION_EVENT) {
            xmlStream->writeAttribute(QStringLiteral("command"), command);
        }
        else  // if(eventType == pqEventTypes::CHECK_EVENT)
        {
            xmlStream->writeAttribute(QStringLiteral("property"), command);
        }
        xmlStream->writeAttribute(QStringLiteral("arguments"), arguments);
        xmlStream->writeEndElement();
    }
}
