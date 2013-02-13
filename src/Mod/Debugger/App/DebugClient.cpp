
#include "PreCompiled.h"

#ifndef _PreComp_
# include <stdarg.h>
# include <QCoreApplication>
# include <QEventLoop>
# include <QTcpSocket>
# include <QTimer>
#endif

#include "DebugClient.h"
#include "PythonDebugger.h"
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Swap.h>

using namespace Debugger;

/*
from Debugger import Debugger as dbg
dbg.start_embedded_debugger("C:/Projects/FreeCAD-git/macros/lego.py")

*/

namespace Debugger {

enum MsgType {
    Invalid = -1,
    Init,
    Eval,
    NoErr,
    Ping,
    Pong,
    Break,
    SetBreak,
    Step,
    Stop,
    Exit,
    Tail, // must be last element
};

const char* MsgIds[] = {
    "INIT",
    "EVAL",
    "EVER",
    "PING",
    "PONG",
    "BRK ",
    "BLST",
    "STEP",
    "STOP",
    "EXIT",
};

class MsgHeader {
    static const size_t LEN = 4;
    char type[LEN];
    char size[LEN];

public:
    MsgHeader()
    {
        memset(this, 0, sizeof(MsgHeader));
    }

    void setMsgType(MsgType id, size_t len)
    {
        memcpy(type, MsgIds[id], LEN);
        for (size_t i=0; i<LEN; i++)
            size[i] = '\0';
        QByteArray ba = QByteArray::number(len);
        for (int i=0; i<ba.length(); i++)
            size[i] = ba.at(i);
    }

    MsgType getMsgType() const
    {
        QByteArray self(type, LEN);
        for (size_t i=0; i<size_t(Tail); i++) {
            if (self == MsgIds[i])
                return MsgType(i);
        }
        return Invalid;
    }

    size_t getMsgSize() const
    {
        QByteArray ba(size);
        return ba.toUInt();
    }

    QByteArray toByteArray() const
    {
        QByteArray ba((const char*)(this), sizeof(MsgHeader));
        return ba;
    }
};
}

DebugClient::DebugClient(QObject * parent)
  : QObject(parent)
{
    // TCP/IP stuff
    tcpSocket = new QTcpSocket(this);
    connect(tcpSocket, SIGNAL(readyRead()),
            this, SLOT(readData()));
    connect(tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(displayError(QAbstractSocket::SocketError)));
    connect(tcpSocket, SIGNAL(connected()),
            this, SLOT(connected()));
    connect(tcpSocket, SIGNAL(disconnected()),
            this, SLOT(disconnected()));
    connect(tcpSocket, SIGNAL(hostFound()),
            this, SLOT(hostFound()));
    connect(tcpSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            this, SLOT(stateChanged(QAbstractSocket::SocketState)));
    debug = new PythonDebugger(this);
}

DebugClient::~DebugClient()
{
}

void DebugClient::connectToHost(const QHostAddress& host, quint16 port)
{
    blockSize = 0;
    tcpSocket->abort();
    tcpSocket->connectToHost(host, port);
}

void DebugClient::sendContext(const QString& file, int line)
{
    QString str = QString::fromAscii("%1%%2").arg(line).arg(file);
    QByteArray msg = str.toUtf8();
    MsgHeader header;
    header.setMsgType(Break, msg.length());
    tcpSocket->write(header.toByteArray());
    tcpSocket->write(msg);
}

void DebugClient::waitForReadyRead()
{
    QEventLoop loop;
    connect(this, SIGNAL(signalNextStep()), &loop, SLOT(quit()));
    MsgHeader header;
    header.setMsgType(Ping, 0);
    tcpSocket->write(header.toByteArray());
    QCoreApplication::instance()->installEventFilter(this);
    loop.exec();
    QCoreApplication::instance()->removeEventFilter(this);
    return;
}

void DebugClient::stepCode(char step)
{
    switch (step) {
    case '0':
        // go
        /*emit*/signalNextStep();
        debug->stepRun();
        break;
    case '1':
        // over
        /*emit*/signalNextStep();
        debug->stepOver();
        break;
    case '2':
        // into
        /*emit*/signalNextStep();
        debug->stepInto();
        break;
    case '3':
        // out
        /*emit*/signalNextStep();
        break;
    default:
        break;
    }
}

bool DebugClient::eventFilter(QObject*, QEvent* e)
{
    if (e->type() == QEvent::KeyPress)
        return true;
    if (e->type() == QEvent::KeyRelease)
        return true;
    if (e->type() == QEvent::MouseButtonPress)
        return true;
    if (e->type() == QEvent::MouseButtonRelease)
        return true;
    return false;
}

void DebugClient::startScript()
{
    debug->start();
    debug->runFile(this->scriptFile);
    debug->stop();
    tcpSocket->disconnectFromHost();
}

void DebugClient::readData()
{
    MsgHeader header;
    while (tcpSocket->bytesAvailable() >= sizeof(MsgHeader)) {
        tcpSocket->read((char*)&header, sizeof(MsgHeader));
        MsgType type = header.getMsgType();
        size_t  size = header.getMsgSize();

        QByteArray bytes;
        if (size > 0) {
            bytes = tcpSocket->read(size);
            if (bytes.length() < size) {
                // error
                qWarning("Failed to read full message\n");
            }
        }

        if (type == Init) {
            QTimer::singleShot(500, this, SLOT(startScript()));
        }
        else if (type == Pong) {
            header.setMsgType(Ping, 0);
            tcpSocket->write(header.toByteArray());
        }
        else if (type == Step) {
            stepCode(bytes[0]);
        }
        else if (type == SetBreak) {
            QList<QByteArray> s = bytes.split('%');
            if (s.size() == 2) {
                debug->toggleBreakpoint(s[0].toInt(), QString::fromUtf8(s[1].constData()));
            }
        }
        //else if (response == "BREAKNOW") {
        //}
        //else if (response == "EVALUATE") {
        //}
        //else if (response == "EVALNOERR") {
        //}
        //else if (response == "EXECUTE") {
        //}
        //else if (response == "RELOAD") {
        //}
        else if (type == Stop) {
            debug->stop();
            tcpSocket->disconnectFromHost();
        }
        else if (type == Exit) {
        }
        else {
            header.setMsgType(Ping, 0);
            tcpSocket->write(header.toByteArray());
        }
    }
}

void DebugClient::displayError(QAbstractSocket::SocketError socketError)
{
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        Base::Console().Warning("The remote host closed the connection.\n");
        break;
    case QAbstractSocket::HostNotFoundError:
        Base::Console().Error("The host was not found. Please check the host name and port settings.\n");
        break;
    case QAbstractSocket::ConnectionRefusedError:
        Base::Console().Error("The connection was refused by the peer. Make sure the server is running.\n");
        break;
    default:
        Base::Console().Error("The following error occurred: %s.\n", (const char*)tcpSocket->errorString().toUtf8());
        break;
    }
}

void DebugClient::connected()
{
    Base::Console().Message("Connected to host %s\n", (const char*)tcpSocket->peerName().toAscii());

    MsgHeader hdr;
    hdr.setMsgType(Init, 0);
    tcpSocket->write(hdr.toByteArray());
}

void DebugClient::disconnected()
{
    Base::Console().Message("Disconnected from host\n");
    signalNextStep();
    debug->stop(); // stop debug tracing
}

void DebugClient::hostFound()
{
}

void DebugClient::stateChanged(QAbstractSocket::SocketState state)
{
    if (state == QAbstractSocket::HostLookupState)
        Base::Console().Message("Lookup host name %s...\n",(const char*)tcpSocket->peerName().toAscii());
}

#include "moc_DebugClient.cpp"
