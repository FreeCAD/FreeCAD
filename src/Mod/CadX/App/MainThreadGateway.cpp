// SPDX-License-Identifier: LGPL-2.1-or-later

#include "MainThreadGateway.h"

#include <QCoreApplication>
#include <QMetaObject>
#include <QThread>

#include <stdexcept>

namespace CadX
{

MainThreadGateway::MainThreadGateway()
    : _qtThread(QCoreApplication::instance()
                    ? QCoreApplication::instance()->thread()
                    : nullptr)
{}

bool MainThreadGateway::isMainThread() const noexcept
{
    return _qtThread && QThread::currentThread() == _qtThread;
}

void MainThreadGateway::run(Task task) const
{
    if (!task) {
        return;
    }
    if (isMainThread()) {
        task();
        return;
    }
    auto* application = QCoreApplication::instance();
    if (!application || !_qtThread || application->thread() != _qtThread) {
        throw std::logic_error("CadX mutation gateway has no initialized FreeCAD main thread");
    }
    std::exception_ptr failure;
    const bool invoked = QMetaObject::invokeMethod(
        application,
        [task = std::move(task), &failure]() mutable {
            try {
                task();
            }
            catch (...) {
                failure = std::current_exception();
            }
        },
        Qt::BlockingQueuedConnection);
    if (!invoked) {
        throw std::logic_error("CadX mutation could not reach the FreeCAD main thread");
    }
    if (failure) {
        std::rethrow_exception(failure);
    }
}

}  // namespace CadX
