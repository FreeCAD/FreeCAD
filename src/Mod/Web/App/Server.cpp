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
#ifndef _PreComp_
#include <QCoreApplication>
#include <QTcpSocket>
#include <memory>
#include <stdexcept>
#endif

#include <Base/Exception.h>
#include <Base/Interpreter.h>

#include "Server.h"


using namespace Web;

Firewall* Firewall::instance = nullptr;

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

Firewall::Firewall() = default;

Firewall::~Firewall() = default;

bool Firewall::filter(const QByteArray&) const
{
    return true;
}

FirewallPython::FirewallPython(const Py::Object& o)
    : obj(o)
{}

FirewallPython::~FirewallPython() = default;

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
    : QEvent(QEvent::User)
    , sock(sock)
    , text(msg)
{}

ServerEvent::~ServerEvent() = default;

QTcpSocket* ServerEvent::socket() const
{
    return sock;
}

const QByteArray& ServerEvent::request() const
{
    return text;
}

// ----------------------------------------------------------------------------

AppServer::AppServer(bool direct, QObject* parent)
    : QTcpServer(parent)
    , direct(direct)
{
    PyObject* mod = PyImport_ImportModule("__main__");
    if (mod) {
        module = mod;
    }
    else {
        throw Py::RuntimeError("Cannot load __main__ module");
    }
}

void AppServer::incomingConnection(qintptr socket)
{
    QTcpSocket* tcpSocket = new QTcpSocket(this);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &AppServer::readClient);
    connect(tcpSocket, &QTcpSocket::disconnected, this, &AppServer::discardClient);
    tcpSocket->setSocketDescriptor(socket);
    addPendingConnection(tcpSocket);
}

void AppServer::readClient()
{
    QTcpSocket* socket = static_cast<QTcpSocket*>(sender());
    if (socket->bytesAvailable() > 0) {
        QByteArray request = socket->readAll();
        std::unique_ptr<ServerEvent> event(std::make_unique<ServerEvent>(socket, request));
        if (direct) {
            customEvent(event.get());
        }
        else {
            QCoreApplication::postEvent(this, event.release());
        }
    }
    //    if (socket->state() == QTcpSocket::UnconnectedState) {
    //        //mark the socket for deletion but do not destroy immediately
    //        socket->deleteLater();
    //    }
}

void AppServer::discardClient()
{
    QTcpSocket* socket = static_cast<QTcpSocket*>(sender());
    socket->deleteLater();
}

void AppServer::customEvent(QEvent* e)
{
    ServerEvent* ev = static_cast<ServerEvent*>(e);
    QByteArray msg = ev->request();
    QTcpSocket* socket = ev->socket();

    std::string str = handleRequest(msg);
    socket->write(str.c_str());
    if (direct) {
        socket->waitForBytesWritten();
    }
    socket->close();
}

std::string AppServer::handleRequest(QByteArray msg)
{
    std::string str;
    if (msg.startsWith("GET ")) {
        msg = QByteArray("GET = ") + msg.mid(4);
        str = runPython(msg);
        if (str == "None") {
            str = getRequest(str);
        }
    }
    else {
        str = runPython(msg);
    }

    return str;
}

std::string AppServer::getRequest(const std::string& str) const
{
    try {
        Base::PyGILStateLocker lock;
        Py::Object attr = module.getAttr(std::string("GET"));
        return attr.as_string();
    }
    catch (Py::Exception& e) {
        e.clear();
        return str;
    }
}

std::string AppServer::runPython(const QByteArray& msg)
{
    std::string str;

    try {
        Firewall* fw = Firewall::getInstance();
        if (!fw || fw->filter(msg)) {
            str = Base::Interpreter().runString(msg);
        }
        else {
            str = "Command blocked";
        }
    }
    catch (Base::PyException& e) {
        str = e.what();
        str += "\n\n";
        str += e.getStackTrace();
    }
    catch (Base::SystemExitException&) {
        throw;
    }
    catch (Base::Exception& e) {
        str = e.what();
    }
    catch (std::exception& e) {
        str = e.what();
    }
    catch (...) {
        str = "Unknown exception thrown";
    }

    return str;
}

#include "moc_Server.cpp"
