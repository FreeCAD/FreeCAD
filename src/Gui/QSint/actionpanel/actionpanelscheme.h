/***************************************************************************
 *                                                                         *
 *   Copyright: https://code.google.com/p/qsint/                           *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#ifndef ACTIONPANELSCHEME_H
#define ACTIONPANELSCHEME_H

#include "qsint_global.h"

#include <QApplication>
#include <QImage>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QHash>
#include <QSize>
#include <QString>

namespace QSint
{

/**
 * #@brief Class representing color scheme for ActionPanel and ActionGroup.
 */
class QSINT_EXPORT ActionPanelScheme
{
public:
    /**
     * @brief Animation effect during expanding/collapsing of the ActionGroup's contents.
     */
    enum FoldEffect
    {
        NoFolding,
        ShrunkFolding,
        SlideFolding
    };

    ActionPanelScheme();

    /** Returns a pointer to the default scheme object.
     * Must be reimplemented in derived classes for custom schemes.
     */
    static ActionPanelScheme* defaultScheme();

    /// Height of the header in pixels.
    int headerSize;
    /// If set to \a true, moving mouse over the header results in changing its opacity slowly.
    bool headerAnimation;

    /// Image of folding button when the group is expanded.
    QPixmap headerButtonFold;
    /// Image of folding button when the group is expanded and mouse cursor is over the button.
    QPixmap headerButtonFoldOver;
    /// Image of folding button when the group is collapsed.
    QPixmap headerButtonUnfold;
    /// Image of folding button when the group is collapsed and mouse cursor is over the button.
    QPixmap headerButtonUnfoldOver;

    QSize headerButtonSize;

    /// Number of steps made for expanding/collapsing animation (default 20).
    int groupFoldSteps;
    /// Delay in ms between steps made for expanding/collapsing animation (default 15).
    int groupFoldDelay;
    /// Sets folding effect during expanding/collapsing.
    FoldEffect groupFoldEffect;
    /// If set to \a true, changes group's opacity slowly during expanding/collapsing.
    bool groupFoldThaw;

    /// The CSS for the ActionPanel/ActionGroup elements.
    QString actionStyle;

    /**
     * @brief Clears the custom action style, resetting to a minimal style.
     */
    void clearActionStyle();

    /**
     * @brief Restores the original action style.
     */
    void restoreActionStyle();

    static const QString minimumStyle;

    /**
     * @brief Generates a custom system style based on the palette.
     * @param p The palette to use for generating the style.
     * @return A QString containing the generated style.
     */
    QString systemStyle(const QPalette& p);

protected:
    /**
     * @brief Draws a fold/unfold icon based on the palette.
     * @param p The palette to use for coloring the icon.
     * @param fold True for fold icon, false for unfold icon.
     * @param hover True for hover effect, false otherwise.
     * @return A QPixmap representing the icon.
     */
    QPixmap drawFoldIcon(const QPalette& p, bool fold, bool hover) const;

private:
    // Store the built-in icons for restoration.
    QPixmap builtinFold;
    QPixmap builtinFoldOver;
    QPixmap builtinUnfold;
    QPixmap builtinUnfoldOver;
    QString builtinScheme;
};

} // namespace QSint

#endif // ACTIONPANELSCHEME_H
