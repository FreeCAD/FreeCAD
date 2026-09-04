// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2024 The FreeCAD Project Association AISBL               *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#pragma once

#include <QListView>

namespace StartGui
{

class FileCardView: public QListView
{
    Q_OBJECT

public:
    explicit FileCardView(QWidget* parent = nullptr);

    int heightForWidth(int width) const override;

    QSize sizeHint() const override;

    /// Controls whether right-clicking a card offers a "Remove" context menu entry. Off by
    /// default; intended for card views (such as Recent Files) where individual entries can
    /// meaningfully be removed by the user, as opposed to e.g. the bundled Examples list.
    void setAllowRemoval(bool allow);
    bool allowsRemoval() const;

Q_SIGNALS:
    /// Emitted when the user selects "Remove" from a card's context menu. filePath is the full
    /// path of the file backing that card, taken from the model's "path" role.
    void fileRemovalRequested(const QString& filePath);

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    int m_cardSpacing;
    bool m_allowRemoval {false};
};

}  // namespace StartGui
