/***************************************************************************
 *   Copyright (c) 2014 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <QCoreApplication>
#include <QTcpSocket>
# include <stdexcept>

#include "Server.h"
#include <Base/Exception.h>
#include <Base/Interpreter.h>

using namespace Web;


ServerEvent::ServerEvent(QTcpSocket* sock, const QByteArray& msg)
  : QEvent(QEvent::User), sock(sock), text(msg)
{
}

ServerEvent::~ServerEvent()
{
}

QTcpSocket* ServerEvent::socket() const
{
    return sock;
}

const QByteArray& ServerEvent::request() const
{
    return text;
}

AppServer::AppServer(QObject* parent)
  : QTcpServer(parent)
{
}

void AppServer::incomingConnection(int socket)
{
    QTcpSocket* s = new QTcpSocket(this);
    connect(s, SIGNAL(readyRead()), this, SLOT(readClient()));
    connect(s, SIGNAL(disconnected()), this, SLOT(discardClient()));
    s->setSocketDescriptor(socket);
}

void AppServer::readClient()
{
    QTcpSocket* socket = (QTcpSocket*)sender();
    if (socket->bytesAvailable() > 0) {
        QByteArray request = socket->readAll();
        QCoreApplication::postEvent(this, new ServerEvent(socket, request));
    }
//    if (socket->state() == QTcpSocket::UnconnectedState) {
//        //mark the socket for deletion but do not destroy immediately
//        socket->deleteLater();
//    }
}

void AppServer::discardClient()
{
    QTcpSocket* socket = (QTcpSocket*)sender();
    socket->deleteLater();
}

void AppServer::customEvent(QEvent* e)
{
    ServerEvent* ev = static_cast<ServerEvent*>(e);
    QByteArray msg = ev->request();
    QTcpSocket* socket = ev->socket();

    std::string str;

    try {
        Base::Interpreter().runString(msg);
    }
    catch (Base::PyException &e) {
        str = e.what();
        str += "\n\n";
        str += e.getStackTrace();
    }
    catch (Base::Exception &e) {
        str = e.what();
    }
    catch (std::exception &e) {
        str = e.what();
    }
    catch (...) {
        str = "Unknown exception thrown";
    }

    socket->write(str.c_str());
    socket->close();
}

#include "moc_Server.cpp"
