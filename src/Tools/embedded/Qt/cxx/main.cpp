
#include <QApplication>
#include <QDialog>
#include <QFileDialog>
#include <QLibrary>
#include <QPushButton>

QLibrary* freecadPlugin = nullptr;

void loadFreeCAD()
{
    if (!freecadPlugin) {
        freecadPlugin = new QLibrary("FreeCADPlugin", qApp);
    }

    if (!freecadPlugin->isLoaded()) {
        if (freecadPlugin->load()) {
            QFunctionPointer ptr = freecadPlugin->resolve("FreeCAD_init");
            if (ptr) {
                ptr();
            }
        }
    }

    // Load a test file
    if (freecadPlugin->isLoaded()) {
        typedef void (*TestFunction)(const char*);
        TestFunction test = (TestFunction)freecadPlugin->resolve("FreeCAD_test");
        if (test) {
            QString file = QFileDialog::getOpenFileName();
            if (!file.isEmpty()) {
                test(file.toUtf8());
            }
        }
    }
}

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QDialog dlg;
    QPushButton* button = new QPushButton(&dlg);
    button->setGeometry(QRect(140, 110, 90, 23));
    button->setText("Load FreeCAD");
    QObject::connect(button, &QPushButton::clicked, &loadFreeCAD);
    dlg.show();
    return app.exec();
}
