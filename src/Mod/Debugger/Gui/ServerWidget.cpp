
#include "ServerWidget.h"

using namespace DebuggerGui;

ServerWidget::ServerWidget(const QHostAddress& host, quint16 port)
  : host(host), port(port)
{
    ui.setupUi(this);
    connect(&server, SIGNAL(emitMessage(const QByteArray&)), this, SLOT(onMessage(const QByteArray&)));
}

void ServerWidget::onMessage(const QByteArray& str)
{
    ui.plainTextEdit->appendPlainText(QString::fromUtf8(str.constData()));
}

void ServerWidget::closeEvent(QCloseEvent*)
{
    server.close();
}

void ServerWidget::showEvent(QShowEvent*)
{
    if (!server.isListening() && !server.listen(host, port))
        throw "";
}

void ServerWidget::hideEvent(QHideEvent*)
{
}

void ServerWidget::on_buttonStepGo_clicked()
{
    server.stepGo();
}

void ServerWidget::on_buttonStepOver_clicked()
{
    server.stepOver();
}

void ServerWidget::on_buttonStepInto_clicked()
{
    server.stepInto();
}

void ServerWidget::on_buttonStepOut_clicked()
{
    server.stepOut();
}

void ServerWidget::on_buttonBreakpoint_clicked()
{
    QString file = "C:/Projects/FreeCAD-git/macros/lego.py";
    server.toggleBreakpoint(file,1);
    server.toggleBreakpoint(file,6);
    server.toggleBreakpoint(file,12);
    server.toggleBreakpoint(file,15);
    server.toggleBreakpoint(file,19);
    server.toggleBreakpoint(file,65);
}

#include "moc_ServerWidget.cpp"
