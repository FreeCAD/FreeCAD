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
# include <QTextStream>
#endif

#include <Base/Tools.h>

#include <Gui/propertyeditor/PropertyItem.h>
#include "../App/PropertyConstraintList.h"
#include "PropertyConstraintListItem.h"


using namespace SketcherGui;
using namespace Gui::PropertyEditor;

TYPESYSTEM_SOURCE(SketcherGui::PropertyConstraintListItem, Gui::PropertyEditor::PropertyItem);

PropertyConstraintListItem::PropertyConstraintListItem()
{

}

QVariant PropertyConstraintListItem::toString(const QVariant& prop) const
{
    const QList<Base::Quantity>& value = prop.value< QList<Base::Quantity> >();
    QString str;
    QTextStream out(&str);
    out << "[";
    for (QList<Base::Quantity>::const_iterator it = value.begin(); it != value.end(); ++it) {
        if (it != value.begin())
            out << ";";
        out << it->getUserString();
    }
    out << "]";
    return QVariant(str);
}

void PropertyConstraintListItem::initialize()
{
    const Sketcher::PropertyConstraintList* list = static_cast<const Sketcher::PropertyConstraintList*>(getPropertyData()[0]);
    const std::vector< Sketcher::Constraint * > &vals = list->getValues();

    int id = 1;
    int iNamed = 0;

    std::vector<PropertyUnitItem *> unnamed;

    for (std::vector< Sketcher::Constraint* >::const_iterator it = vals.begin();it != vals.end(); ++it, ++id) {
        if ((*it)->Type == Sketcher::Distance || // Datum constraint
            (*it)->Type == Sketcher::DistanceX ||
            (*it)->Type == Sketcher::DistanceY ||
            (*it)->Type == Sketcher::Radius ||
            (*it)->Type == Sketcher::Angle ) {

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
            
            item->bind(list->createPath(id-1));
            item->setAutoApply(false);
        }
    }

    // now deal with the unnamed
    if (iNamed == 0) {
        onlyUnnamed = true;
        for (std::vector< PropertyUnitItem* >::const_iterator it = unnamed.begin();it != unnamed.end(); ++it) {
            (*it)->setParent(this);
            this->appendChild((*it));
        }
    }
    else {
        onlyUnnamed = false;
        if (!unnamed.empty()) {
            PropertyConstraintListItem* item = static_cast<PropertyConstraintListItem*>(PropertyConstraintListItem::create()); 
            item->setParent(this);
            item->setPropertyName(tr("Unnamed"));
            this->appendChild(item);

            for (std::vector< PropertyUnitItem* >::const_iterator it = unnamed.begin();it != unnamed.end(); ++it) {
                (*it)->setParent(item);
                item->appendChild((*it));
            }
        }
    }
}

QVariant PropertyConstraintListItem::value(const App::Property* prop) const
{
    assert(prop && prop->getTypeId().isDerivedFrom(Sketcher::PropertyConstraintList::getClassTypeId()));

    PropertyConstraintListItem* self = const_cast<PropertyConstraintListItem*>(this);

    int id = 1;

    QList<Base::Quantity> quantities;
    QList<Base::Quantity> subquantities;
    bool onlyNamed = true;

    const std::vector< Sketcher::Constraint * > &vals = static_cast<const Sketcher::PropertyConstraintList*>(prop)->getValues();
    for (std::vector< Sketcher::Constraint* >::const_iterator it = vals.begin();it != vals.end(); ++it, ++id) {
        if ((*it)->Type == Sketcher::Distance || // Datum constraint
            (*it)->Type == Sketcher::DistanceX ||
            (*it)->Type == Sketcher::DistanceY ||
            (*it)->Type == Sketcher::Radius ||
            (*it)->Type == Sketcher::Angle ) {

            Base::Quantity quant;
            if ((*it)->Type == Sketcher::Angle ) {
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
            PropertyConstraintListItem* self = const_cast<PropertyConstraintListItem*>(this);

            self->blockEvent = true;

            if ((*it)->Name.empty() && !onlyUnnamed) {
                onlyNamed = false;
                subquantities.append(quant);
                PropertyConstraintListItem* unnamednode = static_cast<PropertyConstraintListItem*>(self->child(self->childCount()-1));
                unnamednode->blockEvent = true;
                unnamednode->setProperty(internalName.toLatin1(), QVariant::fromValue<Base::Quantity>(quant));
                unnamednode->blockEvent = false;
            }
            else {
                self->setProperty(internalName.toLatin1(), QVariant::fromValue<Base::Quantity>(quant));
            }

            self->blockEvent = false;
        }
    }

    // The quantities of unnamed constraints are only needed for display purposes inside toString()
    if (!onlyUnnamed && !onlyNamed) {
        self->blockEvent = true;
        self->setProperty("Unnamed", QVariant::fromValue< QList<Base::Quantity> >(subquantities));
        self->blockEvent = false;
    }

    return QVariant::fromValue< QList<Base::Quantity> >(quantities);
}

bool PropertyConstraintListItem::event (QEvent* ev)
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
            if (this->parent()->getTypeId() == SketcherGui::PropertyConstraintListItem::getClassTypeId()) {
                item = static_cast<Sketcher::PropertyConstraintList*>(this->parent()->getFirstProperty());
            }
            else {
                item = static_cast<Sketcher::PropertyConstraintList*>(getFirstProperty());
            }

            const std::vector< Sketcher::Constraint * > &vals = item->getValues();
            for (std::vector< Sketcher::Constraint* >::const_iterator it = vals.begin();it != vals.end(); ++it, ++id) {
                if ((*it)->Type == Sketcher::Distance || // Datum constraint
                    (*it)->Type == Sketcher::DistanceX ||
                    (*it)->Type == Sketcher::DistanceY ||
                    (*it)->Type == Sketcher::Radius ||
                    (*it)->Type == Sketcher::Angle ) {

                    // Get the internal name
                    QString internalName = QString::fromLatin1("Constraint%1").arg(id+1);
                    if (internalName == propName) {
                        double datum = quant.getValue();
                        if ((*it)->Type == Sketcher::Angle)
                            datum = Base::toRadians<double>(datum);
                        const_cast<Sketcher::Constraint *>((*it))->setValue(datum);
                        item->set1Value(id,(*it));
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
}

QWidget* PropertyConstraintListItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    QLineEdit *le = new QLineEdit(parent);
    le->setFrame(false);
    le->setReadOnly(true);
    return le;
}

void PropertyConstraintListItem::setEditorData(QWidget *editor, const QVariant& data) const
{
    QLineEdit* le = qobject_cast<QLineEdit*>(editor);
    le->setText(toString(data).toString());
}

QVariant PropertyConstraintListItem::editorData(QWidget *editor) const
{
    QLineEdit *le = qobject_cast<QLineEdit*>(editor);
    return QVariant(le->text());
}

#include "moc_PropertyConstraintListItem.cpp"
