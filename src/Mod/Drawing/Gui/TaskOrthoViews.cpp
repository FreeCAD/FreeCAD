/***************************************************************************
 *   Copyright (c) 2012 Joe Dowsett <j-dowsett[at]users.sourceforge.net>   *
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
# include <QMessageBox>
#endif

#include "TaskOrthoViews.h"
#include "ui_TaskOrthoViews.h"

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Base/BoundBoxPy.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Drawing/App/FeaturePage.h>

#include <Base/FileInfo.h>
#include <iostream>
#include <Standard_Failure.hxx>

using namespace Gui;
using namespace DrawingGui;
using namespace std;


#if 0 // needed for Qt's lupdate utility
    qApp->translate("QObject", "Front");
    qApp->translate("QObject", "Back");
    qApp->translate("QObject", "Right");
    qApp->translate("QObject", "Left");
    qApp->translate("QObject", "Top");
    qApp->translate("QObject", "Bottom");
#endif

QString number_to_name(int j)
{
    char * temp[] = {"Front","Right","Back","Left","Top","Bottom","Axonometric"};
    QString translated = QObject::tr(temp[j]);
    return translated;
}

void rotate_coords(float& x, float& y, int i)
{
    float temp[4][2] =
    {
        { x, y},
        {-y, x},
        {-x,-y},
        { y,-x}
    };

    float t1 = temp[i][0];
    float t2 = temp[i][1];
    x = t1;
    y = t2;
}

void rotate_coords(int& x, int& y, int i)
{
    int temp[4][2] =
    {
        { x, y},
        {-y, x},
        {-x,-y},
        { y,-x}
    };

    int t1 = temp[i][0];
    int t2 = temp[i][1];
    x = t1;
    y = t2;
}

void rotate_coords(float & x, float & y, float angle)
{
    float tx = x * cos(angle) - y * sin(angle);
    y = x * sin(angle) + y * cos(angle);
    x = tx;
}

float dot(float * r, float * z)
{
    return ( r[0]*z[0] + r[1]*z[1] + r[2]*z[2]);
}

void cross(float * r, float * n, float * p)
{
    p[0] = r[1]*n[2] - r[2]*n[1];
    p[1] = r[2]*n[0] - r[0]*n[2];
    p[2] = r[0]*n[1] - r[1]*n[0];
}

void project(float * r, float * n, float * p)
{
    // for r projected onto plane perpendicular to n
    // r x n is perpendicular to r and n (.: lies on plane)
    // then n x (r x n) is perpendicular to that and to n, is the projection
    float c[3];
    cross(r, n, c);
    cross(n, c, p);
}

void normalise(float * r)
{
    float m = 1/sqrt(r[0]*r[0] + r[1]*r[1] + r[2]*r[2]);
    r[0] *= m;
    r[1] *= m;
    r[2] *= m;
}





orthoView::orthoView(std::string name, const char * targetpage, const char * sourcepart, Base::BoundBox3d partbox)
{
    myname = name;
    mybox = partbox;

    orientation = 0;
    angle = 0;
    pageX = 0;
    pageY = 0;
    scale = 1;
    x = 0;
    y = 0;
    dir = 0;
    active = true;
    axo = false;

    Command::doCommand(Command::Doc,"App.activeDocument().addObject('Drawing::FeatureViewPart','%s')",myname.c_str());
    Command::doCommand(Command::Doc,"App.activeDocument().%s.Source = App.activeDocument().%s",myname.c_str(), sourcepart);
    Command::doCommand(Command::Doc,"App.activeDocument().%s.addObject(App.activeDocument().%s)",targetpage,myname.c_str());
    Command::doCommand(Command::Doc,"App.activeDocument().%s.Direction = (1,0,0)",myname.c_str());

    activate(false);
}


orthoView::~orthoView()
{
}


void orthoView::deleteme()
{
    Command::doCommand(Command::Doc,"App.activeDocument().removeObject('%s')", myname.c_str());
}


void orthoView::activate(bool state)
{
    if (state)
    {
        active = true;
        Command::doCommand(Command::Doc,"Gui.ActiveDocument.getObject('%s').Visibility = True", myname.c_str());
        //Visibility doesn't seem to work, workaround (to counter any problems caused by active = false workaround):
        //setPos();
    }
    else
    {
        active = false;
        Command::doCommand(Command::Doc,"Gui.ActiveDocument.getObject('%s').Visibility = False", myname.c_str());
        //Visibility doesn't seem to work, workaround:
        Command::doCommand(Command::Doc,"App.activeDocument().%s.X = -10000", myname.c_str());
        Command::doCommand(Command::Doc,"App.activeDocument().%s.Y = -10000", myname.c_str());
        dir = 0;
        width = 0;
        height = 0;
    }
}


void orthoView::setDir(int i)
{
    axo = false;
    dir = i;
    int vx = (dir == 1) - (dir == 3);
    int vy = (dir == 0) - (dir == 2);
    int vz = (dir == 4) - (dir == 5);

    //drawings default to funny orientations for each view, line below calculates rotation to correct this
    //drawing will then be oriented correctly for it being primary view, setOrientation is used to modify this for secondary views.
    angle = -90*vx - 90*vy + (vz == -1)*180;

    calcCentre();

    if (active)
    {
        Command::doCommand(Command::Doc,"App.activeDocument().%s.Direction = (%d,%d,%d)",myname.c_str(),vx,vy,vz);
        Command::doCommand(Command::Doc,"App.activeDocument().%s.Label = '%s'",myname.c_str(),number_to_name(i).toStdString().c_str());
    }
}


void orthoView::setDir(float vx, float vy, float vz, float ang, int vert_index)
{
    //calcCentre();
    vert[0] = 0;
    vert[1] = 0;
    vert[2] = 0;

    switch(vert_index)
    {
    case 0:
        vert[1] = -1;
        break;
    case 1:
        vert[0] = 1;
        break;
    case 2:
        vert[1] = 1;
        break;
    case 3:
        vert[0] = -1;
        break;
    case 4:
        vert[2] = 1;
        break;
    case 5:
        vert[2] = -1;
    }

    axo = true;
    n[0] = vx;
    n[1] = vy;
    n[2] = vz;
    angle = ang;
    setOrientation(0);

    if (active)
    {
        Command::doCommand(Command::Doc,"App.activeDocument().%s.Direction = (%f,%f,%f)",myname.c_str(),vx,vy,vz);
        Command::doCommand(Command::Doc,"App.activeDocument().%s.Label = '%s'",myname.c_str(),number_to_name(6).toStdString().c_str());
    }
}


void orthoView::setPos(float px, float py)
{
    if (px != 0 && py !=0)
    {
        pageX = px;
        pageY = py;
    }

    float ox = pageX + x;
    float oy = pageY + y;

    if (active)
    {
        Command::doCommand(Command::Doc,"App.activeDocument().%s.X = %f", myname.c_str(), ox);
        Command::doCommand(Command::Doc,"App.activeDocument().%s.Y = %f", myname.c_str(), oy);
    }
}


void orthoView::setScale(float newScale)
{
    scale = newScale;
    if (active)
        Command::doCommand(Command::Doc,"App.activeDocument().%s.Scale = %f",myname.c_str(), scale);
    calcCentre();
}


void orthoView::setOrientation(int orient)
{
    orientation = orient;
    if (active)
        Command::doCommand(Command::Doc,"App.activeDocument().%s.Rotation = %f", myname.c_str(), (90*orientation+angle));
    calcCentre();
}


void orthoView::calcCentre()
{
//the drawing view 'Position' refers to where the part origin should be on the page
//we need to find the position of the centre of correct bounding box face relative to the part origin
//this depends upon:
// - which view (eg Front, Left, Top etc), given by 'dir'
// - the scale of the drawing, since eg centre.x is the absolute point, while x is the distance on page.
// - the orientation of the view on the page, will switch x/y as per a rotation.

    float cx = mybox.CalcCenter().x;
    float cy = mybox.CalcCenter().y;
    float cz = mybox.CalcCenter().z;

    if (axo)
    {
        float p[3] = {cx, -cy, cz};
        float n_p[3] = {n[0], -n[1], n[2]};
        float proj_p[3];
        float proj_y[3];            // will be the y axis of the projection
        float proj_x[3];            //  will be the x axis of the projection

        project(vert, n_p, proj_y);
        //project(p, n, proj_p);
        cross(proj_y, n_p, proj_x);
        normalise(proj_x);
        normalise(proj_y);
        x = -scale * dot(p, proj_x);
        y = scale * dot(p, proj_y);
        //rotate_coords(x, y, angle)
    }
    else
    {
        float coords[6][2] =
        {
            {-cx, cz},      //front
            { cy, cz},      //right
            { cx, cz},      //back
            {-cy, cz},      //left
            {-cx, -cy},     //top
            {-cx, cy}       //bottom
        };

        x = coords[dir][0] * scale;
        y = coords[dir][1] * scale;
        rotate_coords(x,y,orientation);

        float dx = mybox.LengthX();
        float dy = mybox.LengthY();
        float dz = mybox.LengthZ();

        float dims[6][2] =
        {
            {dx, dz},      //front
            {dy, dz},      //right
            {dx, dz},      //back
            {dy, dz},      //left
            {dx, dy},      //top
            {dx, dy}       //bottom
        };

        width = dims[dir][0];
        height = dims[dir][1];
        if (orientation % 2 == 1)
        {
            float temp = width;
            width = height;
            height = temp;
        }
    }
}


void orthoView::hidden(int state)
{
    if (state == 2)
        Command::doCommand(Command::Doc,"App.activeDocument().%s.ShowHiddenLines = True", myname.c_str());
    else
        Command::doCommand(Command::Doc,"App.activeDocument().%s.ShowHiddenLines = False", myname.c_str());
}


void orthoView::smooth(int state)
{
    if (state == 2)
        Command::doCommand(Command::Doc,"App.activeDocument().%s.ShowSmoothLines = True", myname.c_str());
    else
        Command::doCommand(Command::Doc,"App.activeDocument().%s.ShowSmoothLines = False", myname.c_str());
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////


TaskOrthoViews::TaskOrthoViews(QWidget *parent)
  : ui(new Ui_TaskOrthoViews)
{
    ui->setupUi(this);

	std::vector<App::DocumentObject*> obj = Gui::Selection().getObjectsOfType(Part::Feature::getClassTypeId());
	Base::BoundBox3d bbox;
    std::vector<App::DocumentObject*>::iterator it = obj.begin();
	bbox.Add(static_cast<Part::Feature*>(*it)->Shape.getBoundingBox());

    const char * part = obj.front()->getNameInDocument();
    App::Document* doc = App::GetApplication().getActiveDocument();

    std::vector<App::DocumentObject*> pages = doc->getObjectsOfType(Drawing::FeaturePage::getClassTypeId());
    std::string PageName = pages.front()->getNameInDocument();
    const char * page = PageName.c_str();

    App::DocumentObject * this_page = doc->getObject(page);
    std::string template_name = static_cast<Drawing::FeaturePage*>(this_page)->Template.getValue();

    std::string name1 = doc->getUniqueObjectName("Ortho").c_str();
    views[0] = new orthoView(name1, page, part, bbox);
    name1 = doc->getUniqueObjectName("Ortho").c_str();
    views[1] = new orthoView(name1, page, part, bbox);
    name1 = doc->getUniqueObjectName("Ortho").c_str();
    views[2] = new orthoView(name1, page, part, bbox);
    name1 = doc->getUniqueObjectName("Ortho").c_str();
    views[3] = new orthoView(name1, page, part, bbox);

    margin = 10;
    pagesize(template_name);
    min_space = 15;

    //   [x+2][y+2]
    c_boxes[0][2] = ui->cb02;       //left most, x = -2, y = 0
    c_boxes[1][1] = ui->cb11;
    c_boxes[1][2] = ui->cb12;
    c_boxes[1][3] = ui->cb13;
    c_boxes[2][0] = ui->cb20;       //top most, x = 0, y = -2
    c_boxes[2][1] = ui->cb21;
    c_boxes[2][2] = ui->cb22;       //centre (primary view) checkbox x = y = 0.
    c_boxes[2][3] = ui->cb23;
    c_boxes[2][4] = ui->cb24;       //bottom most, x = 0, y = 2
    c_boxes[3][1] = ui->cb31;
    c_boxes[3][2] = ui->cb32;
    c_boxes[3][3] = ui->cb33;
    c_boxes[4][2] = ui->cb42;       //right most, x = 2, y = 0

    for (int i=0; i < 5; i++)
        for (int j=0; j < 5; j++)
            if ((abs(i-2) + abs(j-2)) < 3)                          //if i,j combination corresponds to valid check box, then proceed with:
                connect(c_boxes[i][j], SIGNAL(toggled(bool)), this, SLOT(cb_toggled(bool)));

    inputs[0] = ui->scale_0;
    inputs[1] = ui->x_1;
    inputs[2] = ui->y_2;
    inputs[3] = ui->spacing_h_3;
    inputs[4] = ui->spacing_v_4;
    ui->tabWidget->setTabEnabled(1,false);

    for (int i=0; i < 5; i++)
        connect(inputs[i], SIGNAL(editingFinished()), this, SLOT(data_entered()));

    connect(ui->projection, SIGNAL(currentIndexChanged(int)), this, SLOT(projectionChanged(int)));
    connect(ui->rotate, SIGNAL(currentIndexChanged(int)), this, SLOT(setRotate(int)));
    connect(ui->smooth, SIGNAL(stateChanged(int)), this, SLOT(smooth(int)));
    connect(ui->hidden, SIGNAL(stateChanged(int)), this, SLOT(hidden(int)));
    connect(ui->auto_tog, SIGNAL(stateChanged(int)), this, SLOT(toggle_auto(int)));
    connect(ui->primary, SIGNAL(activated(int)), this, SLOT(setPrimary(int)));

    connect(ui->axoProj, SIGNAL(activated(int)), this, SLOT(axoChanged(int)));
    connect(ui->axoTop, SIGNAL(activated(int)), this, SLOT(axoTopChanged(int)));
    connect(ui->axoLeft, SIGNAL(activated(int)), this, SLOT(axoChanged(int)));
    connect(ui->flip, SIGNAL(clicked()), this, SLOT(axo_flip()));
    connect(ui->axoScale, SIGNAL(editingFinished()), this, SLOT(axoScale()));

    //these matrices contain information relating relative position on page to which view appears there, and in which orientation

    //first matrix is for front, right, back, left.  Only needs to contain positions above and below primary since in positions horizontally
    //displaced, the view appearing there is simply found from the same f, r, b, l sequence.
    //thus [i][j][k], i values are which primary view (0-3), j is vertical displacement corresponding to (-2/+2, -1, 1)
    //then in k = 0 is the view number, k = 1 is the orientation (*90 clockwise)

    //second matrix is for primaries top (4), i=0 gives horizontal positions, i = 1 vertical ones
    //and bottom (5) i = 2 (horiz) and 3 (vert).
    //then j and k values as for first matrix.

    int temp1[4][3][2] =   {{{2,2}, {4,0},  {5,0}},     //primary 0,        secondary {direction, rotation} in y = -2, -1, 1 positions
                            {{3,2}, {4,1},  {5,3}},     //primary 1,        secondaries in y = 2 position duplicate y = -2
                            {{0,2}, {4,2},  {5,2}},     //primary 2,        secondaries in horizontal positions x = -2, -1, 1, 2
                            {{1,2}, {4,3},  {5,1}}};    //primary 3,        given by linear position from primary = (p + x) mod 4

    int temp2[4][3][2] =   {{{5,2}, {3,1},  {1,3}},     //primary 4, secondaries in horizontal x = -2, -1, 1    (x = 2 duplicates x = -2)
                            {{5,0}, {2,2},  {0,0}},     //primary 4, vertical positions
                            {{4,2}, {3,3},  {1,1}},     //primary 5, horizontal
                            {{4,0}, {0,0},  {2,2}}};    //primary 5, vertical

    for (int i=0; i < 4; i++)
        for (int j=0; j < 3; j++)
            for (int k=0; k < 2; k++)               //initialisation needs to be done this way (rather than initialise maps directly
            {
                map1[i][j][k] = temp1[i][j][k];     //in order to avoid compiler warning
                map2[i][j][k] = temp2[i][j][k];
            }

    float temp[3][6][4][4] =
    // isometric
        {{{{1,1,1,180},{-1,1,-1,180},{-1,1,1,180},{1,1,-1,180}},     // top face is the  Right
        {{1,1,-1,60},{1,-1,1,240},{1,1,1,300},{1,-1,-1,120}},     //                  Front
        {{1,-1,-1,0},{-1,-1,1,0},{1,-1,1,0},{-1,-1,-1,0}},     //                  Left
        {{-1,1,1,60},{-1,-1,-1,240},{-1,-1,1,120},{-1,1,-1,300}},     //                  Back
        {{1,1,1,60},{1,-1,1,120},{-1,-1,1,240},{-1,1,1,300}},     //                  Top
        {{-1,1,-1,60},{1,1,-1,300},{1,-1,-1,240},{-1,-1,-1,120}}},     //                  Bottom

     // dimetric
        {{{0.681,0.267,0.681,180},{-0.681,0.267,-0.681,180},{-0.681,0.267,0.681,180},{0.681,0.267,-0.681,180}},     // top face is the  Right
        {{0.267,0.681,-0.681,0},{0.267,-0.681,0.681,0},{0.267,0.681,0.681,0},{0.267,-0.681,-0.681,0}},     //                  Front
        {{0.681,-0.267,-0.681,0},{-0.681,-0.267,0.681,0},{0.681,-0.267,0.681,0},{-0.681,-0.267,-0.681,0}},     //                  Left
        {{-0.267,0.681,0.681,180},{-0.267,-0.681,-0.681,180},{-0.267,-0.681,0.681,180},{-0.267,0.681,-0.681,180}},     //                  Back
        {{0.681,0.681,0.267,0},{0.681,-0.681,0.267,0},{-0.681,-0.681,0.267,0},{-0.681,0.681,0.267,0}},     //                  Top
        {{-0.681,0.681,-0.267,180},{0.681,0.681,-0.267,180},{0.681,-0.681,-0.267,180},{-0.681,-0.681,-0.267,180}}},     //                  Bottom

    // trimetric
        {{{0.211,0.577,0.788,-98.8},{-0.211,0.577,-0.788,81.2},{-0.788,0.577,0.211,81.2},{0.788,0.577,-0.211,-98.8}},     // top face is the  Right
        {{0.577,0.211,-0.788,81.2},{0.577,-0.211,0.788,-98.8},{0.577,0.788,0.211,-98.8},{0.577,-0.788,-0.211,81.2}},     //                  Front
        {{0.211,-0.577,-0.788,-98.8},{-0.211,-0.577,0.788,81.2},{0.788,-0.577,0.211,81.2},{-0.788,-0.577,-0.211,-98.8}},     //                  Left
        {{-0.577,0.211,0.788,81.2},{-0.577,-0.211,-0.788,-98.8},{-0.577,-0.788,0.211,-98.8},{-0.577,0.788,-0.211,81.2}},     //                  Back
        {{0.788,0.211,0.577,-98.8},{0.211,-0.788,0.577,81.2},{-0.788,-0.211,0.577,81.2},{-0.211,0.788,0.577,-98.8}},     //                  Top
        {{-0.788,0.211,-0.577,-98.8},{0.211,0.788,-0.577,81.2},{0.788,-0.211,-0.577,81.2},{-0.211,-0.788,-0.577,-98.8}}}};     //                  Bottom

    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 6; j++)
            for (int k = 0; k < 4; k++)
                for (int l = 0; l < 4; l++)
                    axonometric[i][j][k][l] = temp[i][j][k][l];

    //initialise variables

    for (int i=0; i < 4; i++)
        for (int j=0; j < 4; j++)
            view_status[i][j] = 0;

    view_count = 0;
    primary = 0;
    rotate = 0;
    proj = 1;
    autoscale = 1;
    axo_flipped = false;

    //below are calculated in case autodims is deselected before these values are initialised.
    float max_dim = max(max(bbox.LengthX(), bbox.LengthY()), bbox.LengthZ());
    scale = min(pagewidth, pageheight)/(4*max_dim+5*min_space);
    horiz = scale*max_dim + min_space;
    vert = horiz;
    x_pos = pagewidth/2;
    y_pos = pageheight/2;

    data[0] = &scale;
    data[1] = &x_pos;
    data[2] = &y_pos;
    data[3] = &horiz;
    data[4] = &vert;


//    Command::doCommand(Command::Doc,"#%d", map1[2][2][1]);

} //end of constructor


TaskOrthoViews::~TaskOrthoViews()
{
    delete views[0];
    delete views[1];
    delete views[2];
    delete views[3];
    delete ui;
}


void TaskOrthoViews::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}


void TaskOrthoViews::pagesize(std::string& page_template)
{
   // /********update num_templates when adding extra templates*******************/

    const int num_templates = 2;
    std::string templates[num_templates] = {"A3_Landscape.svg", "A4_Landscape.svg"};
    int dimensions[num_templates][3] = {{420,297,227},{297,210,153}};       //{width, full height, reduced height}  measured from page edge.

    for (int i=0; i < num_templates; i++)
    {
//        if (templates[i] == page_template)
        if (page_template.find(templates[i]) != std::string::npos)
        {
            pagewidth = dimensions[i][0] - 2*margin;
            pageh1 = dimensions[i][1] - 2*margin;
            pageh2 = dimensions[i][2] - margin;
            return;
        }
    }

    //matching template not found, read through template file for width & height info.

    //code below copied from FeaturePage.cpp
    Base::FileInfo fi(page_template);
    if (!fi.isReadable())
    {
        fi.setFile(App::Application::getResourceDir() + "Mod/Drawing/Templates/" + fi.fileName());
        if (!fi.isReadable())       //if so then really shouldn't have been able to get this far, but just in case...
        {
            pagewidth = 277;
            pageh1 = 190;
            pageh2 = 190;
            return;
        }
    }

    // open Template file
    std::string line;
    std::string temp_line;
    std::ifstream file (fi.filePath().c_str());
    size_t found;
    bool done = false;

    try
    {
        while (!file.eof())
        {
            getline (file,line);
            found = line.find("width=");
            if (found != std::string::npos)
            {
                temp_line = line.substr(7+found);
                sscanf (temp_line.c_str(), "%f", &pagewidth);
                pagewidth -= 2*margin;

                if (done)
                {
                    file.close();
                    return;
                }
                else
                    done = true;
            }

            found = line.find("height=");
            if (found != std::string::npos)
            {
                temp_line = line.substr(8+found);
                sscanf (temp_line.c_str(), "%f", &pageh1);
                pageh1 -= 2*margin;
                pageh2 = pageh1;

                if (done)
                {
                    file.close();
                    return;
                }
                else
                    done = true;
            }

            if (line.find("metadata") != std::string::npos)      //give up if we meet a metadata tag
                break;
        }
    }
    catch (Standard_Failure)
    { }

    file.close();

    //width/height not found??  fallback to A4 landscape simple:
    pagewidth = 277;
    pageh1 = 190;
    pageh2 = 190;
}


void TaskOrthoViews::autodims()
{
    /************************************* calculate real size of views in layout *****************/

    float w1 = 0, w2 = 0, h1 = 0, h2 = 0;
    int min_x = 0, min_y = 0;
    int max_x = 0, max_y = 0;

    w1 = views[0]->width;                                   //w1,h1 are width/height of primary view
    h1 = views[0]->height;                                  //w2 width of first view left/right of primary, h2 height of first view up/down

    for (int i = 0; i < 4; i++)
    {
        min_x = min(min_x, view_status[i][2]);
        min_y = min(min_y, view_status[i][3]);
        max_x = max(max_x, view_status[i][2]);
        max_y = max(max_y, view_status[i][3]);

        if (abs(view_status[i][2]) == 1 && abs(view_status[i][3]) == 0)
            w2 = views[i]->width;
        else if (abs(view_status[i][2]) == 0 && abs(view_status[i][3]) == 1)
            h2 = views[i]->height;
    }

    float width = (min_x == -2)*w1 + (min_x < 0)*w2 + w1 + (max_x > 0)*w2 + (max_x == 2) * w1;
    float height = (min_y == -2)*h1 + (min_y < 0)*h2 + h1 + (max_y > 0)*h2 + (max_y == 2) * h1;
    int wide = max_x - min_x + 1;                           //how many views wide / high?
    int high = max_y - min_y + 1;

    if (max_y > 0 && !c_boxes[3][3]->isChecked() && min_x == 0 && ((max_x == 1)*w2 > w1 || max_x == 2))
        pageheight = pageh1;
    else
        pageheight = pageh2;

    /*************************************** calculate scale **************************************/

    float working_scale = min((pagewidth - (wide + 1) * min_space) / width, (pageheight - (high + 1) * min_space) / height);

    //which gives the largest scale for which the min_space requirements can be met, but we want a 'sensible' scale, rather than 0.28457239...
    //eg if working_scale = 0.115, then we want to use 0.1, similarly 7.65 -> 5, and 76.5 -> 50

    float exponent = floor(log10(working_scale));                       //if working_scale = a * 10^b, what is b?
    working_scale *= pow(10, -exponent);                                //now find what 'a' is.

    float valid_scales[2][8] = {{1, 1.25, 2, 2.5, 3.75, 5, 7.5, 10},    //equate to 1:10, 1:8, 1:5, 1:4, 3:8, 1:2, 3:4, 1:1
                                {1, 1.5, 2, 3, 4, 5, 8, 10}};           //equate to 1:1, 3:2, 2:1, 3:1, 4:1, 5:1, 8:1, 10:1

    int i = 0;
    while (valid_scales[(exponent>=0)][i] <= working_scale)             //choose closest value smaller than 'a' from list.
        i += 1;                                                         //choosing top list if exponent -ve, bottom list for +ve exponent
    i -= 1;
    float chosen_scale = valid_scales[(exponent>=0)][i];
    scale = chosen_scale * pow(10, exponent);                           //now have the appropriate scale, reapply the *10^b


    /************************************* calculate position of views on page ********************/

    width *= scale;
    height *= scale;
    w1 *= scale;
    w2 *= scale;
    h1 *= scale;
    h2 *= scale;

    float space = min((pagewidth - width)/(wide + 1), (pageheight - height)/(high + 1));
    vert = space + (h1 + h2)/2;                                         //centre-centre spacing of views
    horiz = space + (w1 + w2)/2;

    float left = -min_x * horiz + (min_x == -1) * w2/2 + (min_x != -1) * w1/2;      //width of layout left of primary centre
    float right = max_x * horiz + (max_x == 1) * w2/2 + (max_x != 1) * w1/2;        //  "         "   right     "       "
    float up = -min_y * vert + (min_y == -1) * h2/2 + (min_y != -1) * h1/2;
    float down = max_y * vert + (max_y == 1) * h2/2 + (max_y != 1) * h1/2;

    x_pos = pagewidth/2 + margin - (right-left)/2;
    y_pos = pageheight/2 + margin - (down-up)/2;

    x_pos = floor(x_pos * 10 + 0.5) / 10;                               //round to nearest tenth
    y_pos = floor(y_pos * 10 + 0.5) / 10;
    horiz = floor(horiz * 10 + 0.5) / 10;
    vert = floor(vert * 10 + 0.5) / 10;


    /************************************* update gui text boxes **********************************/

    for (int i = 0; i < 5; i++)
        inputs[i]->setText(QString::number(*data[i]));
}


void TaskOrthoViews::compute()
{
    float temp_scale = scale;
    if (autoscale)
        autodims();

    for (int i = 0; i < 4; i++)
    {
        if (i == axo && i > 0)
        {
            if (temp_scale == ui->axoScale->text().toFloat())
            {
                views[i]->setScale(scale);      // only update the axonometric scale if it wasn't manually changed
                ui->axoScale->setText(QString::number(scale));
            }
        }
        else
            views[i]->setScale(scale);

        views[i]->setPos(x_pos + view_status[i][2] * horiz, y_pos + view_status[i][3] * vert);
    }
    Command::updateActive();
    Command::commitCommand();
}


void TaskOrthoViews::validate_cbs()
{
    for (int i=0; i < 5; i++) {
        for (int j=0; j < 5; j++) {
            if ((abs(i-2) + abs(j-2)) < 3) {                            //if i,j combination corresponds to valid check box, then proceed with:

                if (view_count == 0)
                {
                    c_boxes[i][j]->setEnabled(false);
                    c_boxes[i][j]->setChecked(false);
                }
                else if (!c_boxes[i][j]->isChecked()) {                 //only questions boxes 'enableability' if it's not checked
                    if (view_count == 4) {
                        c_boxes[i][j]->setEnabled(false);               //if there are four checked boxes then all others are disabled
                    }
                    else {
                        if ((abs(i-2) + abs(j-2)) == 1) {               //only true for boxes immediately up/down/left/right of centre
                            c_boxes[i][j]->setEnabled(c_boxes[2][2]->isChecked());            //which are enabled if centre box is checked
                        }
                        else {
                            int di = ((i-2) < 0) - ((i-2) > 0);         //which direction is towards centre?
                            int dj = ((j-2) < 0) - ((j-2) > 0);

                            if (c_boxes[i+di][j]->isChecked() + c_boxes[i][j+dj]->isChecked() + (di == 0) + (dj == 0) == 2)
                            {
                                if (!((i == 2)*(j == 2)))               //don't enable the centre one!
/********temporary if statement here, remove the following if to renable 'diagonal' checkboxes *******/
                                    //if ((i-2) * (j-2) == 0)
                                        c_boxes[i][j]->setEnabled(true);    //if this box's inner neighbour(s) are checked, then this one enabled
                            }
                            else
                                c_boxes[i][j]->setEnabled(false);
                        }
                    }
                }
            }
        }
    }
}


void TaskOrthoViews::cb_toggled(bool toggle)
{
    QString name = sender()->objectName().right(2);
    char letter = name.toStdString()[0];
    int dx = letter - '0' - 2;

    letter = name.toStdString()[1];
    int dy = letter - '0' - 2;

    int i = 0;
    if (toggle)
    {
        for (i = 0; i < 4; i++)
        {
            if (view_status[i][0] == 0)
                break;
        }

        int direction, rotation;
        view_status[i][0] = 1;
        view_status[i][2] = dx;
        view_status[i][3] = dy;
        views[i]->activate(true);

        if (abs(dx * dy) == 1)
        {
            axo = i;
            ui->tabWidget->setTabEnabled(1,true);
            ui->axoScale->setText(QString::number(scale));
            set_axo();
        }
        else
        {
            view_data(dx, dy, direction, rotation);
            views[i]->setDir(direction);
            views[i]->setOrientation(rotation);
        }
        view_count += 1;
    }
    else
    {
        if (((abs(dx) == 1 || abs(dy) == 1)) && (dx*dy) == 0)
        {
            c_boxes[dx*2+2][dy*2+2]->setChecked(false);
            if (abs(dx) == 1)
            {
                c_boxes[dx+2][1]->setChecked(false);
                c_boxes[dx+2][3]->setChecked(false);
            }
            else
            {
                c_boxes[1][dy+2]->setChecked(false);
                c_boxes[3][dy+2]->setChecked(false);
            }
        }

        for (i = 0; i < 4; i++)
        {
            if (view_status[i][2] == dx && view_status[i][3] == dy)
                break;
        }

        if (i == axo)
        {
            axo = 0;
            ui->tabWidget->setTabEnabled(1,false);
        }

        views[i]->activate(false);
        view_status[i][0] = 0;
        view_status[i][2] = 0;
        view_status[i][3] = 0;
        view_count -= 1;
    }
    validate_cbs();
    compute();
}


void TaskOrthoViews::view_data(int x, int y, int & direction, int & rotation)
{
    int arr_index;
    rotate_coords(x,y,(4-rotate)%4);
    x = x * proj;
    y = y * proj;

    if (primary < 4)
    {
        if (y == 0)
        {
            rotation = rotate;
            direction = (primary + x + 4) % 4;
        }
        else
        {
            arr_index = (y + 2 - (y > 0)) % 3;              //maps (-2,-1,1,2) to (0,1,2,0)
            direction = map1[primary][arr_index][0];
            rotation = (map1[primary][arr_index][1] + rotate) % 4;
        }
    }
    else
    {
        int offset = (y != 0);
        arr_index = (x == 0)*(y + 2 - (y > 0)) % 3 + (y == 0)*(x + 2 - (x > 0)) % 3;
        direction = map2[2*(primary == 5) + offset][arr_index][0];
        rotation = (map2[2*(primary == 5) + offset][arr_index][1] + rotate) % 4;
    }
}


void TaskOrthoViews::projectionChanged(int index)
{
    proj = 2*(0.5-index);   //gives -1 for 1st angle and 1 for 3rd
    updateSecondaries();
    compute();
}


void TaskOrthoViews::setRotate(int r)
{
    rotate = r;
    views[0]->setOrientation(rotate);
    updateSecondaries();
    compute();
}


void TaskOrthoViews::updateSecondaries()
{
    int direction, rotation;
    int dx, dy;
    int n;

    for (int i = 1; i < 4; i++)
        if ((view_status[i][0] == 1) && (i != axo))
        {
            dx = view_status[i][2];
            dy = view_status[i][3];
            view_data(dx, dy, direction, rotation);
            views[i]->setDir(direction);
            views[i]->setOrientation(rotation);
        }
}


void TaskOrthoViews::setPrimary(int dir)
{
    if (dir == 0)
    {
        for (int i = 0; i < 4; i++)
        {
            views[i]->activate(false);
            view_status[i][0] = 0;
        }
        view_count = 0;
        c_boxes[2][2]->setChecked(false);
    }
    else
    {
        c_boxes[2][2]->setChecked(true);
        view_count += (view_count == 0);
        primary = dir-1;
        view_status[0][0] = 1;
        views[0]->setDir(primary);
        views[0]->setOrientation(rotate);
        views[0]->activate(true);
        updateSecondaries();
        compute();
    }
    validate_cbs();
}


void TaskOrthoViews::hidden(int i)
{
    views[0]->hidden(i);
    views[1]->hidden(i);
    views[2]->hidden(i);
    views[3]->hidden(i);
    Command::updateActive();
    Command::commitCommand();
}


void TaskOrthoViews::smooth(int i)
{
    views[0]->smooth(i);
    views[1]->smooth(i);
    views[2]->smooth(i);
    views[3]->smooth(i);
    Command::updateActive();
    Command::commitCommand();
}


void TaskOrthoViews::axo_flip()
{
    axo_flipped = !axo_flipped;
    set_axo();
}


void TaskOrthoViews::axoTopChanged(int i)
{
    QStringList items;
    items << QString::fromUtf8("Front") << QString::fromUtf8("Right") << QString::fromUtf8("Back") << QString::fromUtf8("Left") << QString::fromUtf8("Top") << QString::fromUtf8("Bottom");

    if (i == 0 || i == 2)
    {
        items.removeAt(0);
        items.removeAt(1);
    }
    else if (i == 1 || i == 3)
    {
        items.removeAt(1);
        items.removeAt(2);
    }
    else
    {
        items.removeAt(4);
        items.removeAt(4);
    }
    ui->axoLeft->clear();
    ui->axoLeft->addItems(items);
    set_axo();
}


void TaskOrthoViews::axoChanged(int i)
{
    if (i == 2)
        ui->flip->setEnabled(true);
    else
        ui->flip->setEnabled(false);

    set_axo();
}


void TaskOrthoViews::axoScale()
{
    bool ok;
    QString temp = ui->axoScale->text();

    float value = temp.toFloat(&ok);
    if (ok)
    {
        views[axo]->setScale(value);
        compute();
    }
    else
        ui->axoScale->setText(temp);
}


void TaskOrthoViews::set_axo()
{
    float v[3];
    float angle;
    int proj, primary, left;

    proj = ui->axoProj->currentIndex();
    primary = ui->axoTop->currentIndex();
    left = ui->axoLeft->currentIndex();

    v[0] = axonometric[proj][primary][left][0];
    v[1] = axonometric[proj][primary][left][1];
    v[2] = axonometric[proj][primary][left][2];
    angle = axonometric[proj][primary][left][3];

    if (axo_flipped && proj == 2)
    {
        int max_i = 2;
        int min_i = 2;
        float abs_v[3] = {abs(v[0]), abs(v[1]), abs(v[2])};

        if (abs_v[0] < abs_v[1] && abs_v[0] < abs_v[2])
            min_i = 0;
        else if (abs_v[1] < abs_v[2])
            min_i = 1;

        if (abs_v[0] > abs_v[1] && abs_v[0] > abs_v[2])
            max_i = 0;
        else if (abs_v[1] > abs_v[2])
            max_i = 1;

        v[min_i] = ((v[min_i] > 0) - (v[min_i] < 0)) * abs_v[max_i];
        v[max_i] = ((v[max_i] > 0) - (v[max_i] < 0)) * abs_v[min_i];

        if (((left == 0 || left == 1) && (primary == 1 || primary == 2)) ||
                ((left == 2 || left == 3) && (primary == 0 || primary == 3)) ||
                ((primary == 5) && (left == 0 || left == 2)) ||
                ((primary == 4) && (left == 1 || left == 3)))
        {
            angle = - angle;
        }
        else
        {
            angle = (angle > 0) ? 98.8 : -81.2;
        }
    }
    views[axo]->setDir(v[0],v[1],v[2], angle, primary);
    compute();
}


void TaskOrthoViews::toggle_auto(int i)
{
    if (i == 2)                                 //auto scale switched on
    {
        autoscale = true;
        ui->label_4->setEnabled(false);
        ui->label_5->setEnabled(false);
        ui->label_6->setEnabled(false);
        for (int j = 0; j < 5; j++)
            inputs[j]->setEnabled(false);       //disable user input boxes
        compute();
    }
    else
    {
        autoscale = false;
        ui->label_4->setEnabled(true);
        ui->label_5->setEnabled(true);
        ui->label_6->setEnabled(true);
        for (int j = 0; j < 5; j++)
            inputs[j]->setEnabled(true);        //enable user input boxes
    }
}


void TaskOrthoViews::data_entered()
{
    //Command::doCommand(Command::Doc,"#1");
    bool ok;

    QString name = sender()->objectName().right(1);
    char letter = name.toStdString()[0];
    int index = letter - '0';


    float value = inputs[index]->text().toFloat(&ok);
    if (ok)
        *data[index] = value;
    else
    {
        inputs[index]->setText(QString::number(*data[index]));
        return;
    }
    compute();
}


bool TaskOrthoViews::user_input()
{
    //if user presses return, this is intercepted by FreeCAD which interprets it as activating the 'OK' button
    //if return was pressed in a text box though, we don't want it to do 'OK', so check to see if a text box has been modified.
    bool modified = false;

    for (int i = 0; i < 5; i++)
    {
        modified = inputs[i]->isModified();         //has input box been modified?
        if (modified)
        {
            inputs[i]->setModified(false);          //reset modified flag
            break;                                  //stop checking
        }
    }
    if (ui->axoScale->isModified())
    {
        ui->axoScale->setModified(false);
        modified = true;
    }
    return modified;
}


void TaskOrthoViews::clean_up(bool keep)
{
    if (keep)           //user ok-ed the drawing
    {
        if (!views[0]->active)
            views[0]->deleteme();
        if (!views[1]->active)
            views[1]->deleteme();
        if (!views[2]->active)
            views[2]->deleteme();
        if (!views[3]->active)
            views[3]->deleteme();
    }
    else                //user cancelled the drawing
    {
        views[0]->deleteme();
        views[1]->deleteme();
        views[2]->deleteme();
        views[3]->deleteme();
    }
}






//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgOrthoViews::TaskDlgOrthoViews()
    : TaskDialog()
{
    widget = new TaskOrthoViews();
    taskbox = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("actions/drawing-orthoviews"), widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgOrthoViews::~TaskDlgOrthoViews()
{
}

//==== calls from the TaskView ===============================================================


void TaskDlgOrthoViews::open()
{
}

void TaskDlgOrthoViews::clicked(int)
{
}

bool TaskDlgOrthoViews::accept()
{
    bool check = widget->user_input();
    if (!check)
        widget->clean_up(true);

    return !check;
}

bool TaskDlgOrthoViews::reject()
{
    widget->clean_up(false);
    return true;
}




#include "moc_TaskOrthoViews.cpp"
