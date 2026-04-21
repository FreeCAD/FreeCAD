/***************************************************************************
 *   Copyright (c) 2024 Kacper Donat <kacper@kadet.net>                    *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <QLayout>
#include <QToolBar>
#include <QPointer>
#include <QWidget>
#include <fastsignals/signal.h>
#include <Base/Parameter.h>

namespace Gui
{

// Qt treats area as Flag so in theory toolbar could be in multiple areas at once.
// We don't do that here so simple enum should suffice.
enum class ToolBarArea
{
    NoToolBarArea,
    LeftToolBarArea,
    RightToolBarArea,
    TopToolBarArea,
    BottomToolBarArea,
    LeftMenuToolBarArea,
    RightMenuToolBarArea,
    StatusBarToolBarArea,
};

class ToolBarAreaWidget: public QWidget
{
    Q_OBJECT
    using inherited = QWidget;

public:
    ToolBarAreaWidget(
        QWidget* parent,
        ToolBarArea area,
        const ParameterGrp::handle& hParam,
        fastsignals::advanced_scoped_connection& conn,
        QTimer* timer = nullptr
    );

    void addWidget(QWidget* widget);
    void insertWidget(int index, QWidget* widget);
    void removeWidget(QWidget* widget);

    void adjustParent();

    QWidget* widgetAt(int index) const
    {
        auto item = _layout->itemAt(index);

        return item ? item->widget() : nullptr;
    }

    int count() const
    {
        return _layout->count();
    }

    int indexOf(QWidget* widget) const
    {
        return _layout->indexOf(widget);
    }

    ToolBarArea area() const
    {
        return _area;
    }

    template<class FuncT>
    void foreachToolBar(FuncT&& func)
    {
        for (int i = 0, count = _layout->count(); i < count; ++i) {
            auto toolbar = qobject_cast<QToolBar*>(widgetAt(i));

            if (!toolbar || toolbar->objectName().isEmpty()
                || toolbar->objectName().startsWith(QStringLiteral("*"))) {
                continue;
            }

            func(toolbar, i, this);
        }
    }

    void saveState();
    void restoreState(const std::map<int, QToolBar*>& toolbars);

private:
    QHBoxLayout* _layout;
    QPointer<QTimer> _sizingTimer;
    ParameterGrp::handle _hParam;
    fastsignals::advanced_scoped_connection& _conn;
    ToolBarArea _area;
};

}  // namespace Gui
