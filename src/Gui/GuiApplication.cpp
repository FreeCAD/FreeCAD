/***************************************************************************
 *   Copyright (c) 2015 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <sstream>
# include <stdexcept>
# include <QByteArray>
# include <QDataStream>
# include <QDebug>
# include <QFileInfo>
# include <QFileOpenEvent>
# include <QSessionManager>
# include <QTimer>
#endif

#include <QLocalServer>
#include <QLocalSocket>

#if defined(Q_OS_WIN)
# include <Windows.h>
#endif
#if defined(Q_OS_UNIX)
# include <sys/types.h>
# include <time.h>
# include <unistd.h>
#endif

#include "GuiApplication.h"
#include "Application.h"
#include "SpaceballEvent.h"
#include "MainWindow.h"

#include <Base/Console.h>
#include <Base/Exception.h>

#include <App/Application.h>

using namespace Gui;

GUIApplication::GUIApplication(int & argc, char ** argv)
    : GUIApplicationNativeEventAware(argc, argv)
{
#if QT_VERSION > 0x050000
    // In Qt 4.x 'commitData' is a virtual method
    connect(this, SIGNAL(commitDataRequest(QSessionManager &)),
            SLOT(commitData(QSessionManager &)), Qt::DirectConnection);
#endif
#if QT_VERSION >= 0x050600
    setFallbackSessionManagementEnabled(false);
#endif
}

GUIApplication::~GUIApplication()
{
}

bool GUIApplication::notify (QObject * receiver, QEvent * event)
{
    if (!receiver) {
        Base::Console().Log("GUIApplication::notify: Unexpected null receiver, event type: %d\n",
            (int)event->type());
        return false;
    }
    try {
        if (event->type() == Spaceball::ButtonEvent::ButtonEventType || 
            event->type() == Spaceball::MotionEvent::MotionEventType)
            return processSpaceballEvent(receiver, event);
        else
            return QApplication::notify(receiver, event);
    }
    catch (const Base::SystemExitException &e) {
        caughtException.reset(new Base::SystemExitException(e));
        qApp->exit(e.getExitCode());
        return true;
    }
    catch (const Base::Exception& e) {
        Base::Console().Error("Unhandled Base::Exception caught in GUIApplication::notify.\n"
                              "The error message is: %s\n", e.what());
    }
    catch (const std::exception& e) {
        Base::Console().Error("Unhandled std::exception caught in GUIApplication::notify.\n"
                              "The error message is: %s\n", e.what());
    }
    catch (...) {
        Base::Console().Error("Unhandled unknown exception caught in GUIApplication::notify.\n");
    }

    // Print some more information to the log file (if active) to ease bug fixing
    try {
        std::stringstream dump;
        dump << "The event type " << (int)event->type() << " was sent to "
             << receiver->metaObject()->className() << "\n";
        dump << "Object tree:\n";
        if (receiver->isWidgetType()) {
            QWidget* w = qobject_cast<QWidget*>(receiver);
            while (w) {
                dump << "\t";
                dump << w->metaObject()->className();
                QString name = w->objectName();
                if (!name.isEmpty())
                    dump << " (" << (const char*)name.toUtf8() << ")";
                w = w->parentWidget();
                if (w)
                    dump << " is child of\n";
            }
            std::string str = dump.str();
            Base::Console().Log("%s",str.c_str());
        }
    }
    catch (...) {
        Base::Console().Log("Invalid recipient and/or event in GUIApplication::notify\n");
    }

    return true;
}

void GUIApplication::commitData(QSessionManager &manager)
{
    if (manager.allowsInteraction()) {
        if (!Gui::getMainWindow()->close()) {
            // cancel the shutdown
            manager.release();
            manager.cancel();
        }
    }
    else {
        // no user interaction allowed, thus close all documents and
        // the main window
        App::GetApplication().closeAllDocuments();
        Gui::getMainWindow()->close();
    }
}

bool GUIApplication::event(QEvent * ev)
{
    if (ev->type() == QEvent::FileOpen) {
        QString file = static_cast<QFileOpenEvent*>(ev)->file();
        QFileInfo fi(file);
        if (fi.suffix().toLower() == QLatin1String("fcstd")) {
            QByteArray fn = file.toUtf8();
            Application::Instance->open(fn, "FreeCAD");
            return true;
        }
    }

    return GUIApplicationNativeEventAware::event(ev);
}

// ----------------------------------------------------------------------------

class GUISingleApplication::Private {
public:
    Private(GUISingleApplication *q_ptr)
      : q_ptr(q_ptr)
      , timer(new QTimer(q_ptr))
      , server(0)
      , running(false)
    {
        timer->setSingleShot(true);
        std::string exeName = App::GetApplication().getExecutableName();
        serverName = QString::fromStdString(exeName);
    }

    ~Private()
    {
        if (server)
            server->close();
        delete server;
    }

    void setupConnection()
    {
        QLocalSocket socket;
        socket.connectToServer(serverName);
        if (socket.waitForConnected(1000)) {
            this->running = true;
        }
        else {
            startServer();
        }
    }

    void startServer()
    {
        // Start a QLocalServer to listen for connections
        server = new QLocalServer();
        QObject::connect(server, SIGNAL(newConnection()),
                         q_ptr, SLOT(receiveConnection()));
        // first attempt
        if (!server->listen(serverName)) {
            if (server->serverError() == QAbstractSocket::AddressInUseError) {
                // second attempt
                server->removeServer(serverName);
                server->listen(serverName);
            }
        }
        if (server->isListening()) {
            Base::Console().Log("Local server '%s' started\n", qPrintable(serverName));
        }
        else {
            Base::Console().Log("Local server '%s' failed to start\n", qPrintable(serverName));
        }
    }

    GUISingleApplication *q_ptr;
    QTimer *timer;
    QLocalServer *server;
    QString serverName;
    QList<QByteArray> messages;
    bool running;
};

GUISingleApplication::GUISingleApplication(int & argc, char ** argv)
    : GUIApplication(argc, argv),
      d_ptr(new Private(this))
{
    d_ptr->setupConnection();
    connect(d_ptr->timer, SIGNAL(timeout()), this, SLOT(processMessages()));
}

GUISingleApplication::~GUISingleApplication()
{
}

bool GUISingleApplication::isRunning() const
{
    return d_ptr->running;
}

bool GUISingleApplication::sendMessage(const QByteArray &message, int timeout)
{
    QLocalSocket socket;
    bool connected = false;
    for(int i = 0; i < 2; i++) {
        socket.connectToServer(d_ptr->serverName);
        connected = socket.waitForConnected(timeout/2);
        if (connected || i > 0)
            break;
        int ms = 250;
#if defined(Q_OS_WIN)
        Sleep(DWORD(ms));
#else
        usleep(ms*1000);
#endif
    }
    if (!connected)
        return false;

    QDataStream ds(&socket);
    ds << message;
    socket.waitForBytesWritten(timeout);
    return true;
}

void GUISingleApplication::receiveConnection()
{
    QLocalSocket *socket = d_ptr->server->nextPendingConnection();
    if (!socket)
        return;

    connect(socket, SIGNAL(disconnected()),
            socket, SLOT(deleteLater()));
    if (socket->waitForReadyRead()) {
        QDataStream in(socket);
        if (!in.atEnd()) {
            d_ptr->timer->stop();
            QByteArray message;
            in >> message;
            Base::Console().Log("Received message: %s\n", message.constData());
            d_ptr->messages.push_back(message);
            d_ptr->timer->start(1000);
        }
    }

    socket->disconnectFromServer();
}

void GUISingleApplication::processMessages()
{
    QList<QByteArray> msg = d_ptr->messages;
    d_ptr->messages.clear();
    Q_EMIT messageReceived(msg);
}

#include "moc_GuiApplication.cpp"
