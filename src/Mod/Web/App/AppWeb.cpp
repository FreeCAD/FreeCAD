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
#include <sstream>
#endif

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/PyObjectBase.h>

#include "Server.h"


// See http://docs.python.org/2/library/socketserver.html
/*
import socket
import threading


ip = "127.0.0.1"
port = 54880

def client(ip, port, message):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((ip, port))
    try:
        sock.sendall(message)
        response = sock.recv(1024)
        print ("Received: {}".format(response))
    finally:
        sock.close()



client(ip, port, b"print ('Hello World')")
client(ip, port, b"import FreeCAD\nFreeCAD.newDocument()")

*/

namespace Web
{
class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("Web")
    {
        add_varargs_method("startServer",
                           &Module::startServer,
                           "startServer(address=127.0.0.1,port=0) -- Start a server.");
        add_varargs_method("waitForConnection",
                           &Module::waitForConnection,
                           "waitForConnection(address=127.0.0.1,port=0,timeout=0)\n"
                           "Start a server, wait for connection and close server.\n"
                           "Its use is disadvised in a the GUI version, since it will\n"
                           "stop responding until the function returns.");
        add_varargs_method("registerServerFirewall",
                           &Module::registerServerFirewall,
                           "registerServerFirewall(callable(string)) -- Register a firewall.");
        initialize("This module is the Web module.");  // register with Python
    }

private:
    Py::Object startServer(const Py::Tuple& args)
    {
        const char* addr = "127.0.0.1";
        int port = 0;
        if (!PyArg_ParseTuple(args.ptr(), "|si", &addr, &port)) {
            throw Py::Exception();
        }
        if (port > USHRT_MAX) {
            throw Py::OverflowError("port number is greater than maximum");
        }
        else if (port < 0) {
            throw Py::OverflowError("port number is lower than 0");
        }

        AppServer* server = new AppServer();
        if (server->listen(QHostAddress(QString::fromLatin1(addr)), port)) {
            QString a = server->serverAddress().toString();
            quint16 p = server->serverPort();
            Py::Tuple t(2);
            t.setItem(0, Py::String((const char*)a.toLatin1()));
            t.setItem(1, Py::Long(p));
            return t;
        }
        else {
            server->deleteLater();
            std::stringstream out;
            out << "Server failed to listen at address " << addr << " and port " << port;
            throw Py::RuntimeError(out.str());
        }
    }

    Py::Object waitForConnection(const Py::Tuple& args)
    {
        const char* addr = "127.0.0.1";
        int port = 0;
        int timeout = 0;
        if (!PyArg_ParseTuple(args.ptr(), "|sii", &addr, &port, &timeout)) {
            throw Py::Exception();
        }
        if (port > USHRT_MAX) {
            throw Py::OverflowError("port number is greater than maximum");
        }
        else if (port < 0) {
            throw Py::OverflowError("port number is lower than 0");
        }

        try {
            AppServer server(true);
            if (server.listen(QHostAddress(QString::fromLatin1(addr)), port)) {
                bool ok = server.waitForNewConnection(timeout);
                QTcpSocket* socket = server.nextPendingConnection();
                if (socket) {
                    socket->waitForReadyRead();
                }

                server.close();
                return Py::Boolean(ok);
            }
            else {
                std::stringstream out;
                out << "Server failed to listen at address " << addr << " and port " << port;
                throw Py::RuntimeError(out.str());
            }
        }
        catch (const Base::SystemExitException& e) {
            throw Py::RuntimeError(e.what());
        }
    }

    Py::Object registerServerFirewall(const Py::Tuple& args)
    {
        PyObject* obj;
        if (!PyArg_ParseTuple(args.ptr(), "O", &obj)) {
            throw Py::Exception();
        }

        Py::Object pyobj(obj);
        if (pyobj.isNone()) {
            Web::Firewall::setInstance(nullptr);
        }
        else {
            Web::Firewall::setInstance(new Web::FirewallPython(pyobj));
        }

        return Py::None();
    }
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

}  // namespace Web


/* Python entry */
PyMOD_INIT_FUNC(Web)
{

    // ADD YOUR CODE HERE
    //
    //
    PyObject* mod = Web::initModule();
    Base::Console().Log("Loading Web module... done\n");
    PyMOD_Return(mod);
}
