/***************************************************************************
 *   Copyright (c) 2015 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef FREECADTASKPANELSCHEME_H
#define FREECADTASKPANELSCHEME_H

#include "actionpanelscheme.h"

#include <QString>
#include <QPixmap>
#include <QPalette>

namespace QSint
{

/**
 * @class FreeCADPanelScheme
 * @brief A specialized panel scheme for FreeCAD task panels.
 */
class QSINT_EXPORT FreeCADPanelScheme : public ActionPanelScheme
{
public:
    explicit FreeCADPanelScheme();

    /**
     * @brief Provides the default scheme for FreeCAD panels.
     * @return A pointer to the default FreeCADPanelScheme instance.
     */
    static ActionPanelScheme* defaultScheme()
    {
        static FreeCADPanelScheme scheme;
        return &scheme;
    }

    /**
     * @brief Clears the custom action style, resetting to a minimal style.
     */
    void clearActionStyle();

    /**
     * @brief Restores the original action style.
     */
    void restoreActionStyle();

private:
    QString builtinScheme;         ///< Backup of the original scheme style.
    QPixmap builtinFold;           ///< Backup of the default fold icon.
    QPixmap builtinFoldOver;       ///< Backup of the default hover fold icon.
    QPixmap builtinUnfold;         ///< Backup of the default unfold icon.
    QPixmap builtinUnfoldOver;     ///< Backup of the default hover unfold icon.
};

/**
 * @class SystemPanelScheme
 * @brief A system-wide default panel scheme for action panels.
 */
class QSINT_EXPORT SystemPanelScheme : public ActionPanelScheme
{
public:
    explicit SystemPanelScheme();

    /**
     * @brief Provides the default system panel scheme.
     * @return A pointer to the default SystemPanelScheme instance.
     */
    static ActionPanelScheme* defaultScheme()
    {
        static SystemPanelScheme scheme;
        return &scheme;
    }

    /**
     * @brief The minimal style definition shared across schemes.
     */
    static const QString minimumStyle;

private:
    /**
     * @brief Draws a fold/unfold icon based on the palette.
     * @param p The palette to use for coloring the icon.
     * @param fold True for fold icon, false for unfold icon.
     * @param hover True for hover effect, false otherwise.
     * @return A QPixmap representing the icon.
     */
    QPixmap drawFoldIcon(const QPalette& p, bool fold, bool hover) const;

    /**
     * @brief Generates a custom system style based on the palette.
     * @param p The palette to use for generating the style.
     * @return A QString containing the generated style.
     */
    QString systemStyle(const QPalette& p) const;
};

} // namespace QSint

#endif // FREECADTASKPANELSCHEME_H

