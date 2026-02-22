// SPDX-License-Identifier: LGPL-3.0-only
/***************************************************************************
 *                                                                         *
 *   Copyright: https://code.google.com/p/qsint/                           *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include "qsint_global.h"

#include <QApplication>
#include <QImage>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QSize>
#include <QString>
#include <QStyle>
#include <QFontMetrics>

namespace QSint
{
/**
 * @brief Provides a color scheme and layout parameters for ActionPanel and ActionGroup widgets.
 *
 * ActionPanels group related actions, and ActionGroups organize actions within a panel.
 * This class defines the visual appearance and behavior (e.g., folding animation) of these components.
 */
class QSINT_EXPORT ActionPanelScheme
{
public:
    /**
     * @brief Animation effect used when expanding or collapsing an ActionGroup's contents.
     */
    enum FoldEffect
    {
        NoFolding,      ///< No folding animation.
        ShrunkFolding,   ///< Contents shrink to a point during folding.
        SlideFolding    ///< Contents slide in and out during folding.
    };

    /**
     * @brief Constructs a default ActionPanelScheme.
     */
    ActionPanelScheme();

    /**
     * @brief Returns a pointer to the default ActionPanelScheme object.
     * Derived classes can override this to provide a custom default scheme.
     * @return A pointer to the default ActionPanelScheme.
     */
    static ActionPanelScheme* defaultScheme();

    /**
     * @brief Height of the header area in pixels.
     */
    int headerSize;

    /**
     * @brief Image of the folding button when the group is expanded.
     */
    QPixmap headerButtonFold;
    /**
     * @brief Image of the folding button when the group is expanded and the mouse is over it.
     */
    QPixmap headerButtonFoldOver;
    /**
     * @brief Image of the folding button when the group is collapsed.
     */
    QPixmap headerButtonUnfold;
    /**
     * @brief Image of the folding button when the group is collapsed and the mouse is over it.
     */
    QPixmap headerButtonUnfoldOver;

    /**
     * @brief Size of the header button.
     */
    QSize headerButtonSize;

    /**
     * @brief Number of steps in the expanding/collapsing animation (default: 20).
     */
    int groupFoldSteps;

    /**
     * @brief Delay in milliseconds between animation steps (default: 15).
     */
    int groupFoldDelay;

    /**
     * @brief Folding effect used during expanding/collapsing.
     */
    FoldEffect groupFoldEffect;

    /**
     * @brief Whether the group's opacity changes slowly during folding.
     */
    bool groupFoldThaw;

    /**
     * @brief CSS style for ActionPanel/ActionGroup elements.
     */
    QString actionStyle;

    /**
     * @brief Clears the custom action style, resetting to a minimal style.
     */
    void clearActionStyle();

    /**
     * @brief Restores the original action style.
     */
    void restoreActionStyle();

protected:
    /**
     * @brief Draws a fold/unfold icon.
     * @param p The palette to use for coloring the icon.
     * @param fold `true` for fold icon, `false` for unfold icon.
     * @param hover `true` for hover effect, `false` otherwise.
     * @return The icon as a QPixmap.
     */
    QPixmap drawFoldIcon(const QPalette& p, bool fold, bool hover) const;

private:
    /**
     * @brief Stores the built-in icons for restoration.
     */
    QPixmap builtinFold;
    QPixmap builtinFoldOver;
    QPixmap builtinUnfold;
    QPixmap builtinUnfoldOver;
};

} // namespace QSint
