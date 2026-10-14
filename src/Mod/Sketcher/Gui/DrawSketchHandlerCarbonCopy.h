// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2022 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <array>

#include <QApplication>
#include <QCoreApplication>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>
#include <QShortcut>
#include <QString>

#include <Gui/Notifications.h>
#include <Gui/Selection/SelectionFilter.h>
#include <Gui/Command.h>
#include <Gui/CommandT.h>
#include <Gui/MDIView.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

#include <Mod/Sketcher/App/SketchObject.h>

#include "DrawSketchHandler.h"
#include "Utils.h"
#include "ViewProviderSketch.h"
#include "SnapManager.h"


namespace SketcherGui
{

class CarbonCopySelection:
    public Gui::SelectionFilterGate,
    public Gui::SelectionGateMessageProvider
{
    enum class Shortcut
    {
        None,
        Control,
        ControlAlt
    };

    struct RejectionMessage
    {
        Sketcher::SketchObject::eReasonList reason;
        const char* source;
        Shortcut shortcut;
    };

    static constexpr std::array<RejectionMessage, 8> rejectionMessages {{
        {Sketcher::SketchObject::rlCircularReference,
         QT_TRANSLATE_NOOP(
             "SketcherGui::CarbonCopySelection",
             "Carbon copy would cause a circular dependency."
         ),
         Shortcut::None},
        {Sketcher::SketchObject::rlOtherDoc,
         QT_TRANSLATE_NOOP(
             "SketcherGui::CarbonCopySelection",
             "This object is in another document."
         ),
         Shortcut::None},
        {Sketcher::SketchObject::rlOtherBody,
         QT_TRANSLATE_NOOP(
             "SketcherGui::CarbonCopySelection",
             "This object belongs to another body. Hold %1 to allow cross-references."
         ),
         Shortcut::Control},
        {Sketcher::SketchObject::rlOtherBodyWithLinks,
         QT_TRANSLATE_NOOP(
             "SketcherGui::CarbonCopySelection",
             "This object belongs to another body and it contains external "
             "geometry. Cross-reference not allowed."
         ),
         Shortcut::None},
        {Sketcher::SketchObject::rlOtherPart,
         QT_TRANSLATE_NOOP(
             "SketcherGui::CarbonCopySelection",
             "This object belongs to another part."
         ),
         Shortcut::None},
        {Sketcher::SketchObject::rlNonParallel,
         QT_TRANSLATE_NOOP(
             "SketcherGui::CarbonCopySelection",
             "The selected sketch is not parallel to this sketch. Hold %1 "
             "to allow non-parallel sketches."
         ),
         Shortcut::ControlAlt},
        {Sketcher::SketchObject::rlAxesMisaligned,
         QT_TRANSLATE_NOOP(
             "SketcherGui::CarbonCopySelection",
             "The XY axes of the selected sketch do not have the same "
             "direction as this sketch. Hold %1 to disregard it."
         ),
         Shortcut::ControlAlt},
        {Sketcher::SketchObject::rlOriginsMisaligned,
         QT_TRANSLATE_NOOP(
             "SketcherGui::CarbonCopySelection",
             "The origin of the selected sketch is not aligned with the "
             "origin of this sketch. Hold %1 to disregard it."
         ),
         Shortcut::ControlAlt}
    }};

    App::DocumentObject* object;

    static bool hasExpectedPlaceholders(const QString& text, Shortcut shortcut)
    {
        static const QRegularExpression placeholders(QStringLiteral("%(?:L?[0-9]{1,2}|n)"));
        auto matches = placeholders.globalMatch(text);
        if (shortcut == Shortcut::None) {
            return !matches.hasNext();
        }
        if (!matches.hasNext()) {
            return false;
        }
        while (matches.hasNext()) {
            if (matches.next().captured() != QStringLiteral("%1")) {
                return false;
            }
        }
        return true;
    }

    static QString shortcutText(Shortcut shortcut)
    {
#ifdef Q_OS_MACOS
        const bool dontSwap = QCoreApplication::testAttribute(Qt::AA_MacDontSwapCtrlAndMeta);
        const QString control =
            dontSwap ? QStringLiteral("\u2303") : QStringLiteral("\u2318");
        if (shortcut == Shortcut::Control) {
            return control;
        }
        const QString option = QStringLiteral("\u2325");
        return dontSwap ? control + option : option + control;
#else
        // Use Qt's translations without extracting these key names into Sketcher's catalog.
        const char* ctrlKey = "Ctrl";
        const char* altKey = "Alt";
        const QString control = QShortcut::tr(ctrlKey);
        return shortcut == Shortcut::Control
            ? control
            : control + QLatin1Char('+') + QShortcut::tr(altKey);
#endif
    }

public:
    explicit CarbonCopySelection(App::DocumentObject* obj)
        : Gui::SelectionFilterGate(nullPointer())
        , object(obj)
    {}

    QString selectionRejectionText() const override
    {
        if (notAllowedReason.empty()) {
            return {};
        }
        QString text = QCoreApplication::translate(
            "SketcherGui::CarbonCopySelection", notAllowedReason.c_str()
        );
        for (const auto& message : rejectionMessages) {
            if (notAllowedReason == message.source) {
                if (text.isEmpty() || !hasExpectedPlaceholders(text, message.shortcut)) {
                    text = QString::fromUtf8(message.source);
                }
                return message.shortcut == Shortcut::None
                    ? text
                    : text.arg(shortcutText(message.shortcut));
            }
        }
        // Preserve informative text even if it is not one of the known reasons.
        return text.isEmpty() ? QString::fromUtf8(notAllowedReason.c_str()) : text;
    }

    bool allow(App::Document* pDoc, App::DocumentObject* pObj, const char* sSubName) override
    {
        Q_UNUSED(sSubName);

        Sketcher::SketchObject* sketch = static_cast<Sketcher::SketchObject*>(object);
        sketch->setAllowOtherBody(
            QApplication::keyboardModifiers() == Qt::ControlModifier
            || QApplication::keyboardModifiers() == (Qt::ControlModifier | Qt::AltModifier)
        );
        sketch->setAllowUnaligned(
            QApplication::keyboardModifiers() == (Qt::ControlModifier | Qt::AltModifier)
        );

        this->notAllowedReason = "";
        Sketcher::SketchObject::eReasonList msg;
        // Reusing code: All good reasons not to allow a carbon copy
        bool xinv = false, yinv = false;
        if (!sketch->isCarbonCopyAllowed(pDoc, pObj, xinv, yinv, &msg)) {
            for (const auto& message : rejectionMessages) {
                if (message.reason == msg) {
                    this->notAllowedReason = message.source;
                    break;
                }
            }
            return false;
        }
        // Carbon copy only works on sketches that are not disallowed (e.g. would produce a circular
        // reference)
        return true;
    }
};

class DrawSketchHandlerCarbonCopy: public DrawSketchHandler
{
    Q_DECLARE_TR_FUNCTIONS(SketcherGui::DrawSketchHandlerCarbonCopy)

public:
    DrawSketchHandlerCarbonCopy() = default;
    ~DrawSketchHandlerCarbonCopy() override
    {
        Gui::Selection().rmvSelectionGate();
    }

    void mouseMove(SnapManager::SnapHandle snapHandle) override
    {
        Q_UNUSED(snapHandle);
        if (Gui::Selection().getPreselection().pObjectName) {
            applyCursor();
        }
    }

    bool pressButton(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        return true;
    }

    bool releaseButton(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        /* this is ok not to call to purgeHandler
         * in continuous creation mode because the
         * handler is destroyed by the quit() method on pressing the
         * right button of the mouse */
        return true;
    }

    bool onSelectionChanged(const Gui::SelectionChanges& msg) override
    {
        if (msg.Type == Gui::SelectionChanges::AddSelection) {
            App::DocumentObject* obj = sketchgui->getObject()->getDocument()->getObject(
                msg.pObjectName
            );
            if (!obj) {
                throw Base::ValueError("Sketcher: Carbon Copy: Invalid object in selection");
            }

            std::string sketchArchType("Sketcher::SketchObjectPython");

            if (obj->is<Sketcher::SketchObject>() || sketchArchType == obj->getTypeId().getName()) {

                try {
                    openCommand(QT_TRANSLATE_NOOP("Command", "Create a carbon copy"));
                    Gui::cmdAppObjectArgs(
                        sketchgui->getObject(),
                        "carbonCopy(\"%s\",%s)",
                        msg.pObjectName,
                        constructionModeAsBooleanText()
                    );

                    commitCommand();

                    tryAutoRecomputeIfNotSolve(sketchgui->getObject<Sketcher::SketchObject>());

                    Gui::Selection().clearSelection();
                    /* this is ok not to call to purgeHandler
                     * in continuous creation mode because the
                     * handler is destroyed by the quit() method on pressing the
                     * right button of the mouse */
                }
                catch (const Base::Exception&) {
                    Gui::NotifyError(
                        sketchgui,
                        QT_TRANSLATE_NOOP("Notifications", "Error"),
                        QT_TRANSLATE_NOOP("Notifications", "Failed to add carbon copy")
                    );
                    abortCommand();
                }
                return true;
            }
        }
        updateHint();
        return false;
    }

private:
    void activated() override
    {
        setAxisPickStyle(false);
        Gui::MDIView* mdi = Gui::Application::Instance->activeDocument()->getActiveView();
        Gui::View3DInventorViewer* viewer;
        viewer = static_cast<Gui::View3DInventor*>(mdi)->getViewer();
        viewer->setSelectionEnabled(true);

        Gui::Selection().clearSelection();
        Gui::Selection().rmvSelectionGate();
        Gui::Selection().addSelectionGate(new CarbonCopySelection(sketchgui->getObject()));
    }

    QString getCrosshairCursorSVGName() const override
    {
        return QStringLiteral("Sketcher_Pointer_CarbonCopy");
    }

    void deactivated() override
    {
        Q_UNUSED(sketchgui);
        setAxisPickStyle(true);
    }

public:
    std::list<Gui::InputHint> getToolHints() const override
    {
        return {
            {tr("%1 pick sketch to copy", "Sketcher CarbonCopy: hint"),
             {Gui::InputHint::UserInput::MouseLeft}},
        };
    }
};

}  // namespace SketcherGui
