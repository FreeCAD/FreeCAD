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

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QApplication>
#include <QBuffer>
#include <QImageWriter>
#include <QMessageBox>
#include <QTcpSocket>
#endif

#include <Base/Interpreter.h>
#include <Base/Exception.h>

#include "OnlineDocumentation.h"
#include "MainWindow.h"


using namespace Gui;

// the favicon
// clang-format off
// NOLINTBEGIN
static const unsigned int navicon_data_len = 318;
static const unsigned char navicon_data[] = {
    0x00,0x00,0x01,0x00,0x01,0x00,0x10,0x10,0x10,0x00,0x01,0x00,0x04,0x00,
    0x28,0x01,0x00,0x00,0x16,0x00,0x00,0x00,0x28,0x00,0x00,0x00,0x10,0x00,
    0x00,0x00,0x20,0x00,0x00,0x00,0x01,0x00,0x04,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0x00,
    0x84,0x82,0x84,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x11,0x11,0x00,
    0x00,0x00,0x00,0x00,0x01,0x10,0x01,0x10,0x00,0x00,0x00,0x00,0x11,0x00,
    0x00,0x10,0x00,0x00,0x00,0x00,0x11,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x11,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x11,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x11,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x11,0x00,0x00,0x10,
    0x00,0x00,0x00,0x00,0x01,0x10,0x01,0x10,0x00,0x20,0x00,0x00,0x00,0x11,
    0x11,0x00,0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,
    0x00,0x00,0x00,0x00,0x02,0x22,0x22,0x20,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0xff,0xff,0x00,0x00,0xfc,0x3f,0x00,0x00,0xf9,0x9f,0x00,0x00,
    0x93,0xdf,0x00,0x00,0x93,0xff,0x00,0x00,0x93,0xff,0x00,0x00,0x93,0xff,
    0x00,0x00,0x93,0xfd,0x00,0x00,0x81,0xd8,0x00,0x00,0x99,0x9d,0x00,0x00,
    0x9c,0x3d,0x00,0x00,0x9f,0xfd,0x00,0x00,0x80,0xfd,0x00,0x00,0xff,0x7d,
    0x00,0x00,0xfe,0x01,0x00,0x00,0xff,0x7f,0x00,0x00};
// NOLINTEND
// clang-format on

PythonOnlineHelp::PythonOnlineHelp() = default;

PythonOnlineHelp::~PythonOnlineHelp() = default;

QByteArray PythonOnlineHelp::loadResource(const QString& filename) const
{
    if (filename == QLatin1String("/favicon.ico")) {
        return loadFavicon();
    }

    if (filename == QLatin1String("/")) {
        return loadIndexPage();
    }

    return loadHelpPage(filename);
}

QByteArray PythonOnlineHelp::loadFavicon() const
{
    // Return a resource icon in ico format
    QByteArray res;
    QBuffer buffer;
    buffer.open(QBuffer::WriteOnly);
    QImageWriter writer;
    writer.setDevice(&buffer);
    writer.setFormat("ICO");
    if (writer.canWrite()) {
        const int size = 24;
        QPixmap px = qApp->windowIcon().pixmap(size, size);  // NOLINT
        writer.write(px.toImage());
        buffer.close();
        res = buffer.data();
    }
    else {
        // fallback
        res.reserve(navicon_data_len);
        for (int i = 0; i < (int)navicon_data_len; i++) {
            res[i] = navicon_data[i];
        }
    }

    return res;
}

QByteArray PythonOnlineHelp::invoke(const std::function<std::string(Py::Module&)>& func) const
{
    // get the global interpreter lock otherwise the app may crash with the error
    // 'PyThreadState_Get: no current thread' (see pystate.c)
    Base::PyGILStateLocker lock;
    try {
        return tryInvoke(func);
    }
    catch (const Py::Exception&) {
        // load the error page
        Base::PyException e;
        e.ReportException();
        return loadFailed(QString::fromUtf8(e.what()));
    }
}

QByteArray PythonOnlineHelp::tryInvoke(const std::function<std::string(Py::Module&)>& func) const
{
    PyObject* module = PyImport_ImportModule("freecad.freecad_doc");
    if (!module) {
        throw Py::Exception();
    }

    Py::Module mod(module, true);
    std::string contents = func(mod);
    QByteArray res;
    res.append("HTTP/1.0 200 OK\n");
    res.append("Content-type: text/html\n");
    res.append(contents.c_str());
    return res;
}

QByteArray PythonOnlineHelp::loadIndexPage() const
{
    return invoke([](Py::Module& mod) {
        Py::String output(mod.callMemberFunction("getIndex"));
        return static_cast<std::string>(output);
    });
}

QByteArray PythonOnlineHelp::loadHelpPage(const QString& filename) const
{
    return invoke([filename](Py::Module& mod) {
        QString fn = filename.mid(1);
        QString name = fn.left(fn.length() - 5);

        Py::Tuple args(1);
        args.setItem(0, Py::String(name.toStdString()));
        Py::String output(mod.callMemberFunction("getPage", args));
        return static_cast<std::string>(output);
    });
}

QByteArray PythonOnlineHelp::fileNotFound() const
{
    const int pageNotFound = 404;
    QString contentType = QString::fromLatin1(
        "text/html\r\n"
        "\r\n"
        "<html><head><title>Error</title></head>"
        "<body bgcolor=\"#f0f0f8\">"
        "<table width=\"100%\" cellspacing=0 cellpadding=2 border=0 summary=\"heading\">"
        "<tr bgcolor=\"#7799ee\">"
        "<td valign=bottom>&nbsp;<br>"
        "<font color=\"#ffffff\" face=\"helvetica, arial\">&nbsp;<br><big><big><strong>FreeCAD "
        "Documentation</strong></big></big></font></td>"
        "<td align=right valign=bottom>"
        "<font color=\"#ffffff\" face=\"helvetica, arial\">&nbsp;</font></td></tr></table>"
        "<p><p>"
        "<h1>404 - File not found</h1>"
        "<div><p><strong>The requested URL was not found on this server."
        "</strong></p>"
        "</div></body>"
        "</html>"
        "\r\n");

    QString header = QString::fromLatin1("content-type: %1\r\n").arg(contentType);

    QString http(QLatin1String("HTTP/1.1 %1 %2\r\n%3\r\n"));
    QString httpResponseHeader =
        http.arg(pageNotFound).arg(QString::fromLatin1("File not found"), header);

    QByteArray res = httpResponseHeader.toLatin1();
    return res;
}

QByteArray PythonOnlineHelp::loadFailed(const QString& error) const
{
    const int pageNotFound = 404;
    QString contentType =
        QString::fromLatin1(
            "text/html\r\n"
            "\r\n"
            "<html><head><title>Error</title></head>"
            "<body bgcolor=\"#f0f0f8\">"
            "<table width=\"100%\" cellspacing=0 cellpadding=2 border=0 summary=\"heading\">"
            "<tr bgcolor=\"#7799ee\">"
            "<td valign=bottom>&nbsp;<br>"
            "<font color=\"#ffffff\" face=\"helvetica, arial\">&nbsp;<br><big><big><strong>FreeCAD "
            "Documentation</strong></big></big></font></td>"
            "<td align=right valign=bottom>"
            "<font color=\"#ffffff\" face=\"helvetica, arial\">&nbsp;</font></td></tr></table>"
            "<p><p>"
            "<h1>%1</h1>"
            "</body>"
            "</html>"
            "\r\n")
            .arg(error);

    QString header = QString::fromLatin1("content-type: %1\r\n").arg(contentType);

    QString http(QLatin1String("HTTP/1.1 %1 %2\r\n%3\r\n"));
    QString httpResponseHeader =
        http.arg(pageNotFound).arg(QString::fromLatin1("File not found"), header);

    QByteArray res = httpResponseHeader.toLatin1();
    return res;
}

HttpServer::HttpServer(QObject* parent)
    : QTcpServer(parent)
    , disabled(false)
{}

void HttpServer::incomingConnection(qintptr socket)
{
    if (disabled) {
        return;
    }

    // When a new client connects the server constructs a QTcpSocket and all
    // communication with the client is done over this QTcpSocket. QTcpSocket
    // works asynchronously, this means that all the communication is done
    // in the two slots readClient() and discardClient().
    auto s = new QTcpSocket(this);
    connect(s, &QTcpSocket::readyRead, this, &HttpServer::readClient);
    connect(s, &QTcpSocket::disconnected, this, &HttpServer::discardClient);
    s->setSocketDescriptor(socket);
}

void HttpServer::pause()
{
    disabled = true;
}

void HttpServer::resume()
{
    disabled = false;
}

void HttpServer::readClient()
{
    if (disabled) {
        return;
    }

    // This slot is called when the client sent data to the server. The
    // server looks if it was a GET request and  sends back the
    // corresponding HTML document from the ZIP file.
    auto socket = qobject_cast<QTcpSocket*>(sender());
    if (socket && socket->canReadLine()) {
        // NOLINTBEGIN
        QString httpRequestHeader = QString::fromLatin1(socket->readLine());
        QStringList lst = httpRequestHeader.simplified().split(QLatin1String(" "));
        QString method;
        QString path;
        if (lst.count() > 0) {
            QString m = lst[0];
            if (lst.count() > 1) {
                QString p = lst[1];
                if (lst.count() > 2) {
                    QString v = lst[2];
                    if (v.length() >= 8 && v.left(5) == QLatin1String("HTTP/") && v[5].isDigit()
                        && v[6] == QLatin1Char('.') && v[7].isDigit()) {
                        method = m;
                        path = p;
                    }
                }
            }
        }
        // NOLINTEND

        if (method == QLatin1String("GET")) {
            socket->write(help.loadResource(path));
            socket->close();
            if (socket->state() == QTcpSocket::UnconnectedState) {
                // mark the socket for deletion but do not destroy immediately
                socket->deleteLater();
            }
        }
    }
}

void HttpServer::discardClient()
{
    if (auto socket = qobject_cast<QTcpSocket*>(sender())) {
        socket->deleteLater();
    }
}

// --------------------------------------------------------------------

/* TRANSLATOR Gui::StdCmdPythonHelp */

StdCmdPythonHelp::StdCmdPythonHelp()
    : Command("Std_PythonHelp")
    , server(nullptr)
{
    sGroup = "Tools";
    sMenuText = QT_TR_NOOP("Automatic Python Modules Documentation");
    sToolTipText = QT_TR_NOOP("Opens the Python Modules documentation");
    sWhatsThis = "Std_PythonHelp";
    sStatusTip = sToolTipText;
    sPixmap = "applications-python";
}

StdCmdPythonHelp::~StdCmdPythonHelp()
{
    if (server) {
        server->close();
        delete server;
    }
}

void StdCmdPythonHelp::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // try to open a connection over this port
    const qint16 port = 7465;
    if (!this->server) {
        this->server = new HttpServer();
    }

    // if server is not yet running try to open one
    if (this->server->isListening()
        || this->server->listen(QHostAddress(QHostAddress::LocalHost), port)) {
        std::string url = "http://localhost:";
        url += std::to_string(port);
        OpenURLInBrowser(url.c_str());
    }
    else {
        QMessageBox::critical(Gui::getMainWindow(),
                              QObject::tr("No Server"),
                              QObject::tr("Unable to start the server to port %1: %2.")
                                  .arg(port)
                                  .arg(server->errorString()));
    }
}

bool Gui::OpenURLInBrowser(const char* URL)
{
    // The webbrowser Python module allows to start the system browser in an OS-independent way
    Base::PyGILStateLocker lock;
    try {
        PyObject* module = PyImport_ImportModule("webbrowser");
        if (module) {
            Py::Module mod(module, true);
            mod.callMemberFunction("open", Py::TupleN(Py::String(URL)));
            return true;
        }

        throw Py::Exception();
    }
    catch (Py::Exception& e) {
        e.clear();
        QMessageBox::critical(Gui::getMainWindow(),
                              QObject::tr("No Browser"),
                              QObject::tr("Unable to open your system browser."));
        return false;
    }
}


#include "moc_OnlineDocumentation.cpp"
