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
#endif

#include <Python.h>
#include <climits>

#include <Base/Console.h>
#include <Base/PyObjectBase.h>
#include <Base/Exception.h>
#include <CXX/Objects.hxx>
#include "Server.h"

using namespace Web;


// See http://docs.python.org/2/library/socketserver.html
/*
import socket
import threading
import SocketServer


ip = "127.0.0.1"
port=54880

def client(ip, port, message):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((ip, port))
    try:
        sock.sendall(message)
        response = sock.recv(1024)
        print "Received: {}".format(response)
    finally:
        sock.close()



client(ip, port, "print 'Hello World 1'")
client(ip, port, "import FreeCAD\nFreeCAD.newDocument()")
client(ip, port, "Hello World 3\n")

*/

/* module functions */
static PyObject * startServer(PyObject *self, PyObject *args)
{
    const char* addr = "127.0.0.1";
    int port=0;
    if (!PyArg_ParseTuple(args, "|si",&addr,&port))
        return NULL;
    if (port > USHRT_MAX) {
        PyErr_SetString(PyExc_OverflowError, "port number is greater than maximum");
        return 0;
    }
    else if (port < 0) {
        PyErr_SetString(PyExc_OverflowError, "port number is lower than 0");
        return 0;
    }

    PY_TRY {
        AppServer* server = new AppServer();

        if (server->listen(QHostAddress(QString::fromLatin1(addr)), port)) {
            QString a = server->serverAddress().toString();
            quint16 p = server->serverPort();
            Py::Tuple t(2);
            t.setItem(0, Py::String((const char*)a.toLatin1()));
            t.setItem(1, Py::Int(p));
            return Py::new_reference_to(t);
        }
        else {
            server->deleteLater();
            PyErr_Format(PyExc_RuntimeError, "Server failed to listen at address %s and port %d", addr, port);
            return 0;
        }
    } PY_CATCH;

    Py_Return;
}

/* registration table  */
struct PyMethodDef Web_methods[] = {
    {"startServer",startServer      ,METH_VARARGS,
     "startServer(address=127.0.0.1,port=0) -- Start a server."},
    {NULL, NULL}        /* end of table marker */
};
