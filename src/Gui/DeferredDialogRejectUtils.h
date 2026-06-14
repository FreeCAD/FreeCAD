// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

#pragma once

#include <string>
#include <type_traits>

#include <QDialogButtonBox>
#include <QMetaObject>
#include <QPointer>

#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Control.h>

namespace Gui
{

struct DeferredDialogRejectState
{
    bool pending = false;
    std::string documentName;
    QMetaObject::Connection connection;
};

template<typename Sender, typename Signal, typename Receiver, typename Slot>
inline void ensureDeferredDialogRejectConnection(
    DeferredDialogRejectState& state,
    Sender* sender,
    Signal signal,
    Receiver* receiver,
    Slot slot
)
{
    if (state.connection || !sender) {
        return;
    }

    state.connection = QObject::connect(sender, signal, receiver, slot, Qt::QueuedConnection);
}

template<typename UpdateFn>
inline void setDeferredDialogRejectPending(
    DeferredDialogRejectState& state,
    bool pending,
    QDialogButtonBox* buttonBox,
    UpdateFn&& update
)
{
    state.pending = pending;
    if (buttonBox) {
        buttonBox->setEnabled(!pending);
    }

    update(pending);
}

template<typename Dialog, typename RejectFn, typename SetPendingFn, typename CloseFn = std::nullptr_t>
inline void finishDeferredDialogReject(
    Dialog* dialog,
    DeferredDialogRejectState& state,
    bool readyToClose,
    RejectFn&& rejectNow,
    SetPendingFn&& setPending,
    CloseFn&& closeDialog = nullptr
)
{
    if (!state.pending || !readyToClose) {
        return;
    }

    QPointer<Dialog> guard(dialog);
    const bool success = rejectNow();
    if (!guard) {
        return;
    }

    if (!success) {
        setPending(false);
        return;
    }

    App::Document* document = state.documentName.empty()
        ? nullptr
        : App::GetApplication().getDocument(state.documentName.c_str());
    if constexpr (std::is_same_v<std::decay_t<CloseFn>, std::nullptr_t>) {
        if (document && Gui::Control().activeDialog(document) == dialog) {
            Gui::Control().closeDialog(document);
        }
    }
    else {
        closeDialog(document, dialog);
    }
}

}  // namespace Gui
