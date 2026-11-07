// SPDX-FileCopyrightText: 2026 Benjamin Nauck
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "customtitlebarkit/MenuIntegration.h"
#include "customtitlebarkit/CustomTitleBarWindow.h"
#include "customtitlebarkit/FoldableMenuBar.h"

#include <QHBoxLayout>
#include <QMenuBar>
#include <QVBoxLayout>

DefaultMenuIntegration::DefaultMenuIntegration(QObject *parent)
    : MenuIntegration(parent)
{
}

void DefaultMenuIntegration::install(QMenuBar *menuBar, CustomTitleBarWindow *window)
{
    menuBar->setNativeMenuBar(false);

    m_container = new QWidget(window->leftArea());
    // Transparent border forces Qt's styled rendering path, needed for
    // proper vertical centering of QMenuBar (platform-native ignores layout).
    m_container->setStyleSheet("border: 0px solid transparent;");
    auto *vbox = new QVBoxLayout(m_container);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->setSpacing(0);
    menuBar->setParent(m_container);
    menuBar->setVisible(true);
    vbox->addWidget(menuBar, 0, Qt::AlignVCenter);

    auto *layout = window->leftArea()->layout();
    if (!layout) {
        layout = new QHBoxLayout(window->leftArea());
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
    }
    static_cast<QHBoxLayout *>(layout)->insertWidget(0, m_container);
}

void DefaultMenuIntegration::uninstall(CustomTitleBarWindow *window)
{
    if (m_container) {
        // Reparent the menuBar out of the container before destroying it
        if (auto *layout = m_container->layout()) {
            while (auto *item = layout->takeAt(0)) {
                if (item->widget())
                    item->widget()->setParent(window);
                delete item;
            }
        }
        delete m_container;
        m_container = nullptr;
    }
}

// --- FoldableMenuIntegration ---

FoldableMenuIntegration::FoldableMenuIntegration(QWidget *brandWidget, QObject *parent)
    : MenuIntegration(parent)
    , m_brandWidget(brandWidget)
{
}

void FoldableMenuIntegration::install(QMenuBar *menuBar, CustomTitleBarWindow *window)
{
    m_foldableBar = new FoldableMenuBar(window->leftArea());
    m_foldableBar->setMenuBar(menuBar);
    m_foldableBar->setOverlayMaskTargets({window->leftArea(), window->rightArea()});
    m_foldableBar->setFoldable(true);

    if (m_brandWidget)
        m_foldableBar->setBrandWidget(m_brandWidget);

    auto *layout = window->leftArea()->layout();
    if (!layout) {
        layout = new QHBoxLayout(window->leftArea());
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
    }
    static_cast<QHBoxLayout *>(layout)->insertWidget(0, m_foldableBar);
}

void FoldableMenuIntegration::uninstall(CustomTitleBarWindow *window)
{
    if (m_foldableBar) {
        // Reclaim the brand widget before the FoldableMenuBar is destroyed
        if (m_foldableBar->brandWidget()) {
            m_brandWidget = m_foldableBar->brandWidget();
            m_foldableBar->setBrandWidget(nullptr);
            m_brandWidget->setParent(nullptr);
            m_brandWidget->hide();
        }
        // Reparent the QMenuBar out so it survives deletion of the FoldableMenuBar,
        // and restore it to a clean state (foldable mode leaves it hidden + disabled).
        if (auto *mb = window->menuBar()) {
            mb->setParent(window);
            mb->setEnabled(true);
            mb->clearMask();
            // Don't call setVisible() — the next install() method handles
            // visibility (native hides, inline/foldable shows via addWidget).
        }
        delete m_foldableBar;
        m_foldableBar = nullptr;
    }
}
