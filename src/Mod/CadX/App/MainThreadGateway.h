// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <functional>
class QThread;

namespace CadX
{

class MainThreadGateway
{
public:
    using Task = std::function<void()>;

    MainThreadGateway();
    bool isMainThread() const noexcept;
    void run(Task task) const;

private:
    QThread* _qtThread = nullptr;
};

}  // namespace CadX
