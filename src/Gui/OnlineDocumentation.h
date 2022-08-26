/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef GUI_ONLINEDOCUMENTATION_H
#define GUI_ONLINEDOCUMENTATION_H

#include <QObject>
#include <QTcpServer>
#include "Command.h"


namespace Gui {

/// opens a URL in the system Browser
bool GuiExport OpenURLInBrowser(const char * URL);

/**
 * Returns the content of an HTML page which gets sent to
 * the client to be displayed.
 * @author Werner Mayer
 */
class PythonOnlineHelp : public QObject
{
    Q_OBJECT

public:
    PythonOnlineHelp();
    ~PythonOnlineHelp() override;

    QByteArray loadResource(const QString& filename) const;
    QByteArray fileNotFound() const;
    QByteArray loadFailed(const QString& error) const;
};

/**
 * The HttpServer class implements a simple HTTP server.
 */
class HttpServer : public QTcpServer
{
    Q_OBJECT

public:
    explicit HttpServer(QObject* parent = nullptr);

    void incomingConnection(qintptr socket) override;
    void pause();
    void resume();

private Q_SLOTS:
    void readClient();
    void discardClient();

private:
    PythonOnlineHelp help;
    bool disabled;
};

// --------------------------------------------------------------------

class StdCmdPythonHelp : public Command
{
public:
    StdCmdPythonHelp();
    ~StdCmdPythonHelp() override;
    const char* className() const override
    { return "Gui::StdCmdPythonHelp"; }

protected:
    void activated(int iMsg) override;

private:
    HttpServer* server;
};

}

#endif // GUI_ONLINEDOCUMENTATION_H
