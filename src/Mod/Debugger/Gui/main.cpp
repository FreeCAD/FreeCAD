
#include <QApplication>

#include <stdlib.h>

#include "DebugServer.h"
#include "ServerWidget.h"

int main(int argc, char *argv[])
{
    quint16 port = 4567;
    QHostAddress host(QHostAddress::Any);
    for (int i=1; i<argc;i++) {
        QString arg = QString::fromAscii(argv[i]);
        if (arg.startsWith(QLatin1String("-host=")))
            host.setAddress(arg.mid(6));
        else if (arg.startsWith(QLatin1String("-port=")))
            port = arg.mid(6).toUShort();
    }

    try {
        QApplication app(argc, argv);
        DebuggerGui::ServerWidget w(host, port);
        w.show();
        return app.exec();
    }
    catch (...) {
        return -1;
    }
}
