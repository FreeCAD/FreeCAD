#ifndef DEBUGSERVER_H
#define DEBUGSERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>

namespace Debugger {

class DebugServer : public QTcpServer
{
    Q_OBJECT

public:
    DebugServer(QObject *parent = 0);
    void incomingConnection(int socket);
    void stepGo();
    void stepOver();
    void stepInto();
    void stepOut();
    void toggleBreakpoint(const QString&, int);

private Q_SLOTS:
    void onConnected();
    void onDisplayError(QAbstractSocket::SocketError socketError);
    void onStateChanged(QAbstractSocket::SocketState);
    void onReceiveData();

Q_SIGNALS:
    void emitMessage(const QByteArray&);

private:
    QByteArray msg;
};

} // namespace Debugger

#endif // DEBUGSERVER_H
