// SPDX-FileCopyrightText: 2026 Benjamin Nauck
// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QPushButton>
#include <QTextEdit>

#include "customtitlebarkit/CustomTitleBarWindow.h"
#include "customtitlebarkit/MenuIntegration.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Load default stylesheet
    QFile qss(QDir(QCoreApplication::applicationDirPath()).filePath("default.qss"));
    if (qss.open(QIODevice::ReadOnly | QIODevice::Text))
        app.setStyleSheet(qss.readAll());

    CustomTitleBarWindow window;
    window.setWindowTitle("Custom Title Bar Example");
    window.resize(800, 600);

    // Drop-in menuBar() — default integration is platform-appropriate
    auto *fileMenu = window.menuBar()->addMenu("&File");
    fileMenu->addAction("New");
    fileMenu->addAction("Open...");
    fileMenu->addAction("Save");
    fileMenu->addSeparator();
    fileMenu->addAction("Exit", &app, &QApplication::quit);

    auto *editMenu = window.menuBar()->addMenu("&Edit");
    editMenu->addAction("Undo");
    editMenu->addAction("Redo");
    editMenu->addSeparator();
    editMenu->addAction("Cut");
    editMenu->addAction("Copy");
    editMenu->addAction("Paste");

    auto *viewMenu = window.menuBar()->addMenu("&View");
    viewMenu->addAction("Zoom In");
    viewMenu->addAction("Zoom Out");
    viewMenu->addSeparator();
    viewMenu->addAction("Full Screen");

    auto *helpMenu = window.menuBar()->addMenu("&Help");
    helpMenu->addAction("About");

    // Titlebar areas are only available in Custom mode
    if (window.mode() == CustomTitleBarWindow::Mode::Custom) {
        // Left area
        auto *leftLayout = qobject_cast<QHBoxLayout *>(window.leftArea()->layout());
        if (!leftLayout) {
            leftLayout = new QHBoxLayout(window.leftArea());
            leftLayout->setContentsMargins(0, 0, 0, 0);
            leftLayout->setSpacing(0);
        }

        auto *searchButton = new QPushButton("Button");
        leftLayout->addWidget(searchButton);

        // Right area
        auto *rightLayout = new QHBoxLayout(window.rightArea());
        rightLayout->setContentsMargins(0, 0, 8, 0);
        rightLayout->setSpacing(8);

        // Toggle button to switch menu integration at runtime
        auto *toggleMenuBtn = new QPushButton("Foldable Menu");
        toggleMenuBtn->setCheckable(true);
        QObject::connect(toggleMenuBtn, &QPushButton::toggled, &window, [&window](bool checked) {
            if (checked) {
                auto *logo = new QLabel(" MyApp");
                logo->setStyleSheet("font-size: 14px; font-weight: bold;");
                window.setMenuIntegration(new FoldableMenuIntegration(logo, &window));
            } else {
                window.setMenuIntegration(nullptr); // reset to platform default
            }
        });
        rightLayout->addWidget(toggleMenuBtn);

        auto *settingsButton = new QPushButton("Settings");
        rightLayout->addWidget(settingsButton);
    }

    // Central widget
    auto *editor = new QTextEdit;
    editor->setPlaceholderText("Main content area...\n\nClick 'Foldable Menu' to toggle menu integration.");
    window.setCentralWidget(editor);

    window.show();

    return app.exec();
}
