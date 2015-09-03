/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef GUI_AUTOSAVER_H
#define GUI_AUTOSAVER_H

#include <QObject>
#include <map>
#include <string>

namespace App {
class Document;
}

namespace Gui {

/*!
 The class AutoSaver is used to automatically save a document to a temporary file.
 @author Werner Mayer
 */
class AutoSaver : public QObject
{
    Q_OBJECT

private:
    static AutoSaver* self;
    AutoSaver(QObject* parent);
    virtual ~AutoSaver();

public:
    static AutoSaver* instance();
    /*!
     Sets the timeout in milliseconds. A value of 0 means that no timer is used.
     */
    void setTimeout(int ms);

protected:
    void slotCreateDocument(const App::Document& Doc);
    void slotDeleteDocument(const App::Document& Doc);
    void timerEvent(QTimerEvent * event);
    void saveDocument(const std::string&);

private:
    int timeout; /*!< Timeout in milliseconds */
    std::map<std::string, int> timerMap;
};

} //namespace Gui


#endif //GUI_AUTOSAVER_H
