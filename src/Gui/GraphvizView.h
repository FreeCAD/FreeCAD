/***************************************************************************
 *   Copyright (c) 2014 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef GUI_GRAPHVIZVIEW_H
#define GUI_GRAPHVIZVIEW_H

#include "MDIView.h"

class QGraphicsScene;
class QGraphicsView;

namespace Gui 
{
class GuiExport GraphvizView : public MDIView
{
    Q_OBJECT

public:
    GraphvizView(const QPixmap&, QWidget* parent=0);
    ~GraphvizView();

    void setDependencyGraph(const std::string&);
    QByteArray exportGraph(const QString& filter);

    /// Message handler
    virtual bool onMsg(const char* pMsg,const char** ppReturn);
    /// Message handler test
    virtual bool onHasMsg(const char* pMsg) const;
    /** @name Printing */
    //@{
    virtual void print(QPrinter* printer);
    /** Print content of view */
    virtual void print();
    /** Print to PDF file */
    virtual void printPdf();
    /** Show a preview dialog */
    virtual void printPreview();
    //@}

private:
    std::string graphCode;
    QGraphicsScene* scene;
    QGraphicsView* view;
};

} // namespace Gui

#endif // GUI_GRAPHVIZVIEW_H
