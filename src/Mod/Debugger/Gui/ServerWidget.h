#ifndef SERVERWIDGET_H
#define SERVERWIDGET_H

#include <QWidget>
#include "DebugServer.h"
#include "ui_ServerWidget.h"

namespace DebuggerGui {
class ServerWidget : public QWidget
{
    Q_OBJECT

public:
    ServerWidget(const QHostAddress& host, quint16 port);

protected:
    void closeEvent(QCloseEvent*);
    void showEvent(QShowEvent*);
    void hideEvent(QHideEvent*);

protected Q_SLOTS:
    void on_buttonStepGo_clicked();
    void on_buttonStepOver_clicked();
    void on_buttonStepInto_clicked();
    void on_buttonStepOut_clicked();
    void on_buttonBreakpoint_clicked();
    void onMessage(const QByteArray&);

private:
    Debugger::DebugServer server;
    QHostAddress host;
    quint16 port;
    Ui_ServerWidget ui;
};

} // DebuggerGui

#endif //SERVERWIDGET_H
