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
#include <sstream>

using namespace Gui;
using namespace DrawingGui;
using namespace std;


int name_to_number(const QString& nme)
{
    char * temp[] = {"","Front","Back","Right","Left","Top","Bottom"};
    for (int j=0; j < 7; j++)
    {
        if (QObject::tr(temp[j]) == nme)
            return j;
    }
    return 0;
}

QString number_to_name(int j)
{
    char * temp[] = {"","Front","Back","Right","Left","Top","Bottom"};
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
    angle = 0;

    Command::doCommand(Command::Doc,"App.activeDocument().addObject('Drawing::FeatureViewPart','%s')",myname.c_str());
    Command::doCommand(Command::Doc,"App.activeDocument().%s.Source = App.activeDocument().%s",myname.c_str(), sourcepart);
    Command::doCommand(Command::Doc,"App.activeDocument().%s.addObject(App.activeDocument().%s)",targetpage,myname.c_str());

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
    dir = i;
    int vx = (dir == 3) - (dir == 4);
    int vy = (dir == 1) - (dir == 2);
    int vz = (dir == 5) - (dir == 6);

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
        Command::doCommand(Command::Doc,"App.activeDocument().%s.Rotation = %d", myname.c_str(), (90*orientation+angle));
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

    float coords[7][2] =
    {
        {0, 0},
        {-cx, cz},      //front
        { cx, cz},       //back
        { cy, cz},       //right
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

    float dims[4][2] =
    {
        { 0,  0},      //zero height/width for no direction
        {dx, dz},      //front & back
        {dy, dz},      //right & left
        {dx, dy}       //top & bottom
    };

    width = dims[(dir+1)/2][0];
    height = dims[(dir+1)/2][1];
    if (orientation % 2 == 1)
    {
        float temp = width;
        width = height;
        height = temp;
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

    primary = 0;
    secondary_1 = 0;
    secondary_2 = 0;
    rotate = 0;
    proj = 1;
    autoscale = 1;

    margin = 10;
    pagesize(template_name);
    min_space = 15;

    //below are calculated in case autodims is deselected before these values are initialised.
    float max_dim = max(max(bbox.LengthX(), bbox.LengthY()), bbox.LengthZ());
    scale = min(pagewidth, pageh2)/(3*max_dim+4*min_space);
    spacing_1 = scale*max_dim + min_space;
    spacing_2 = spacing_1;
    x_pos = pagewidth/2;
    y_pos = pageh1/2;


    connect(ui->projection, SIGNAL(currentIndexChanged(int)), this, SLOT(projectionChanged(int)));
    connect(ui->rotate, SIGNAL(currentIndexChanged(int)), this, SLOT(setRotate(int)));
    connect(ui->smooth, SIGNAL(stateChanged(int)), this, SLOT(smooth(int)));
    connect(ui->hidden, SIGNAL(stateChanged(int)), this, SLOT(hidden(int)));
    connect(ui->auto_tog, SIGNAL(stateChanged(int)), this, SLOT(toggle_auto(int)));
    connect(ui->spacing1, SIGNAL(editingFinished()), this, SLOT(data_entered()));
    connect(ui->spacing2, SIGNAL(editingFinished()), this, SLOT(data_entered()));
    connect(ui->x, SIGNAL(editingFinished()), this, SLOT(data_entered()));
    connect(ui->y, SIGNAL(editingFinished()), this, SLOT(data_entered()));
    connect(ui->scale, SIGNAL(editingFinished()), this, SLOT(data_entered()));
    connect(ui->primary, SIGNAL(activated(int)), this, SLOT(setPrimary(int)));
    connect(ui->secondary_1, SIGNAL(activated(int)), this, SLOT(setSecondary_1(int)));
    connect(ui->secondary_2, SIGNAL(activated(int)), this, SLOT(setSecondary_2(int)));
    
//this matrix contains the relative positions of specified views in third angle projection.
//horizontal coord specifies the primary view, vertical coord specifies a secondary view
//but to retrieve, get transfrom[secondary][primary]
//then matrix contents {x,y,r} specifies directional x and y of secondary view relative to primary view (with y increasing downwards as per SVG
//r specifies rotation of view from default, eg 1 = 90 degrees clockwise.
//
//first angle can be obtained by taking -ve of x and y coords, no change of r is required.

    // primary view direction ->       1           2           3           4           5           6
    int temp[7][7][3] ={{{0,0,0},   {0,0,0},    {0,0,0},    {0,0,0},    {0,0,0},    {0,0,0},    {0,0,0}},  
    /*secondary 1 */    {{0,0,0},   {0,0,0},    {0,0,0},    {-1,0,0},   {1,0,0},    {0,1,0},    {0,-1,0}},
    /*view      2 */    {{0,0,0},   {0,0,0},    {0,0,0},    {1,0,0},    {-1,0,0},   {0,-1,2},   {0,1,2}},
    /*direction 3 */    {{0,0,0},   {1,0,0},    {-1,0,0},   {0,0,0},    {0,0,0},    {1,0,3},    {1,0,1}},
    /*          4 */    {{0,0,0},   {-1,0,0},   {1,0,0},    {0,0,0},    {0,0,0},    {-1,0,1},   {-1,0,3}},
    /*          5 */    {{0,0,0},   {0,-1,0},   {0,-1,2},   {0,-1,1},   {0,-1,3},   {0,0,0},    {0,0,0}},
    /*          6 */    {{0,0,0},   {0,1,0},    {0,1,2},    {0,1,3},    {0,1,1},    {0,0,0},    {0,0,0}}};
    
    for (int i=0; i < 7; i++)
        for (int j=0; j < 7; j++)
            for (int k=0; k < 3; k++)                   //initialisation needs to be done this way (rather than initialise transform directly
                transform[i][j][k] = temp[i][j][k];     //in order to avoid compiler warning

//    Command::doCommand(Command::Doc,"#%d", transform[6][3][2]);
} //end of constructor

TaskOrthoViews::~TaskOrthoViews()
{
    delete views[0];
    delete views[1];
    delete views[2];
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
    /********update num_templates when adding extra templates*******************/
    
    const int num_templates = 2;
    std::string templates[num_templates] = {"A3_Landscape.svg", "A4_Landscape.svg"};
    int dimensions[num_templates][3] = {{420,297,227},{297,210,153}};       //{width, full height, reduced height}  measured from page edge.
    
    for (int i=0; i < num_templates; i++)
    {      
        if (templates[i] == page_template)
        {
            pagewidth = dimensions[i][0] - 2*margin;
            pageh1 = dimensions[i][1] - 2*margin;
            pageh2 = dimensions[i][2] - ((dimensions[i][1] == dimensions[i][2]) + 1) * margin;
            return;
        }
    }
    
    //matching template not found, read through template file for width & height info.
    
    //code below copied from FeaturePage.cpp
    Base::FileInfo fi(page_template);
    if (!fi.isReadable()) {
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
    string line;
    string temp_line;
    ifstream file (fi.filePath().c_str());
    size_t found;
    bool done = false;
    
    try
    {
        while (!file.eof())
        {
            getline (file,line);
            found = line.find("width=");
            if (found != string::npos)
            {
                temp_line = line.substr(10);
                found = temp_line.find("\"");
                temp_line = temp_line.substr(0,found);
                stringstream num_str(temp_line);
                num_str >> pagewidth;
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
            if (found != string::npos)
            {
                temp_line = line.substr(11);
                found = temp_line.find("\"");
                temp_line = temp_line.substr(0,found);
                stringstream num_str_2(temp_line);
                num_str_2 >> pageh1;
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
            
            if (line.find("metadata") != string::npos)      //give up if we meet a metadata tag
                break;
        }
    }
    catch (Standard_Failure) { }
    
    file.close();        
    
    //width/height not found??  fallback to A4 landscape simple:
    pagewidth = 277;
    pageh1 = 190;
    pageh2 = 190;
}


void TaskOrthoViews::autodims(float s1_x, float s1_y, float s2_x, float s2_y)
{
    int num_wide = abs(s1_x) + abs(s2_x) + 2;       //tells us how many 'spaces' there are in each direction
    int num_high = abs(s1_y) + abs(s2_y) + 2;       //eg if s1_x = 1, then 3 spaces, page_edge -> primary -> secondary -> page_edge

    //now calculate real (non-scaled) width/height of views when combined togethor
    float width = views[0]->width;                  //dimensions of primary view
    float height = views[0]->height;        
    width += (s1_x != 0) * views[1]->width;         //only add width if secondary view is alongside primary  (ie s1_x <> 0)
    height += (s1_y != 0) * views[1]->height;       //equivalently for height
    width += (s2_x != 0) * views[2]->width;
    height += (s2_y != 0) * views[2]->height;

    int pageheight;                                 //allow extra page height if view arrangement avoids the information box on the bottom right
    if (((s1_x + s1_y + s2_x + s2_y) == 2) && (views[0]->width <= width/2))
        pageheight = pageh1;
    else
        pageheight = pageh2;

    //now evaluate available space / space required for each direction, and choose the smaller.
    float working_scale = min((pagewidth - num_wide * min_space) / width, (pageheight - num_high * min_space) / height);

    //that gives the largest scale for which the min_space requirements can be met, but we want a 'sensible' scale, rather than 0.28457239!
    //eg if working_scale = 0.115, then we want to use 0.1
    //   if working_scale = 7.65, then we want to use 5
    //   if working_scale = 76.5, then we want to use 50
    float exponent = floor(log10(working_scale));       //if working_scale = a * 10^b, what is b?
    working_scale *= pow(10, -exponent);                //now find what 'a' is.

    float valid_scales[2][8] = {{1, 1.25, 2, 2.5, 3.75, 5, 7.5, 10},  //equate to 1:10, 1:8, 1:5, 1:4, 3:8, 1:2, 3:4, 1:1
                                {1, 1.5, 2, 3, 4, 5, 8, 10}};      //equate to 1:1, 3:2, 2:1, 3:1, 4:1, 5:1, 8:1, 10:1
    int i = 0;
    while (valid_scales[(exponent>=0)][i] <= working_scale)     //choose closest value smaller than 'a' from list.
        i += 1;                                                 //choosing top list if exponent -ve, bottom list for +ve exponent
    i -= 1;
    float chosen_scale = valid_scales[(exponent>=0)][i];
    scale = chosen_scale * pow(10, exponent);        //now have the appropriate scale, reapply the *10^b

    ///////////////////////
    //now we move on to the placements

    width *= scale;     
    height *= scale;
    
    //which direction gives the smallest space between views?  Space is given by (page size - size of views) / (number of spaces)
    float space = min((pagewidth - width)/num_wide, (pageheight - height)/num_high);
    
    //now calculate the spacing of the secondary views from the primary one
    //***********************spacing is currently view centre -> view centre********************************
    //so the spacing is equal to (primary centre -> prmry edge) + (secondary centre -> scdry edge) + space
    //these depend upon whether secondary is vertically or horizontally primary from primary
    
    if (s1_x != 0)                              //secondary_1 is horizontally placed from primary
        spacing_1 = space + scale*(views[0]->width + views[1]->width)/2;
    else                                        //secondary_1 is vertically placed from primary
        spacing_1 = space + scale*(views[0]->height + views[1]->height)/2;
        

    if (s2_x != 0)                              //same for secondary_2
        spacing_2 = space + scale*(views[0]->width + views[2]->width)/2;
    else
        spacing_2 = space + scale*(views[0]->height + views[2]->height)/2;
    
    spacing_1 = floor(spacing_1*10 + 0.5) / 10;
    spacing_2 = floor(spacing_2*10 + 0.5) / 10;
    
    ///////////////////////////////////////////
    //finally, evaluate position of primary view.
    
    int pos_wide = s1_x + s2_x;                 //=1 = primary on left, 0 = primary in middle, -1 = on right 
    int pos_high = s1_y + s2_y;                 //=1 = at top, 0 = in middle, -1 = at bottom
    
    if (pos_wide == 1)
        x_pos = (pagewidth / 2.0 + margin) - ((width + space) - scale * views[0]->width)/2;
    else if (pos_wide == 0)
        x_pos = pagewidth / 2.0 + margin;
    else 
        x_pos = (pagewidth / 2.0 + margin) + ((width + space) - scale * views[0]->width)/2;

    if (pos_high == 1)
        y_pos = (pageheight / 2.0 + margin) - ((height + space) - scale * views[0]->height)/2;
    else if (pos_high == 0)
        y_pos = pageheight / 2.0 + margin;
    else
        y_pos = (pageheight / 2.0 + margin) + ((height + space) - scale * views[0]->height)/2;
    
    x_pos = floor(x_pos * 10 + 0.5) / 10;
    y_pos = floor(y_pos * 10 + 0.5) / 10;
    
    ////////////
    //update the gui boxes with calculated information
    ui->scale->setText(QString::number(scale));
    ui->x->setText(QString::number(x_pos));
    ui->y->setText(QString::number(y_pos));
    ui->spacing1->setText(QString::number(spacing_1));
    ui->spacing2->setText(QString::number(spacing_2));
}

void TaskOrthoViews::compute()
{
//secondary 1
    float s1_x = proj * transform[secondary_1][primary][0];
    float s1_y = proj * transform[secondary_1][primary][1];
    rotate_coords(s1_x,s1_y,rotate);
    float s1_r = (rotate + transform[secondary_1][primary][2])%4;    

//secondary 2
    float s2_x = proj * transform[secondary_2][primary][0];
    float s2_y = proj * transform[secondary_2][primary][1];
    rotate_coords(s2_x,s2_y,rotate);
    float s2_r = (rotate + transform[secondary_2][primary][2])%4;    

    //autodims will retrieve information from views[] regarding image dimensions
    //thus need to set views[] direction and orientation before calling autodims.
    views[0]->setDir(primary);
    views[0]->setOrientation(rotate);

    views[1]->setDir(secondary_1);
    views[1]->setOrientation(s1_r);

    views[2]->setDir(secondary_2);
    views[2]->setOrientation(s2_r);

    if (autoscale)
        autodims(s1_x, s1_y, s2_x, s2_y);

    views[0]->setScale(scale);
    views[0]->setPos(x_pos,y_pos);

    views[1]->setScale(scale);
    views[1]->setPos(x_pos + spacing_1 * s1_x, y_pos + spacing_1 * s1_y);
    
    views[2]->setScale(scale);
    views[2]->setPos(x_pos + spacing_2 * s2_x, y_pos + spacing_2 * s2_y);

    Command::updateActive();
    Command::commitCommand();
}
   
void TaskOrthoViews::projectionChanged(int index)
{
    proj = 2*(0.5-index);   //gives -1 for 1st angle and 1 for 3rd
    compute();
}

void TaskOrthoViews::setRotate(int r)
{
    rotate = r;
    compute();
}

void TaskOrthoViews::populate_s1()
{
    ui->secondary_1->clear();
    ui->secondary_1->addItem(QString());
    
    int i = 0;
    int k = 0;
    for (int j=1; j < 7; j++)
        if (((j+1)/2 != (primary+1)/2) && (j != secondary_2))
        {
            k += 1;
            ui->secondary_1->addItem(number_to_name(j));
            if (j == secondary_1)
                i = k;
        }

    ui->secondary_1->setCurrentIndex(i);
}

void TaskOrthoViews::populate_s2()
{
    ui->secondary_2->clear();
    ui->secondary_2->addItem(QString());

    int i = 0;
    int k = 0;
    for (int j=1; j < 7; j++)
        if (((j+1)/2 != (primary+1)/2) && (j != secondary_1))
        {
            k += 1;
            ui->secondary_2->addItem(number_to_name(j));
            if (j == secondary_2)
                i = k;
        }

    ui->secondary_2->setCurrentIndex(i);
}

void TaskOrthoViews::setPrimary(int dir)
{
    if (dir == 0)
    {
        views[0]->activate(false);
        views[1]->activate(false);
        views[2]->activate(false);
        primary = 0;
        secondary_1 = 0;
        secondary_2 = 0;
        ui->secondary_1->setCurrentIndex(0);
        ui->secondary_2->setCurrentIndex(0);
    }
    else
    {
        if (!views[0]->active)
            views[0]->activate(true);
            
        primary = dir;

        if ((primary+1)/2 == (secondary_1+1)/2)         //eg primary = 1, secondary_1 = 2
        {
            views[1]->activate(false);
            secondary_1 = 0;
            ui->secondary_1->setCurrentIndex(0);
        }

        if ((primary+1)/2 == (secondary_2+1)/2)
        {
            views[2]->activate(false);
            secondary_2 = 0;
            ui->secondary_2->setCurrentIndex(0);
        }
        populate_s1();
        populate_s2();
        compute();
    }
}

void TaskOrthoViews::setSecondary_1(int dir)
{
    if (dir == 0)
    {
        views[1]->activate(false);
        secondary_1 = 0;
    }
    else
    {
        QString text = ui->secondary_1->currentText();
        int value = name_to_number(text);
        if (!views[1]->active)
            views[1]->activate(true);
        secondary_1 = value;
    }
    populate_s2();
    compute();
}

void TaskOrthoViews::setSecondary_2(int dir)
{
    if (dir == 0)
    {
        views[2]->activate(false);
        secondary_2 = 0;
    }
    else
    {
        QString text = ui->secondary_2->currentText();
        int value = name_to_number(text);
        if (!views[2]->active)
            views[2]->activate(true);
        secondary_2 = value;
    }
    populate_s1();
    compute();
}

void TaskOrthoViews::hidden(int i)
{
    views[0]->hidden(i);
    views[1]->hidden(i);
    views[2]->hidden(i);
    Command::updateActive();
    Command::commitCommand();
}

void TaskOrthoViews::smooth(int i)
{
    views[0]->smooth(i);
    views[1]->smooth(i);
    views[2]->smooth(i);
    Command::updateActive();
    Command::commitCommand();
}

void TaskOrthoViews::toggle_auto(int i)
{
    if (i == 2)                     //auto scale switched on
    {
        autoscale = true;
        ui->scale->setEnabled(false);
        ui->x->setEnabled(false);
        ui->y->setEnabled(false);
        ui->spacing1->setEnabled(false);
        ui->spacing2->setEnabled(false);
        ui->label_4->setEnabled(false);
        ui->label_5->setEnabled(false);
        ui->label_6->setEnabled(false);
        compute();
    }
    else
    {
        autoscale = false;
        ui->scale->setEnabled(true);
        ui->x->setEnabled(true);
        ui->y->setEnabled(true);
        ui->spacing1->setEnabled(true);
        ui->spacing2->setEnabled(true);        
        ui->label_4->setEnabled(true);        
        ui->label_5->setEnabled(true);        
        ui->label_6->setEnabled(true);        
    }
}

void TaskOrthoViews::data_entered()
{
    bool ok;

    float value = ui->spacing1->text().toFloat(&ok);    
    if (ok)
       spacing_1 = value;
    else
    {
        ui->spacing1->setText(QString::number(spacing_1));
        return;
    }
    
    value = ui->spacing2->text().toFloat(&ok);
    if (ok)
       spacing_2 = value;
    else
    {
        ui->spacing2->setText(QString::number(spacing_2));
        return;
    }

    value = ui->x->text().toFloat(&ok);
    if (ok)
        x_pos = value;
    else
    {
        ui->x->setText(QString::number(x_pos));
        return;
    }

    value = ui->y->text().toFloat(&ok);
    if (ok)
        y_pos = value;
    else
    {
        ui->y->setText(QString::number(y_pos));
        return;
    }

    value = ui->scale->text().toFloat(&ok);
    if (ok)
        scale = value;
    else
    {
        ui->scale->setText(QString::number(scale));
        return;
    }
    compute();
}

bool TaskOrthoViews::user_input()
{
    //if user presses return, this is intercepted by FreeCAD which interprets it as activating the 'OK' button
    //if return was pressed in a text box though, we don't want it to do 'OK', so check to see if a text box has been modified.
    bool modified = (ui->spacing1->isModified() || ui->spacing2->isModified() || ui->x->isModified() || ui->y->isModified() || ui->scale->isModified());

    if (modified)
    {
        ui->spacing1->setModified(false);
        ui->spacing2->setModified(false);
        ui->x->setModified(false);
        ui->y->setModified(false);
        ui->scale->setModified(false);
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
    }
    else                //user cancelled the drawing
    {
        views[0]->deleteme();
        views[1]->deleteme();
        views[2]->deleteme();        
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
