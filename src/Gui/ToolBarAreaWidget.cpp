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


#include <QHBoxLayout>
#include <QTimer>


#include "MainWindow.h"
#include "ToolBarAreaWidget.h"
#include "ToolBarManager.h"
#include <Base/Tools.h>

using namespace Gui;

namespace
{
QString widgetPersistenceKey(QWidget* widget)
{
    if (!widget) {
        return {};
    }

    if (auto toolbar = qobject_cast<QToolBar*>(widget)) {
        return ToolBarManager::toolBarPersistenceKey(toolbar);
    }

    return widget->objectName();
}
QString widgetLegacyPersistenceKey(QWidget* widget)
{
    auto toolbar = qobject_cast<QToolBar*>(widget);
    if (!toolbar) {
        return {};
    }

    const auto legacyKey = toolbar->objectName();
    if (legacyKey.isEmpty() || legacyKey == ToolBarManager::toolBarPersistenceKey(toolbar)) {
        return {};
    }

    return legacyKey;
}

QWidget* findRestorableWidget(ToolBarAreaWidget* area, const QString& key)
{
    if (!area || key.isEmpty()) {
        return nullptr;
    }

    if (auto widget = area->findChild<QWidget*>(key)) {
        return widget;
    }

    for (auto toolbar : area->findChildren<QToolBar*>()) {
        if (ToolBarManager::toolBarPersistenceKey(toolbar) == key || toolbar->objectName() == key) {
            return toolbar;
        }
    }

    return nullptr;
}
}  // namespace

ToolBarAreaWidget::ToolBarAreaWidget(
    QWidget* parent,
    ToolBarArea area,
    const ParameterGrp::handle& hParam,
    fastsignals::advanced_scoped_connection& conn,
    QTimer* timer
)
    : QWidget(parent)
    , _sizingTimer(timer)
    , _hParam(hParam)
    , _conn(conn)
    , _area(area)
{
    _layout = new QHBoxLayout(this);
    _layout->setContentsMargins(QMargins());
}

void ToolBarAreaWidget::addWidget(QWidget* widget)
{
    // if widget already exist don't do anything
    if (_layout->indexOf(widget) >= 0) {
        return;
    }

    if (auto toolbar = qobject_cast<ToolBar*>(widget)) {
        toolbar->updateCustomGripVisibility();
    }

    _layout->addWidget(widget);
    adjustParent();

    QString name = widgetPersistenceKey(widget);

    if (!name.isEmpty()) {
        Base::ConnectionBlocker block(_conn);
        _hParam->SetInt(name.toUtf8().constData(), _layout->count() - 1);
        if (const auto legacyKey = widgetLegacyPersistenceKey(widget); !legacyKey.isEmpty()) {
            _hParam->RemoveInt(legacyKey.toUtf8().constData());
        }
    }
}

void ToolBarAreaWidget::insertWidget(int index, QWidget* widget)
{
    int currentIndex = _layout->indexOf(widget);

    // we are inserting widget at the same place, this is no-op
    if (currentIndex == index) {
        return;
    }

    // widget already exists in the area, we need to first remove it and then recreate
    if (currentIndex > 0) {
        _layout->removeWidget(widget);
    }

    _layout->insertWidget(index, widget);

    if (auto toolbar = qobject_cast<ToolBar*>(widget)) {
        toolbar->updateCustomGripVisibility();
    }

    adjustParent();
    saveState();
}

void ToolBarAreaWidget::removeWidget(QWidget* widget)
{
    _layout->removeWidget(widget);

    if (auto toolbar = qobject_cast<ToolBar*>(widget)) {
        toolbar->updateCustomGripVisibility();
    }

    QString name = widgetPersistenceKey(widget);
    if (!name.isEmpty()) {
        Base::ConnectionBlocker block(_conn);
        _hParam->RemoveInt(name.toUtf8().constData());
        if (const auto legacyKey = widgetLegacyPersistenceKey(widget); !legacyKey.isEmpty()) {
            _hParam->RemoveInt(legacyKey.toUtf8().constData());
        }
    }

    adjustParent();
}

void ToolBarAreaWidget::setParameters(const ParameterGrp::handle& hParam)
{
    _hParam = hParam;
}

void ToolBarAreaWidget::adjustParent()
{
    if (_sizingTimer) {
        _sizingTimer->start(10);
    }
}

void ToolBarAreaWidget::saveState()
{
    Base::ConnectionBlocker block(_conn);

    for (auto& v : _hParam->GetIntMap()) {
        _hParam->RemoveInt(v.first.c_str());
    }

    foreachToolBar([this](QToolBar* toolbar, int idx, ToolBarAreaWidget*) {
        auto key = ToolBarManager::toolBarPersistenceKey(toolbar);
        _hParam->SetInt(key.toUtf8().constData(), idx);
    });
}

void ToolBarAreaWidget::restoreState(
    const std::map<int, QToolBar*>& toolbars,
    const QMap<QString, bool>& widgetVisibility
)
{
    for (const auto& [index, toolbar] : toolbars) {
        bool visible = toolbar->isVisible();
        getMainWindow()->removeToolBar(toolbar);
        toolbar->setOrientation(Qt::Horizontal);
        insertWidget(index, toolbar);
        toolbar->setVisible(visible);
    }

    for (auto it = widgetVisibility.cbegin(); it != widgetVisibility.cend(); ++it) {
        auto widget = findRestorableWidget(this, it.key());

        if (widget) {
            widget->setVisible(it.value());
        }
    }
}
