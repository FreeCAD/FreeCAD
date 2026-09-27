// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Ladislav Michl
// SPDX-FileNotice: Part of the FreeCAD project.

#include <QList>
#include <QMetaType>
#include <QString>

#include <Base/Quantity.h>
#include "MaterialMetaTypes.h"
#include "ValueVariant.h"

using namespace MatGui;

QVariant MatGui::toQVariant(const Materials::Value& value)
{
    if (value.is<std::string>()) {
        return QString::fromStdString(value.get<std::string>());
    }
    if (value.is<bool>()) {
        return value.get<bool>();
    }
    if (value.is<int>()) {
        return value.get<int>();
    }
    if (value.is<double>()) {
        return value.get<double>();
    }
    if (value.is<Base::Quantity>()) {
        return QVariant::fromValue(value.get<Base::Quantity>());
    }
    if (value.is<Materials::ValueList>()) {
        QList<QVariant> list;
        for (const auto& item : value.get<Materials::ValueList>()) {
            list.append(toQVariant(item));
        }
        return list;
    }
    return {};
}

Materials::Value MatGui::fromQVariant(const QVariant& variant)
{
    if (variant.isNull()) {
        return {};
    }
    if (variant.userType() == qMetaTypeId<Base::Quantity>()) {
        return variant.value<Base::Quantity>();
    }
    if (variant.userType() == qMetaTypeId<QList<QVariant>>()) {
        Materials::ValueList list;
        for (const auto& item : variant.value<QList<QVariant>>()) {
            list.push_back(fromQVariant(item));
        }
        return list;
    }
    switch (variant.userType()) {
        case QMetaType::Bool:
            return variant.toBool();
        case QMetaType::Int:
        case QMetaType::UInt:
        case QMetaType::Long:
        case QMetaType::LongLong:
            return variant.toInt();
        case QMetaType::Float:
        case QMetaType::Double:
            return variant.toDouble();
        default:
            return variant.toString().toStdString();
    }
}
