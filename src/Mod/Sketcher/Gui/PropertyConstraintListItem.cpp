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
# include <algorithm>
# include <QComboBox>
# include <QFontDatabase>
# include <QLayout>
# include <QLocale>
# include <QPixmap>
# include <QSpinBox>
# include <QTextStream>
# include <QTimer>
#endif

#include <Base/Tools.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/PropertyGeo.h>
#include <App/PropertyFile.h>
#include <App/PropertyUnits.h>
#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <Gui/Placement.h>
#include <Gui/FileDialog.h>
#include <Gui/DlgPropertyLink.h>
#include <Gui/QuantitySpinBox.h>

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

QVariant PropertyConstraintListItem::value(const App::Property* prop) const
{
    assert(prop && prop->getTypeId().isDerivedFrom(Sketcher::PropertyConstraintList::getClassTypeId()));
    
    PropertyConstraintListItem* self = const_cast<PropertyConstraintListItem*>(this);
    
    int id = 1;

    QList<Base::Quantity> quantities;
    const std::vector< Sketcher::Constraint * > &vals = static_cast<const Sketcher::PropertyConstraintList*>(prop)->getValues();
    for (std::vector< Sketcher::Constraint* >::const_iterator it = vals.begin();it != vals.end(); ++it, ++id) {
        if ((*it)->Type == Sketcher::Distance || // Datum constraint
            (*it)->Type == Sketcher::DistanceX ||
            (*it)->Type == Sketcher::DistanceY ||
            (*it)->Type == Sketcher::Radius ||
            (*it)->Type == Sketcher::Angle) {

            Base::Quantity quant;
            if ((*it)->Type == Sketcher::Angle) {
                double datum = Base::toDegrees<double>((*it)->Value);
                quant.setUnit(Base::Unit::Angle);
                quant.setValue(datum);
            }
            else {
                quant.setUnit(Base::Unit::Length);
                quant.setValue((*it)->Value);
            }

            quantities.append(quant);
            
            QString name = QString::fromStdString((*it)->Name);
            if (name.isEmpty())
                name = QString::fromLatin1("Constraint%1").arg(id);
            
            PropertyConstraintListItem* self = const_cast<PropertyConstraintListItem*>(this);
            self->blockEvent=true;
            self->setProperty(name.toLatin1(), QVariant::fromValue<Base::Quantity>(quant));
            self->blockEvent=false;    

        }
    }

    
    return QVariant::fromValue< QList<Base::Quantity> >(quantities);

}

void PropertyConstraintListItem::setValue(const QVariant& value)
{
    // see PropertyConstraintListItem::event
}

bool PropertyConstraintListItem::event (QEvent* ev)
{
    if (ev->type() == QEvent::DynamicPropertyChange) {
        if(!blockEvent) {
            QDynamicPropertyChangeEvent* ce = static_cast<QDynamicPropertyChangeEvent*>(ev);
            QVariant prop = property(ce->propertyName());
            QString propName = QString::fromLatin1(ce->propertyName());
            Base::Quantity quant = prop.value<Base::Quantity>();

            int id = 0;
            Sketcher::PropertyConstraintList* item = static_cast<Sketcher::PropertyConstraintList*>(getFirstProperty());
            
            const std::vector< Sketcher::Constraint * > &vals = item->getValues();
            for (std::vector< Sketcher::Constraint* >::const_iterator it = vals.begin();it != vals.end(); ++it, ++id) {
                if ((*it)->Type == Sketcher::Distance || // Datum constraint
                    (*it)->Type == Sketcher::DistanceX ||
                    (*it)->Type == Sketcher::DistanceY ||
                    (*it)->Type == Sketcher::Radius ||
                    (*it)->Type == Sketcher::Angle) {


                    // Get the name
                    QString name = QString::fromStdString((*it)->Name);
                    if (name.isEmpty())
                        name = QString::fromLatin1("Constraint%1").arg(id+1);
                    if (name == propName) {
                        double datum = quant.getValue();
                        if ((*it)->Type == Sketcher::Angle)
                            datum = Base::toRadians<double>(datum);
                        const_cast<Sketcher::Constraint *>((*it))->Value = datum;
                        item->set1Value(id,(*it));
                        break;
                    }
                }
            }
        }
    }
    return PropertyItem::event(ev);
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

void PropertyConstraintListItem::initialize()
{
    const Sketcher::PropertyConstraintList* item=static_cast<const Sketcher::PropertyConstraintList*>(getPropertyData()[0]);
    
    const std::vector< Sketcher::Constraint * > &vals = item->getValues();
       
    int id = 1;

    for (std::vector< Sketcher::Constraint* >::const_iterator it = vals.begin();it != vals.end(); ++it, ++id) {
        if ((*it)->Type == Sketcher::Distance || // Datum constraint
            (*it)->Type == Sketcher::DistanceX ||
            (*it)->Type == Sketcher::DistanceY ||
            (*it)->Type == Sketcher::Radius ||
            (*it)->Type == Sketcher::Angle) {

            
            // Get the name
            
            QString name = QString::fromStdString((*it)->Name);
            if (name.isEmpty())
                name = QString::fromLatin1("Constraint%1").arg(id);
            PropertyUnitItem* item = static_cast<PropertyUnitItem*>(PropertyUnitItem::create());
            item->setParent(this);
            item->setPropertyName(name);
            this->appendChild(item);            
        }
    }
    
}

#include "moc_PropertyConstraintListItem.cpp"