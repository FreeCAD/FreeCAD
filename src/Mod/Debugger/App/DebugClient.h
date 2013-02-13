
#ifndef DEBUGCLIENT_H
#define DEBUGCLIENT_H

#include <QAbstractSocket>

class QTcpSocket;
class QHostAddress;

namespace Debugger
{
class PythonDebugger;

class DebugClient : public QObject
{
    Q_OBJECT

public:
    DebugClient(QObject * parent = 0);
    ~DebugClient();

    void connectToHost(const QHostAddress& host, quint16 port);
    void setScriptFile(const QString& file)
    { scriptFile = file; }

    void sendContext(const QString&, int);
    void waitForReadyRead();

protected Q_SLOTS:
    void readData();
    void displayError(QAbstractSocket::SocketError socketError);
    void connected();
    void disconnected();
    void hostFound();
    void stateChanged(QAbstractSocket::SocketState);
    void startScript();

protected:
    void stepCode(char);
    bool eventFilter(QObject*, QEvent*);

Q_SIGNALS:
    void signalNextStep();

protected:
    quint16 blockSize;
    QTcpSocket *tcpSocket;
    PythonDebugger* debug;
    QString scriptFile;
};

} // namespace Debugger

#endif // DEBUGCLIENT_H
