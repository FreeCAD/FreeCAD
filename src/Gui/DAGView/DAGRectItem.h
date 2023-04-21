/***************************************************************************
 *   Copyright (c) 2015 Thomas Anderson <blobfish[at]gmx.com>              *
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

#ifndef DAGRECTITEM_H
#define DAGRECTITEM_H

#include <QBrush>
#include <QGraphicsRectItem>


namespace Gui
{
  namespace DAG
  {
    /*all right I give up! the parenting combined with the zvalues is fubar!
     * you can't control any kind of layering between children of separate parents
     */
    class RectItem : public QGraphicsRectItem
    {
    public:
      explicit RectItem(QGraphicsItem* parent = nullptr);
      void setBackgroundBrush(const QBrush &brushIn){backgroundBrush = brushIn;}
      void setEditingBrush(const QBrush &brushIn){editBrush = brushIn;}
      void preHighlightOn(){preSelected = true;}
      void preHighlightOff(){preSelected = false;}
      void selectionOn(){selected = true;}
      void selectionOff(){selected = false;}
      bool isSelected(){return selected;}
      bool isPreSelected(){return preSelected;}
      void editingStart(){editing = true;}
      void editingFinished(){editing = false;}
      bool isEditing(){return editing;}
    protected:
      void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;
    private:
      QBrush backgroundBrush; //!< brush used for background. not used yet.
      QBrush editBrush; //!< brush used when object is in edit mode.
      //start with booleans, may expand to state.
      bool selected;
      bool preSelected;
      bool editing;
    };
  }
}

#endif // DAGRECTITEM_H
