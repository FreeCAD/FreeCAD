
#include "PreCompiled.h"

// STL
#include <iostream>
#include <string>
#include <stdlib.h>

#include "DebugServer.h"

using namespace Debugger;

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

DebugServer::DebugServer(QObject *parent)
    : QTcpServer(parent)
{
    connect(this, SIGNAL(newConnection()), this, SLOT(onConnected()));
}

void DebugServer::incomingConnection(int socket)
{
    QTcpSocket* sock = new QTcpSocket(this);
    connect(sock, SIGNAL(disconnected()),
            sock, SLOT(deleteLater()));
    connect(sock, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(onDisplayError(QAbstractSocket::SocketError)));
    connect(sock, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            this, SLOT(onStateChanged(QAbstractSocket::SocketState)));
    connect(sock, SIGNAL(readyRead()),
            this, SLOT(onReceiveData()));
    sock->setSocketDescriptor(socket);
}

void DebugServer::onConnected()
{
    // loopback
    if (this->serverAddress() == QHostAddress::Any) {
        this->close();
    }
}

void DebugServer::stepGo()
{
    MsgHeader header;
    header.setMsgType(Step, 1);
    this->msg += header.toByteArray();
    this->msg += '0';
}

void DebugServer::stepOver()
{
    MsgHeader header;
    header.setMsgType(Step, 1);
    this->msg += header.toByteArray();
    this->msg += '1';
}

void DebugServer::stepInto()
{
    MsgHeader header;
    header.setMsgType(Step, 1);
    this->msg += header.toByteArray();
    this->msg += '2';
}

void DebugServer::stepOut()
{
    MsgHeader header;
    header.setMsgType(Step, 1);
    this->msg += header.toByteArray();
    this->msg += '3';
}

void DebugServer::toggleBreakpoint(const QString& file, int line)
{
    QString str = QString::fromAscii("%1%%2").arg(line).arg(file);
    QByteArray ary = str.toUtf8();
    MsgHeader header;
    header.setMsgType(SetBreak, ary.length());
    this->msg += header.toByteArray();
    this->msg += ary;
}

void DebugServer::onDisplayError(QAbstractSocket::SocketError socketError)
{
    QTcpSocket* tcpClient = qobject_cast<QTcpSocket*>(sender());
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        std::cout << "TCP: The remote host closed the connection.\n";
        break;
    case QAbstractSocket::HostNotFoundError:
        std::cout << "TCP: The host was not found. Please check the host name and port settings.\n";
        break;
    case QAbstractSocket::ConnectionRefusedError:
        std::cout << "TCP: The connection was refused by the peer. Make sure the server is running.\n";
        break;
    default:
        std::cout << "The following error occurred: " << (const char*)tcpClient->errorString().toAscii() << "\n";
        break;
    }
}

void DebugServer::onStateChanged(QAbstractSocket::SocketState state)
{
    QTcpSocket* tcpClient = qobject_cast<QTcpSocket*>(sender());
    QString text;
    if (state == QAbstractSocket::HostLookupState) {
        text = QString::fromAscii("TCP: Lookup host name %1...").arg(tcpClient->peerName());
    }
    else if (state == QAbstractSocket::ClosingState) {
        text = QString::fromAscii("TCP: Closing %1...").arg(tcpClient->peerName());
    }
    else if (state == QAbstractSocket::UnconnectedState) {
        text = QString::fromAscii("TCP: Disconnected from host %1...").arg(tcpClient->peerName());
    }

    if (!text.isEmpty())
        std::cout << (const char*)text.toAscii() << "\n";
}

void DebugServer::onReceiveData()
{
    QTcpSocket* tcpClient = qobject_cast<QTcpSocket*>(sender());
    if (!this->msg.isEmpty()) {
        tcpClient->write(this->msg);
        this->msg.clear();
    }

    MsgHeader header;
    while (tcpClient->bytesAvailable() >= sizeof(MsgHeader)) {
        tcpClient->read((char*)&header, sizeof(MsgHeader));
        MsgType type = header.getMsgType();
        size_t  size = header.getMsgSize();

        QByteArray bytes;
        if (size > 0) {
            bytes = tcpClient->read(size);
            if (bytes.length() < size) {
                // error
                qWarning("Failed to read full message\n");
            }
        }

        if (type == Init) {
            header.setMsgType(Init, 0);
            tcpClient->write(header.toByteArray());
        }
        else if (type == Ping) {
            header.setMsgType(Pong, 0);
            tcpClient->write(header.toByteArray());
        }
        else if (type == Step) {
            header.setMsgType(Pong, 0);
            tcpClient->write(header.toByteArray());
        }
        else if (type == Break) {
            emitMessage(bytes);
        }
        else {
            header.setMsgType(Pong, 0);
            tcpClient->write(header.toByteArray());
        }
    }
}

#include "moc_DebugServer.cpp"
