/***************************************************************************
 *   Copyright (c) 2011 Joe Dowsett <j-dowsett[at]users.sourceforge.net>   *
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
 
#ifndef GUI_TASKVIEW_TASKORTHOVIEWS_H
#define GUI_TASKVIEW_TASKORTHOVIEWS_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/TaskView/TaskDialog.h>
#include "ui_TaskOrthoViews.h"
#include <Base/BoundBox.h>

class Ui_TaskOrthoViews;

namespace DrawingGui {


class orthoView
{
public:
    orthoView(std::string, const char *, const char *, Base::BoundBox3d);
    ~orthoView();
    
    void activate(bool);
    void setDir(int);
    void setPos(float = 0, float = 0);
    void setScale(float);
    void setOrientation(int);
    void deleteme();
    void hidden(int);
    void smooth(int);

public:
    bool active;
    float width;
    float height;

private:
    void calcCentre();
    void updateView();

private:
    std::string myname;
    Base::BoundBox3d mybox;
    int dir;
    int angle;
    int orientation;
    float x, y;
    float pageX, pageY;
    float scale;
};






class TaskOrthoViews : public QWidget//: public Gui::TaskView::TaskBox
{
    Q_OBJECT

public:
    TaskOrthoViews(QWidget *parent = 0);
    ~TaskOrthoViews();
    bool user_input();
    void clean_up(bool);

protected Q_SLOTS:
    void setPrimary(int);
    void setRotate(int);
    void setSecondary_1(int);
    void setSecondary_2(int);
    void projectionChanged(int);
    void hidden(int);
    void smooth(int);
    void toggle_auto(int);
    void data_entered();

protected:
    void changeEvent(QEvent *);
    
private:
    void pagesize(std::string&);
    void compute();
    void autodims(float, float, float, float);
    void populate_s1();
    void populate_s2();
    
private:
    class Private;
    Ui_TaskOrthoViews * ui;
    orthoView * views[3];
    int transform[7][7][3];         //matrix containing relative positions and rotations of secondary views depending upon primary view
    int primary;                    //view direction of primary view
    int secondary_1, secondary_2;   //view direction of secondary views
    int spacing_1, spacing_2;       //spacings of secondary view centre from primary view centre
    float x_pos, y_pos;             //x and y coords for primary view
    int rotate;                     //rotate primary view clockwise by rotate*90
    int proj;                       //first (=-1) or third (=1) angle projection
    float scale;                    //scale of drawing
    bool autoscale;                 //whether or not to run autodims
    float pagewidth, pageh1, pageh2;  //these are actually the available width and height, calculated in constructor.
    int margin;
    int min_space;                  //minimum space between views, and page edge
};




//////////////////////////////////////////////////////////////



/// simulation dialog for the TaskView
class TaskDlgOrthoViews : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgOrthoViews();
    ~TaskDlgOrthoViews();

    
public:
    void open();
    bool accept();
    bool reject();
    void clicked(int);
    
//    QDialogButtonBox::StandardButtons getStandardButtons() const
//    { return QDialogButtonBox::Ok|QDialogButtonBox::Cancel; }

private:
    TaskOrthoViews * widget;
    Gui::TaskView::TaskBox* taskbox;

};

} //namespace DrawingGui


#endif // GUI_TASKVIEW_OrthoViews_H

