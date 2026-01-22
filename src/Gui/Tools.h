/***************************************************************************
 *   Copyright (c) 2020 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *   Copyright (c) 2025 Kacper Donat <kacper@kadet.net>                     *
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

#include <FCConfig.h>
#include <FCGlobal.h>

#include <QFontMetrics>
#include <QKeyEvent>
#include <QKeySequence>
#include <QModelIndex>

/**
 * @brief The QtTools namespace
 *
 * Helper namespace to provide utilities to ease work with Qt.
 */
namespace Gui::QtTools
{
inline int horizontalAdvance(const QFontMetrics& fm, QChar ch)
{
    return fm.horizontalAdvance(ch);
}

inline int horizontalAdvance(const QFontMetrics& fm, const QString& text, int len = -1)
{
    return fm.horizontalAdvance(text, len);
}

inline bool matches(QKeyEvent* ke, const QKeySequence& ks)
{
    uint searchkey = (ke->modifiers() | ke->key()) & ~(Qt::KeypadModifier | Qt::GroupSwitchModifier);
    return ks == QKeySequence(searchkey);
}

inline QKeySequence::StandardKey deleteKeySequence()
{
#ifdef FC_OS_MACOSX
    return QKeySequence::Backspace;
#else
    return QKeySequence::Delete;
#endif
}

// clang-format off
/**
 * TreeWalkCallable is a function that takes const QModelIndex& and:
 *  - returns void, if there is no stopping logic;
 *  - returns boolean, if there is logic that should stop tree traversal.
 */
template<typename Func>
concept TreeWalkCallable =
    std::is_invocable_r_v<void, Func, const QModelIndex&> ||
    std::is_invocable_r_v<bool, Func, const QModelIndex&>;
// clang-format on

/**
 * @brief Recursively traverses a QAbstractItemModel tree structure.
 *
 * The function traverses a tree model starting from a given index, or the root
 * if no index is provided. For each node, it invokes the provided callable `func`.
 *
 * The callable can:
 * - Return `void`, in which case the traversal continues through all nodes.
 * - Return `bool`, in which case returning `true` stops further traversal.
 *
 * @param[in] model The tree model to traverse.
 * @param[in] func A callable object applied to each QModelIndex. It can either
 *             return `void` or `bool` (for stopping logic).
 * @param[in] index The starting index for traversal. If omitted, defaults to the root.
 */
void walkTreeModel(
    const QAbstractItemModel* model,
    TreeWalkCallable auto&& func,
    const QModelIndex& index = {}
)
{
    using ReturnType = std::invoke_result_t<decltype(func), const QModelIndex&>;

    if (index.isValid()) {
        if constexpr (std::is_same_v<ReturnType, void>) {
            func(index);
        }
        else if constexpr (std::is_same_v<ReturnType, bool>) {
            if (func(index)) {
                return;
            }
        }
    }

    for (int i = 0; i < model->rowCount(index); ++i) {
        walkTreeModel(model, func, model->index(i, 0, index));
    }
}

template<typename T>
T valueOr(const QVariant& variant, const T& defaultValue)
{
    return variant.canConvert<T>() ? variant.value<T>() : defaultValue;
}

}  // namespace Gui::QtTools
