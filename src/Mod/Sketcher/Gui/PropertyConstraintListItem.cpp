/***************************************************************************
 * Copyright (c) 2014 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>        *
 *                                                                          *
 * This file is part of the FreeCAD CAx development system.                 *
 *                                                                          *
 * This library is free software; you can redistribute it and/or            *
 * modify it under the terms of the GNU Library General Public              *
 * License as published by the Free Software Foundation; either             *
 * version 2 of the License, or (at your option) any later version.         *
 *                                                                          *
 * This library is distributed in the hope that it will be useful,          *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the             *
 * GNU Library General Public License for more details.                     *
 *                                                                          *
 * You should have received a copy of the GNU Library General Public        *
 * License along with this library; see the file COPYING.LIB. If not,       *
 * write to the Free Software Foundation, Inc., 59 Temple Place,            *
 * Suite 330, Boston, MA 02111-1307, USA                                    *
 *                                                                          *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QDebug>
#include <QTextStream>
#include <memory>
#endif

#include <Base/Tools.h>
#include <Mod/Sketcher/App/PropertyConstraintList.h>

#include "PropertyConstraintListItem.h"


using namespace SketcherGui;
using namespace Gui::PropertyEditor;

PROPERTYITEM_SOURCE(SketcherGui::PropertyConstraintListItem)

PropertyConstraintListItem::PropertyConstraintListItem()
{
    blockEvent = false;
    onlyUnnamed = false;
}

PropertyConstraintListItem::~PropertyConstraintListItem()
{}

QVariant PropertyConstraintListItem::toString(const QVariant& prop) const
{
    const QList<Base::Quantity>& value = prop.value<QList<Base::Quantity>>();
    QString str;
    QTextStream out(&str);
    out << "[";
    for (QList<Base::Quantity>::const_iterator it = value.begin(); it != value.end(); ++it) {
        if (it != value.begin()) {
            out << ";";
        }
        out << it->getUserString();
    }
    out << "]";
    return QVariant(str);
}

void PropertyConstraintListItem::initialize()
{
    const Sketcher::PropertyConstraintList* list =
        static_cast<const Sketcher::PropertyConstraintList*>(getPropertyData()[0]);
    const std::vector<Sketcher::Constraint*>& vals = list->getValues();

    int id = 1;
    int iNamed = 0;

    std::vector<PropertyUnitItem*> unnamed;

    for (std::vector<Sketcher::Constraint*>::const_iterator it = vals.begin(); it != vals.end();
         ++it, ++id) {
        if ((*it)->Type == Sketcher::Distance ||  // Datum constraint
            (*it)->Type == Sketcher::DistanceX || (*it)->Type == Sketcher::DistanceY
            || (*it)->Type == Sketcher::Radius || (*it)->Type == Sketcher::Diameter
            || (*it)->Type == Sketcher::Angle) {

            PropertyUnitItem* item = static_cast<PropertyUnitItem*>(PropertyUnitItem::create());

            // Get the name
            QString internalName = QString::fromLatin1("Constraint%1").arg(id);
            QString name = QString::fromUtf8((*it)->Name.c_str());
            if (name.isEmpty()) {
                name = internalName;
                item->setPropertyName(name);
                unnamed.push_back(item);
            }
            else {
                iNamed++;
                item->setParent(this);
                item->setPropertyName(name);
                // The call of 'setPropertyName' calls 'setObjectName'. But 'name' can contain
                // some non-7-bit ASCII characters and thus the delegation of a property to the
                // parent item may fail because 'qPrintable(objectName())' is used therefore and
                // may return a different string. See 'PropertyItem::setData()' for more details.
                // To make the delegation to work properly we set a different object name which is
                // guaranteed to be 7-bit ASCII.
                //
                // See also PropertyConstraintListItem::value()
                // See also PropertyConstraintListItem::event()
                item->setObjectName(internalName);
                this->appendChild(item);
            }

            item->bind(list->createPath(id - 1));
            item->setAutoApply(false);
        }
    }

    // now deal with the unnamed
    if (iNamed == 0) {
        onlyUnnamed = true;
        for (std::vector<PropertyUnitItem*>::const_iterator it = unnamed.begin();
             it != unnamed.end();
             ++it) {
            (*it)->setParent(this);
            this->appendChild((*it));
        }
    }
    else {
        onlyUnnamed = false;
        if (!unnamed.empty()) {
            PropertyConstraintListItem* item =
                static_cast<PropertyConstraintListItem*>(PropertyConstraintListItem::create());
            item->setParent(this);
            item->setPropertyName(tr("Unnamed"));
            this->appendChild(item);

            for (std::vector<PropertyUnitItem*>::const_iterator it = unnamed.begin();
                 it != unnamed.end();
                 ++it) {
                (*it)->setParent(item);
                item->appendChild((*it));
            }
        }
    }
}

void PropertyConstraintListItem::assignProperty(const App::Property* prop)
{
    // Hint: When renaming a constraint that was unnamed before then it can happen that
    // a constraint appears twice in the property editor, one time in this group and a
    // second time inside the Unnamed group
    if (!prop->getTypeId().isDerivedFrom(Sketcher::PropertyConstraintList::getClassTypeId())) {
        return;
    }

    const Sketcher::PropertyConstraintList* list =
        static_cast<const Sketcher::PropertyConstraintList*>(prop);
    const std::vector<Sketcher::Constraint*>& vals = list->getValues();

    // search for the group of unnamed items if available and take it out
    int numUnnamed = 0;
    PropertyConstraintListItem* unnamed = nullptr;
    for (int i = this->childCount() - 1; i >= 0; i--) {
        unnamed = qobject_cast<PropertyConstraintListItem*>(this->child(i));
        if (unnamed) {
            numUnnamed = unnamed->childCount();
            this->takeChild(i);
            break;
        }
    }

    int id = 1;
    int namedIndex = 0;
    int unnamedIndex = 0;
    int numNamed = this->childCount();
    this->onlyUnnamed = true;

    for (std::vector<Sketcher::Constraint*>::const_iterator it = vals.begin(); it != vals.end();
         ++it, ++id) {
        if ((*it)->Type == Sketcher::Distance ||  // Datum constraint
            (*it)->Type == Sketcher::DistanceX || (*it)->Type == Sketcher::DistanceY
            || (*it)->Type == Sketcher::Radius || (*it)->Type == Sketcher::Diameter
            || (*it)->Type == Sketcher::Angle) {

            PropertyUnitItem* child = nullptr;
            if ((*it)->Name.empty()) {
                // search inside the group item for unnamed constraints
                if (!unnamed) {
                    unnamed = static_cast<PropertyConstraintListItem*>(
                        PropertyConstraintListItem::create());
                    unnamed->setPropertyName(tr("Unnamed"));
                }

                if (unnamedIndex < numUnnamed) {
                    child = static_cast<PropertyUnitItem*>(unnamed->child(unnamedIndex));
                }
                else {
                    child = static_cast<PropertyUnitItem*>(PropertyUnitItem::create());
                    unnamed->appendChild(child);
                    child->setParent(unnamed);
                }
                unnamedIndex++;
            }
            else {
                // search inside this item
                if (namedIndex < numNamed) {
                    child = dynamic_cast<PropertyUnitItem*>(this->child(namedIndex));
                }

                if (!child) {
                    child = static_cast<PropertyUnitItem*>(PropertyUnitItem::create());
                    this->appendChild(child);
                    child->setParent(this);
                }
                namedIndex++;
                this->onlyUnnamed = false;
            }

            // Get the name
            QString internalName = QString::fromLatin1("Constraint%1").arg(id);
            QString name = QString::fromUtf8((*it)->Name.c_str());
            if (name.isEmpty()) {
                name = internalName;
            }

            if (child->objectName() != internalName) {
                child->setPropertyName(name);
                child->setObjectName(internalName);

                child->bind(list->createPath(id - 1));
                child->setAutoApply(false);
            }
        }
    }

    // at the Unnamed group at very the end
    if (unnamed) {
        this->appendChild(unnamed);
        unnamed->setParent(this);
    }
}

QVariant PropertyConstraintListItem::value(const App::Property* prop) const
{
    assert(prop
           && prop->getTypeId().isDerivedFrom(Sketcher::PropertyConstraintList::getClassTypeId()));

    PropertyConstraintListItem* self = const_cast<PropertyConstraintListItem*>(this);

    int id = 1;

    QList<Base::Quantity> quantities;
    QList<Base::Quantity> subquantities;
    bool onlyNamed = true;

    const std::vector<Sketcher::Constraint*>& vals =
        static_cast<const Sketcher::PropertyConstraintList*>(prop)->getValues();
    for (std::vector<Sketcher::Constraint*>::const_iterator it = vals.begin(); it != vals.end();
         ++it, ++id) {
        if ((*it)->Type == Sketcher::Distance ||  // Datum constraint
            (*it)->Type == Sketcher::DistanceX || (*it)->Type == Sketcher::DistanceY
            || (*it)->Type == Sketcher::Radius || (*it)->Type == Sketcher::Diameter
            || (*it)->Type == Sketcher::Angle) {

            Base::Quantity quant;
            if ((*it)->Type == Sketcher::Angle) {
                double datum = Base::toDegrees<double>((*it)->getValue());
                quant.setUnit(Base::Unit::Angle);
                quant.setValue(datum);
            }
            else {
                quant.setUnit(Base::Unit::Length);
                quant.setValue((*it)->getValue());
            }

            quantities.append(quant);

            // Use a 7-bit ASCII string for the internal name.
            // See also comment in PropertyConstraintListItem::initialize()
            QString internalName = QString::fromLatin1("Constraint%1").arg(id);

            if ((*it)->Name.empty() && !onlyUnnamed) {
                onlyNamed = false;
                subquantities.append(quant);
                PropertyItem* child = self->child(self->childCount() - 1);
                PropertyConstraintListItem* unnamednode =
                    qobject_cast<PropertyConstraintListItem*>(child);
                if (unnamednode) {
                    unnamednode->blockEvent = true;
                    unnamednode->setProperty(internalName.toLatin1(),
                                             QVariant::fromValue<Base::Quantity>(quant));
                    unnamednode->blockEvent = false;
                }
                else {
                    qWarning() << "Item is not of type PropertyConstraintListItem but"
                               << typeid(*child).name();
                }
            }
            else {
                self->blockEvent = true;
                self->setProperty(internalName.toLatin1(),
                                  QVariant::fromValue<Base::Quantity>(quant));
                self->blockEvent = false;
            }
        }
    }

    // The quantities of unnamed constraints are only needed for display purposes inside toString()
    if (!onlyUnnamed && !onlyNamed) {
        self->blockEvent = true;
        self->setProperty("Unnamed", QVariant::fromValue<QList<Base::Quantity>>(subquantities));
        self->blockEvent = false;
    }

    return QVariant::fromValue<QList<Base::Quantity>>(quantities);
}

bool PropertyConstraintListItem::event(QEvent* ev)
{
    if (ev->type() == QEvent::DynamicPropertyChange) {
        if (!blockEvent) {
            QDynamicPropertyChangeEvent* ce = static_cast<QDynamicPropertyChangeEvent*>(ev);
            // Get property via internal name of a PropertyUnit
            QVariant prop = property(ce->propertyName());
            QString propName = QString::fromLatin1(ce->propertyName());
            Base::Quantity quant = prop.value<Base::Quantity>();

            Sketcher::PropertyConstraintList* item;

            int id = 0;
            if (dynamic_cast<SketcherGui::PropertyConstraintListItem*>(this->parent())) {
                item = static_cast<Sketcher::PropertyConstraintList*>(
                    this->parent()->getFirstProperty());
            }
            else {
                item = static_cast<Sketcher::PropertyConstraintList*>(getFirstProperty());
            }

            const std::vector<Sketcher::Constraint*>& vals = item->getValues();
            for (std::vector<Sketcher::Constraint*>::const_iterator it = vals.begin();
                 it != vals.end();
                 ++it, ++id) {
                if ((*it)->Type == Sketcher::Distance ||  // Datum constraint
                    (*it)->Type == Sketcher::DistanceX || (*it)->Type == Sketcher::DistanceY
                    || (*it)->Type == Sketcher::Radius || (*it)->Type == Sketcher::Diameter
                    || (*it)->Type == Sketcher::Angle) {

                    // Get the internal name
                    QString internalName = QString::fromLatin1("Constraint%1").arg(id + 1);
                    if (internalName == propName) {
                        double datum = quant.getValue();
                        if ((*it)->Type == Sketcher::Angle) {
                            datum = Base::toRadians<double>(datum);
                        }
                        std::unique_ptr<Sketcher::Constraint> copy((*it)->clone());
                        copy->setValue(datum);
                        item->set1Value(id, copy.get());
                        break;
                    }
                }
            }
        }
    }

    return PropertyItem::event(ev);
}

void PropertyConstraintListItem::setValue(const QVariant& value)
{
    // see PropertyConstraintListItem::event
    Q_UNUSED(value);
}

QWidget* PropertyConstraintListItem::createEditor(QWidget* parent,
                                                  const QObject* receiver,
                                                  const char* method) const
{
    Q_UNUSED(receiver);
    Q_UNUSED(method);
    QLineEdit* le = new QLineEdit(parent);
    le->setFrame(false);
    le->setReadOnly(true);
    return le;
}

void PropertyConstraintListItem::setEditorData(QWidget* editor, const QVariant& data) const
{
    QLineEdit* le = qobject_cast<QLineEdit*>(editor);
    le->setText(toString(data).toString());
}

QVariant PropertyConstraintListItem::editorData(QWidget* editor) const
{
    QLineEdit* le = qobject_cast<QLineEdit*>(editor);
    return QVariant(le->text());
}

#include "moc_PropertyConstraintListItem.cpp"
