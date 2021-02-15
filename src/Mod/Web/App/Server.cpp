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
#include <stdexcept>

#include "Server.h"
#include <Base/Exception.h>
#include <Base/Interpreter.h>

using namespace Web;

Firewall* Firewall::instance = 0;

Firewall* Firewall::getInstance()
{
    return instance;
}

void Firewall::setInstance(Firewall* inst)
{
    if (inst != instance) {
        delete instance;
        instance = inst;
    }
}

Firewall::Firewall()
{
}

Firewall::~Firewall()
{
}

bool Firewall::filter(const QByteArray&) const
{
    return true;
}

FirewallPython::FirewallPython(const Py::Object& o)
  : obj(o)
{
}

FirewallPython::~FirewallPython()
{
}

bool FirewallPython::filter(const QByteArray& msg) const
{
    Base::PyGILStateLocker lock;
    try {
        Py::Callable call(obj);
        Py::Tuple args(1);
        args.setItem(0, Py::String(msg.constData()));
        Py::Boolean ok(call.apply(args));
        return static_cast<bool>(ok);
    }
    catch (const Py::Exception&) {
        Base::PyException e;
        throw Base::RuntimeError(e.what());
    }
}

// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------

AppServer::AppServer(QObject* parent)
  : QTcpServer(parent)
{
}

#if QT_VERSION >=0x050000
void AppServer::incomingConnection(qintptr socket)
#else
void AppServer::incomingConnection(int socket)
#endif
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

    std::string str = runPython(msg);

    socket->write(str.c_str());
    socket->close();
}

std::string AppServer::runPython(const QByteArray& msg)
{
    std::string str;

    try {
        Firewall* fw = Firewall::getInstance();
        if (!fw || fw->filter(msg))
            str = Base::Interpreter().runString(msg);
        else
            str = "Command blocked";
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

    return str;
}

#include "moc_Server.cpp"
