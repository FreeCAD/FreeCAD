// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

#include <cstddef>
#include <cstdio>
#include <vector>

#include <QApplication>
#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QLibrary>
#include <QString>

#include <src/App/InitApplication.h>

namespace
{

using RunQtModuleTestFn = int (*)(int argc, char** argv);

QString modulePath(const char* moduleArgument)
{
    const QString path = QString::fromLocal8Bit(moduleArgument);
    if (QFileInfo(path).isAbsolute()) {
        return path;
    }

    return QCoreApplication::applicationDirPath() + QDir::separator() + path;
}

}  // namespace

int main(int argc, char** argv)
{
    if (argc < 3) {
        std::fprintf(stderr, "Usage: %s <module> <entry-point> [QTest arguments...]\n", argv[0]);
        return 1;
    }

    const QString moduleFile = modulePath(argv[1]);
    const QByteArray entryPoint = QByteArray(argv[2]);

    tests::initApplication();

    QApplication app(argc, argv);

    QLibrary module(moduleFile);
    module.setLoadHints(QLibrary::ResolveAllSymbolsHint | QLibrary::ExportExternalSymbolsHint);
    if (!module.load()) {
        std::fprintf(
            stderr,
            "Failed to load %s: %s\n",
            qPrintable(module.fileName()),
            qPrintable(module.errorString())
        );
        return 1;
    }

    auto run = reinterpret_cast<RunQtModuleTestFn>(module.resolve(entryPoint.constData()));
    if (!run) {
        std::fprintf(
            stderr,
            "Failed to resolve test entry point %s: %s\n",
            entryPoint.constData(),
            qPrintable(module.errorString())
        );
        return 1;
    }

    std::vector<char*> testArgv;
    testArgv.reserve(static_cast<std::size_t>(argc) - 1);
    testArgv.push_back(argv[0]);
    for (int i = 3; i < argc; ++i) {
        testArgv.push_back(argv[i]);
    }

    return run(static_cast<int>(testArgv.size()), testArgv.data());
}
