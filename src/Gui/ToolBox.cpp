/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QEvent>
# include <QGridLayout>
# include <QToolBox>
#endif

#include "ToolBox.h"


using namespace Gui::DockWnd;

/**
 * Constructs a toolbox called \a name with parent \a parent and flags \a f.
 */
ToolBox::ToolBox( QWidget *parent )
  : QWidget(parent)
{
  _pToolBox = new QToolBox( this );
  connect(_pToolBox, &QToolBox::currentChanged, this, &ToolBox::currentChanged);

  auto pGrid = new QGridLayout(this);
  pGrid->addWidget(_pToolBox, 0, 0);
}

ToolBox::~ToolBox()
{
  delete _pToolBox;
}

/**
 * Adds the widget w in a new tab at bottom of the toolbox. The new tab's label is set to \a label.
 * Returns the new tab's index.
 */
int ToolBox::addItem ( QWidget * w, const QString & label )
{
  return _pToolBox->addItem( w, label );
}

/**
 * Adds the widget \a item in a new tab at bottom of the toolbox. The new tab's label is set to \a label, and
 * the \a iconSet is displayed to the left of the \a label. Returns the new tab's index.
 */
int ToolBox::addItem ( QWidget * item, const QIcon & iconSet, const QString & label )
{
  return _pToolBox->addItem( item, iconSet, label );
}

/**
 * This is an overloaded member function, provided for convenience. It behaves essentially like the above function.
 *
 * Inserts the widget \a item at position \a index, or at the bottom of the toolbox if \a index is out of range.
 * The new item's label is set to \a label. Returns the new item's index.
 */
int ToolBox::insertItem ( int index, QWidget * item, const QString & label )
{
  return _pToolBox->insertItem( index, item, label );
}

/**
 * Inserts the widget \a item at position \a index, or at the bottom of the toolbox if \a index is out of range.
 * The new item's label is set to \a label, and the \a iconSet is displayed to the left of the \a label.
 * Returns the new item's index.
 */
int ToolBox::insertItem ( int index, QWidget * item, const QIcon & iconSet, const QString & label )
{
  return _pToolBox->insertItem( index, item, iconSet, label );
}

/**
 * Removes the widget \a item from the toolbox.
 * Returns the removed widget's index, or -1 if the widget was not in this tool box.
 */
void ToolBox::removeItem ( int index )
{
  _pToolBox->removeItem( index );
}

/**
 * If \a enabled is true then the item at position \a index is enabled; otherwise item \a index is disabled.
 */
void ToolBox::setItemEnabled ( int index, bool enabled )
{
  _pToolBox->setItemEnabled( index, enabled );
}

/**
 * Returns true if the item at position \a index is enabled; otherwise returns false.
 */
bool ToolBox::isItemEnabled ( int index ) const
{
  return _pToolBox->isItemEnabled( index );
}

/**
 * Sets the label of the item at position \a index to \a label.
 */
void ToolBox::setItemText ( int index, const QString & label )
{
  _pToolBox->setItemText( index, label );
}

/**
 * Returns the label of the item at position \a index, or a null string if \a index is out of range.
 */
QString ToolBox::itemText ( int index ) const
{
  return _pToolBox->itemText( index );
}

/**
 * Sets the icon of the item at position \a index to \a iconSet.
 */
void ToolBox::setItemIcon ( int index, const QIcon & iconSet )
{
  _pToolBox->setItemIcon( index, iconSet );
}

/**
 * Returns the icon of the item at position \a index, or a null icon if \a index is out of range.
 */
QIcon ToolBox::itemIcon ( int index ) const
{
  return _pToolBox->itemIcon( index );
}

/**
 * Sets the tooltip of the item at position \a index to \a toolTip.
 */
void ToolBox::setItemToolTip ( int index, const QString & toolTip )
{
  _pToolBox->setItemToolTip( index, toolTip );
}

/**
 * Returns the tooltip of the item at position \a index, or a null string if \a index is out of range.
 */
QString ToolBox::itemToolTip ( int index ) const
{
  return _pToolBox->itemToolTip( index );
}

/**
 * Returns the toolbox's current item, or 0 if the toolbox is empty.
 */
QWidget * ToolBox::currentWidget () const
{
  return _pToolBox->currentWidget();
}

/**
 * Sets the current item to be \a item.
 */
void ToolBox::setCurrentWidget ( QWidget * item )
{
  _pToolBox->setCurrentWidget( item );
}

/**
 * Returns the index of the current item, or -1 if the toolbox is empty.
 */
int ToolBox::currentIndex () const
{
  return _pToolBox->currentIndex();
}

/**
 * Returns the item at position \a index, or 0 if there is no such item.
 */
QWidget * ToolBox::widget ( int index ) const
{
  return _pToolBox->widget( index );
}

/**
 * Returns the index of item \a item, or -1 if the item does not exist.
 */
int ToolBox::indexOf ( QWidget * item ) const
{
  return _pToolBox->indexOf ( item );
}

/**
 * Returns the number of items contained in the toolbox.
 */
int ToolBox::count () const
{
  return _pToolBox->count();
}

/**
 * Sets the index of the current item, or -1 if the toolbox is empty to \a index.
 */
void ToolBox::setCurrentIndex ( int index )
{
  _pToolBox->setCurrentIndex( index );
}

/**
 * If to a new language is switched this method gets called.
 */
void ToolBox::changeEvent(QEvent *e)
{
  if (e->type() == QEvent::LanguageChange) {
    QWidget::changeEvent(e);
    int ct = count();
    for ( int i=0; i<ct; i++ ) {
      QWidget* w = widget( i );
      if ( w )
        setItemText( i, w->windowTitle() );
    }
  } else {
    QWidget::changeEvent(e);
  }
}

#include "moc_ToolBox.cpp"
