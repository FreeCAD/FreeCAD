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
    void setDir(float,float,float,float,int);
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
    float angle;
    float n[3];
    int orientation;
    float x, y;
    float pageX, pageY;
    float scale;
    bool axo;
    float vert[3];
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
    void cb_toggled(bool);
    void projectionChanged(int);
    void hidden(int);
    void smooth(int);
    void toggle_auto(int);
    void data_entered();
    void axoChanged(int);
    void axoTopChanged(int);
    void axo_flip();
    void axoScale();

protected:
    void changeEvent(QEvent *);

private:
    void pagesize(std::string&);
    void autodims();
    void compute();
    void validate_cbs();
    void view_data(int, int, int &, int &);
    void updateSecondaries();
    void set_axo();

private:
    class Private;
    Ui_TaskOrthoViews * ui;
    orthoView * views[4];
    QCheckBox * c_boxes[5][5];      //matrix of pointers to gui checkboxes
    QLineEdit * inputs[5];          //pointers to manual position/scale boxes
    float * data[5];                //pointers to scale, x_pos, y_pos, horiz, vert

    int map1[4][3][2];              //contains view directions and rotations for vertical secondary positions, for primaries 1,2,3,4
    int map2[4][3][2];              //contains view directions and rotations for H and V secondary positions, primaries 5,6
    float axonometric[3][6][4][4];  //contains view direction vectors and rotations for axonometric views

    int view_status[4][4];          //matrix containing status of four orthoView objects (in use, axo, rel x, rel y)
    int view_count;                 //number of active views

    int primary;                    //view direction of primary view
    float x_pos, y_pos;             //x and y coords for primary view
    int rotate;                     //rotate primary view clockwise by rotate*90
    int proj;                       //first (=-1) or third (=1) angle projection
    float scale;                    //scale of drawing
    bool autoscale;                 //whether or not to run autodims

    float horiz, vert;              //centre-centre distances

    bool axo_flipped;
    int axo;

    float pagewidth, pageheight;      //these are actually the available width and height, calculated in constructor.
    float pageh1, pageh2;             //h1 - total usable page height, h2 - total height allowing for info box.
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

