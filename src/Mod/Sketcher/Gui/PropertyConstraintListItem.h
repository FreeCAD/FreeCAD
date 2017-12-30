
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
#ifndef PROPERTYCONSTRAINTLISTITEM_H
#define PROPERTYCONSTRAINTLISTITEM_H

#include <QObject>
#include <QPointer>
#include <QItemEditorFactory>
#include <vector>
#include <QList>

#include <Base/Type.h>
#include <Base/Quantity.h>
#include <Base/UnitsApi.h>
#include <App/PropertyStandard.h>
#include <Gui/Widgets.h>

#include <Gui/propertyeditor/PropertyItem.h>


namespace SketcherGui {

class PropertyConstraintListItem: public Gui::PropertyEditor::PropertyItem
{
    Q_OBJECT
    PROPERTYITEM_HEADER

    virtual ~PropertyConstraintListItem();
    virtual void assignProperty(const App::Property* prop);
    virtual QWidget* createEditor(QWidget* parent, const QObject* receiver, const char* method) const;
    virtual void setEditorData(QWidget *editor, const QVariant& data) const;
    virtual QVariant editorData(QWidget *editor) const;

protected:
    virtual QVariant toString(const QVariant&) const;
    virtual QVariant value(const App::Property*) const;
    virtual void setValue(const QVariant&);
    virtual bool event (QEvent* ev);

    virtual void initialize();

protected:
    PropertyConstraintListItem();
    bool blockEvent;
    bool onlyUnnamed;
};

} //namespace SketcherGui


#endif
