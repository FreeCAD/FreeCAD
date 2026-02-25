/***************************************************************************
 *   Copyright (c) 2018 Torsten Sadowski <tsadowski[at]gmx.net>            *
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

#pragma once

#include <QAbstractNativeEventFilter>

namespace Gui
{
class RawInputEventFilter: public QAbstractNativeEventFilter
{
public:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    using EventFilter = bool (*)(void* message, long* result);
#else
    using EventFilter = bool (*)(void* message, qintptr* result);
#endif
    RawInputEventFilter(EventFilter filter)
        : eventFilter(filter)
    {}
    virtual ~RawInputEventFilter()
    {}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    virtual bool nativeEventFilter(const QByteArray& /*eventType*/, void* message, long* result)
    {
        return eventFilter(message, result);
    }
#else
    virtual bool nativeEventFilter(const QByteArray& /*eventType*/, void* message, qintptr* result)
    {
        return eventFilter(message, result);
    }
#endif

private:
    EventFilter eventFilter;
};
}  // namespace Gui
