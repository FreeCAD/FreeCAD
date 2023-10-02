/***************************************************************************
 *   Copyright (c) 2014 Joe Dowsett <dowsettjoe[at]yahoo[dot]co[dot]uk>    *
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
#include <QLineEdit>
#include <QMenu>
#endif

#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Control.h>
#include <Mod/Drawing/App/FeaturePage.h>
#include <Mod/Part/App/PartFeature.h>

#include "TaskOrthoViews.h"
#include "ui_TaskOrthoViews.h"


using namespace Gui;
using namespace DrawingGui;
using namespace std;
namespace bp = boost::placeholders;

#if 0  // needed for Qt's lupdate utility
    qApp->translate("QObject", "Make axonometric...");
    qApp->translate("QObject", "Edit axonometric settings...");
    qApp->translate("QObject", "Make orthographic");
#endif


void pagesize(string& page_template, int dims[4], int block[4])
{
    dims[0] = 10;  // default to A4_Landscape with 10mm margins
    dims[1] = 10;
    dims[2] = 287;
    dims[3] = 200;

    block[0] = block[1] = 0;  // default to no title block
    block[2] = block[3] = 0;

    int t0, t1, t2, t3 = 0;

    // code below copied from FeaturePage.cpp
    Base::FileInfo fi(page_template);
    if (!fi.isReadable()) {
        fi.setFile(App::Application::getResourceDir() + "Mod/Drawing/Templates/" + fi.fileName());
        if (!fi.isReadable()) {  // if so then really shouldn't have been able to get this far, but
                                 // just in case...
            return;
        }
    }

    // open Template file
    string line;
    ifstream file(fi.filePath().c_str());

    try {
        while (getline(file, line)) {
            if (line.find("<!-- Working space") != string::npos) {
                (void)sscanf(line.c_str(),
                             "%*s %*s %*s %d %d %d %d",
                             &dims[0],
                             &dims[1],
                             &dims[2],
                             &dims[3]);  // eg "    <!-- Working space 10 10 410 287 -->"
                getline(file, line);

                if (line.find("<!-- Title block") != string::npos) {
                    (void)sscanf(line.c_str(),
                                 "%*s %*s %*s %d %d %d %d",
                                 &t0,
                                 &t1,
                                 &t2,
                                 &t3);  // eg "    <!-- Working space 10 10 410 287 -->"
                }

                break;
            }

            if (line.find("metadata") != string::npos) {  // give up if we meet a metadata tag
                break;
            }
        }
    }
    catch (Standard_Failure&) {
    }


    if (t3 != 0) {
        block[2] = t2 - t0;  // block width
        block[3] = t3 - t1;  // block height

        if (t0 <= dims[0]) {  // title block on left
            block[0] = -1;
        }
        else if (t2 >= dims[2]) {  // title block on right
            block[0] = 1;
        }

        if (t1 <= dims[1]) {  // title block at top
            block[1] = 1;
        }
        else if (t3 >= dims[3]) {  // title block at bottom
            block[1] = -1;
        }
    }

    dims[2] -= dims[0];  // width
    dims[3] -= dims[1];  // height
}


///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

orthoview::orthoview(App::Document* parent,
                     App::DocumentObject* part,
                     App::DocumentObject* page,
                     Base::BoundBox3d* partbox)
{
    parent_doc = parent;
    myname = parent_doc->getUniqueObjectName("Ortho");

    x = 0;
    y = 0;
    cx = partbox->GetCenter().x;
    cy = partbox->GetCenter().y;
    cz = partbox->GetCenter().z;

    this_view = static_cast<Drawing::FeatureViewPart*>(
        parent_doc->addObject("Drawing::FeatureViewPart", myname.c_str()));
    static_cast<App::DocumentObjectGroup*>(page)->addObject(this_view);
    this_view->Source.setValue(part);

    pageX = 0;
    pageY = 0;
    scale = 1;

    rel_x = 0;
    rel_y = 0;
    ortho = true;
    auto_scale = true;

    away = false;
    tri = false;
    axo = 0;
}

orthoview::~orthoview()
{}

void orthoview::set_data(int r_x, int r_y)
{
    rel_x = r_x;
    rel_y = r_y;

    char label[15];
    sprintf(label, "Ortho_%i_%i", rel_x, rel_y);  // label name for view, based on relative position

    this_view->Label.setValue(label);
    ortho = ((rel_x * rel_y) == 0);
}

void orthoview::deleteme()
{
    parent_doc->removeObject(myname.c_str());
}

void orthoview::setPos(float px, float py)
{
    if (px != 0 && py != 0) {
        pageX = px;
        pageY = py;
    }

    float ox = pageX - scale * x;
    float oy = pageY + scale * y;

    this_view->X.setValue(ox);
    this_view->Y.setValue(oy);
}

void orthoview::setScale(float newScale)
{
    scale = newScale;
    this_view->Scale.setValue(scale);
}

float orthoview::getScale()
{
    return scale;
}

void orthoview::calcCentre()
{
    x = X_dir.X() * cx + X_dir.Y() * cy + X_dir.Z() * cz;
    y = Y_dir.X() * cx + Y_dir.Y() * cy + Y_dir.Z() * cz;
}

void orthoview::hidden(bool state)
{
    this_view->ShowHiddenLines.setValue(state);
}

void orthoview::smooth(bool state)
{
    this_view->ShowSmoothLines.setValue(state);
}

void orthoview::set_projection(const gp_Ax2& cs)
{
    gp_Ax2 actual_cs;
    gp_Dir actual_X;

    // coord system & directions for desired projection
    X_dir = cs.XDirection();
    Y_dir = cs.YDirection();
    Z_dir = cs.Direction();

    // coord system of created view - same code as used in projection algos
    // actual_cs = gp_Ax2(gp_Pnt(0,0,0), gp_Dir(Z_dir.X(),Z_dir.Y(),Z_dir.Z()));

    // but as the file gets saved the projection direction gets rounded.
    // this can lead to choosing a different normal x-direction when the file
    // gets reloaded see issue #1909
    // we anticipate the actual_cs after reloading by rounding the Z_dir now
    const double x = round(Z_dir.X() * 1e12) / 1e12;
    const double y = round(Z_dir.Y() * 1e12) / 1e12;
    const double z = round(Z_dir.Z() * 1e12) / 1e12;
    actual_cs = gp_Ax2(gp_Pnt(0, 0, 0), gp_Dir(x, y, z));

    actual_X = actual_cs.XDirection();

    // angle between desired projection and actual projection
    float rotation = X_dir.Angle(actual_X);

    if (rotation != 0 && abs(M_PI - rotation) > 0.05) {
        if (!Z_dir.IsEqual(actual_X.Crossed(X_dir), 0.05)) {
            rotation = -rotation;
        }
    }

    calcCentre();

    // this_view->Direction.setValue(Z_dir.X(), Z_dir.Y(), Z_dir.Z());
    this_view->Direction.setValue(x, y, z);
    this_view->Rotation.setValue(180 * rotation / M_PI);
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

OrthoViews::OrthoViews(App::Document* doc, const char* pagename, const char* partname)
{
    horiz = nullptr;
    vert = nullptr;

    parent_doc = doc;
    parent_doc->openTransaction("Create view");

    part = parent_doc->getObject(partname);
    bbox.Add(static_cast<Part::Feature*>(part)->Shape.getBoundingBox());

    page = parent_doc->getObject(pagename);
    Gui::Application::Instance->showViewProvider(page);
    load_page();

    min_space = 15;  // should be preferenced

    min_r_x = max_r_x = 0;
    min_r_y = max_r_y = 0;

    rotate_coeff = 1;
    smooth = false;
    hidden = false;
    autodims = true;

    width = height = depth = 0;
    layout_width = layout_height = 0;
    gap_x = gap_y = 0;
    offset_x = offset_y = 0;
    scale = 0;
    num_gaps_x = num_gaps_y = 0;

    this->connectDocumentDeletedObject =
        doc->signalDeletedObject.connect(boost::bind(&OrthoViews::slotDeletedObject, this, bp::_1));
    this->connectApplicationDeletedDocument = App::GetApplication().signalDeleteDocument.connect(
        boost::bind(&OrthoViews::slotDeletedDocument, this, bp::_1));
}

OrthoViews::~OrthoViews()
{
    for (int i = views.size() - 1; i >= 0; i--) {
        delete views[i];
    }

    try {
        page->recomputeFeature();
    }
    catch (...) {
    }
}

void OrthoViews::slotDeletedDocument(const App::Document& Obj)
{
    if (parent_doc == &Obj) {
        Gui::Control().closeDialog();
    }
}

void OrthoViews::slotDeletedObject(const App::DocumentObject& Obj)
{
    if (page == &Obj || part == &Obj) {
        Gui::Control().closeDialog();
    }
    else {
        for (std::vector<orthoview*>::iterator it = views.begin(); it != views.end(); ++it) {
            if ((*it)->getViewPart() == &Obj) {
                views.erase(it);
                break;
            }
        }
    }
}

void OrthoViews::load_page()
{
    string template_name = static_cast<Drawing::FeaturePage*>(page)->Template.getValue();
    pagesize(template_name, large, block);
    page_dims = large;

    // process page dims for title block data
    if (block[0] != 0) {
        title = true;

        // max vertical space avoiding title block
        small_v[1] = large[1];             // y margin same as large page
        small_v[3] = large[3];             // y size same as large page
        small_v[2] = large[2] - block[2];  // x width same as large width - block width
        if (block[0] == -1) {
            small_v[0] = large[0] + block[2];  // x margin same as large + block width
            horiz = &min_r_x;
        }
        else {
            small_v[0] = large[0];  // x margin same as large
            horiz = &max_r_x;
        }

        // max horizontal space avoiding title block
        small_h[0] = large[0];
        small_h[2] = large[2];
        small_h[3] = large[3] - block[3];
        if (block[1] == 1) {
            small_h[1] = large[1] + block[3];
            vert = &max_r_y;
        }
        else {
            small_h[1] = large[1];
            vert = &min_r_y;
        }
    }
    else {
        title = false;
    }
}

void OrthoViews::calc_layout_size()  // calculate the real world size of given view layout, assuming
                                     // no space
{
    // note that views in relative positions x = -4, -2, 0 , 2 etc etc
    // have width = orientated part width
    // while those in relative positions x = -3, -1, 1 etc
    // have width = orientated part depth

    // similarly in y positions, height = part height or depth

    layout_width = (1 + floor(max_r_x / 2.0) + floor(-min_r_x / 2.0)) * width;
    layout_width += (ceil(max_r_x / 2.0) + ceil(-min_r_x / 2.0)) * depth;
    layout_height = (1 + floor(max_r_y / 2.0) + floor(-min_r_y / 2.0)) * height;
    layout_height += (ceil(max_r_y / 2.0) + ceil(-min_r_y / 2.0)) * depth;
}

void OrthoViews::choose_page()  // chooses which bit of page space to use depending upon layout &
                                // titleblock
{
    int h = abs(*horiz);  // how many views in direction of title block  (horiz points to min_r_x or
                          // max_r_x)
    int v = abs(*vert);
    float layout_corner_width = (1 + floor(h / 2.0)) * width
        + ceil(h / 2.0) * depth;  // from (0, 0) view inclusively, how wide and tall is the layout
                                  // in the direction of the title block
    float layout_corner_height = (1 + floor(v / 2.0)) * height + ceil(v / 2.0) * depth;
    float rel_space_x = layout_corner_width / layout_width
        - 1.0 * block[2] / large[2];  // relative to respective sizes, how much space between (0, 0)
                                      // and title block,
    float rel_space_y = layout_corner_height / layout_height
        - 1.0 * block[3] / large[3];  //                      can be -ve if block extends into /
                                      //                      beyond (0, 0) view
    float view_x, view_y, v_x_r, v_y_r;
    bool interferes = false;
    float a, b;

    for (int i = min_r_x; i <= max_r_x; i++) {
        for (int j = min_r_y; j <= max_r_y; j++) {
            if (index(i, j) != -1)  // is there a view in this position?
            {
                a = i * block[0]
                    * 0.5;  // reflect i and j so as +ve is in direction of title block ##
                b = j * block[1] * 0.5;
                view_x = ceil(a + 0.5) * width
                    + ceil(a) * depth;  // extreme coords of view in direction of block, measured
                                        // from far corner of (0, 0) view,
                view_y = ceil(b + 0.5) * height
                    + ceil(b) * depth;  //                      can be -ve if view is on opposite
                                        //                      side of (0, 0) from title block
                v_x_r = view_x / layout_width;  // make relative
                v_y_r = view_y / layout_height;
                if (v_x_r > rel_space_x
                    && v_y_r > rel_space_y) {  // ## so that can use > in this condition regardless
                                               // of position of block
                    interferes = true;
                }
            }
        }
    }

    if (!interferes) {
        page_dims = large;
    }
    else {
        if (min(small_h[2] / layout_width, small_h[3] / layout_height)
            > min(small_v[2] / layout_width, small_v[3] / layout_height)) {
            page_dims = small_h;
        }
        else {
            page_dims = small_v;
        }
    }
}

void OrthoViews::calc_scale()  // compute scale required to meet minimum space requirements
{
    float scale_x, scale_y, working_scale;

    scale_x = (page_dims[2] - num_gaps_x * min_space) / layout_width;
    scale_y = (page_dims[3] - num_gaps_y * min_space) / layout_height;

    working_scale = min(scale_x, scale_y);

    // which gives the largest scale for which the min_space requirements can be met, but we want a
    // 'sensible' scale, rather than 0.28457239... eg if working_scale = 0.115, then we want to use
    // 0.1, similarly 7.65 -> 5, and 76.5 -> 50

    float exponent = floor(log10(working_scale));  // if working_scale = a * 10^b, what is b?
    working_scale *= pow(10, -exponent);           // now find what 'a' is.

    float valid_scales[2][8] = {
        {1, 1.25, 2, 2.5, 3.75, 5, 7.5, 10},  // equate to 1:10, 1:8, 1:5, 1:4, 3:8, 1:2, 3:4, 1:1
        {1, 1.5, 2, 3, 4, 5, 8, 10}};         // equate to 1:1, 3:2, 2:1, 3:1, 4:1, 5:1, 8:1, 10:1

    int i = 7;
    while (valid_scales[(exponent >= 0)][i]
           > working_scale) {  // choose closest value smaller than 'a' from list.
        i -= 1;                // choosing top list if exponent -ve, bottom list for +ve exponent
    }

    scale = valid_scales[(exponent >= 0)][i]
        * pow(10, exponent);  // now have the appropriate scale, reapply the *10^b
}

void OrthoViews::calc_offsets()  // calcs SVG coords for centre of upper left view
{
    // space_x is the empty clear white space between views
    // gap_x is the centre - centre distance between views

    float space_x = (page_dims[2] - scale * layout_width) / num_gaps_x;
    float space_y = (page_dims[3] - scale * layout_height) / num_gaps_y;

    gap_x = space_x + scale * (width + depth) * 0.5;
    gap_y = space_y + scale * (height + depth) * 0.5;

    if (min_r_x % 2 == 0) {
        offset_x = page_dims[0] + space_x + 0.5 * scale * width;
    }
    else {
        offset_x = page_dims[0] + space_x + 0.5 * scale * depth;
    }

    if (max_r_y % 2 == 0) {
        offset_y = page_dims[1] + space_y + 0.5 * scale * height;
    }
    else {
        offset_y = page_dims[1] + space_y + 0.5 * scale * depth;
    }
}

void OrthoViews::set_views()  // process all views - scale & positions
{
    float x;
    float y;

    for (unsigned int i = 0; i < views.size(); i++) {
        x = offset_x + (views[i]->rel_x - min_r_x) * gap_x;
        y = offset_y + (max_r_y - views[i]->rel_y) * gap_y;

        if (views[i]->auto_scale) {
            views[i]->setScale(scale);
        }

        views[i]->setPos(x, y);
    }
}

void OrthoViews::process_views()  // update scale and positions of views
{
    if (autodims) {
        calc_layout_size();

        if (title) {
            choose_page();
        }

        calc_scale();
        calc_offsets();
    }

    set_views();
    parent_doc->recompute();
}

void OrthoViews::set_hidden(bool state)
{
    hidden = state;

    for (unsigned int i = 0; i < views.size(); i++) {
        views[i]->hidden(hidden);
    }

    parent_doc->recompute();
}

void OrthoViews::set_smooth(bool state)
{
    smooth = state;

    for (unsigned int i = 0; i < views.size(); i++) {
        views[i]->smooth(smooth);
    }

    parent_doc->recompute();
}

void OrthoViews::set_primary(gp_Dir facing,
                             gp_Dir right)  // set the orientation of the primary view
{
    primary.SetDirection(facing);
    primary.SetXDirection(right);
    gp_Dir up = primary.YDirection();

    // compute dimensions of part when orientated according to primary view
    width =
        abs(right.X() * bbox.LengthX() + right.Y() * bbox.LengthY() + right.Z() * bbox.LengthZ());
    height = abs(up.X() * bbox.LengthX() + up.Y() * bbox.LengthY() + up.Z() * bbox.LengthZ());
    depth = abs(facing.X() * bbox.LengthX() + facing.Y() * bbox.LengthY()
                + facing.Z() * bbox.LengthZ());

    if (views.size() == 0) {
        add_view(0, 0);
    }
    else {
        views[0]->set_projection(primary);
        set_all_orientations();  // reorient all other views appropriately
        process_views();
    }
}

void OrthoViews::set_orientation(int index)  // set orientation of single view
{
    double rotation;
    int n;       // how many 90* rotations from primary view?
    gp_Dir dir;  // rotate about primary x axis (if in a relative y position) or y axis?
    gp_Ax2 cs;

    if (views[index]->ortho) {
        if (views[index]->rel_x != 0) {
            dir = primary.YDirection();
            n = views[index]->rel_x;
        }
        else {
            dir = primary.XDirection();
            n = -views[index]->rel_y;
        }

        rotation = n * rotate_coeff * M_PI / 2;  // rotate_coeff is -1 or 1 for 1st or 3rd angle
        cs = primary.Rotated(gp_Ax1(gp_Pnt(0, 0, 0), dir), rotation);
        views[index]->set_projection(cs);
    }
}

void OrthoViews::set_all_orientations()  // set orientations of all views (ie projection or primary
                                         // changed)
{
    for (unsigned int i = 1; i < views.size(); i++)  // start from 1 - the 0 is the primary view
    {
        if (views[i]->ortho) {
            set_orientation(i);
        }
        else {
            set_Axo(views[i]->rel_x, views[i]->rel_y);
        }
    }
}

void OrthoViews::set_projection(int proj)  // 1 = 1st angle, 3 = 3rd angle
{
    if (proj == 3) {
        rotate_coeff = 1;
    }
    else if (proj == 1) {
        rotate_coeff = -1;
    }

    set_all_orientations();
    process_views();
}

void OrthoViews::add_view(int rel_x, int rel_y)  // add a new view to the layout
{
    if (index(rel_x, rel_y) == -1) {
        orthoview* view = new orthoview(parent_doc, part, page, &bbox);
        view->set_data(rel_x, rel_y);
        views.push_back(view);

        max_r_x = max(max_r_x, rel_x);
        min_r_x = min(min_r_x, rel_x);
        max_r_y = max(max_r_y, rel_y);
        min_r_y = min(min_r_y, rel_y);

        num_gaps_x = max_r_x - min_r_x + 2;
        num_gaps_y = max_r_y - min_r_y + 2;

        int i = views.size() - 1;
        views[i]->hidden(hidden);
        views[i]->smooth(smooth);

        if (views[i]->ortho) {
            set_orientation(i);
        }
        else {
            set_Axo(rel_x, rel_y);
        }

        process_views();
    }
}

void OrthoViews::del_view(int rel_x, int rel_y)  // remove a view from the layout
{
    int num = index(rel_x, rel_y);

    if (num > 0) {
        {
            boost::signals2::shared_connection_block blocker(connectDocumentDeletedObject);
            views[num]->deleteme();
            delete views[num];
            views.erase(views.begin() + num);
        }

        min_r_x = max_r_x = 0;
        min_r_y = max_r_y = 0;

        for (unsigned int i = 1; i < views.size(); i++)  // start from 1 - the 0 is the primary view
        {
            min_r_x = min(min_r_x, views[i]->rel_x);  // calculate extremes from remaining views
            max_r_x = max(max_r_x, views[i]->rel_x);
            min_r_y = min(min_r_y, views[i]->rel_y);
            max_r_y = max(max_r_y, views[i]->rel_y);
        }

        num_gaps_x = max_r_x - min_r_x + 2;
        num_gaps_y = max_r_y - min_r_y + 2;

        process_views();
    }
}

void OrthoViews::del_all()
{
    boost::signals2::shared_connection_block blocker(connectDocumentDeletedObject);
    for (int i = views.size() - 1; i >= 0; i--)  // count downwards to delete from back
    {
        views[i]->deleteme();
        delete views[i];
        views.pop_back();
    }
}

int OrthoViews::is_Ortho(int rel_x, int rel_y)  // is the view at r_x, r_y an ortho or axo one?
{
    int result = index(rel_x, rel_y);

    if (result != -1) {
        result = views[result]->ortho;
    }

    return result;
}

int OrthoViews::index(int rel_x, int rel_y)  // index in vector of view, -1 if doesn't exist
{
    int index = -1;

    for (unsigned int i = 0; i < views.size(); i++) {
        if (views[i]->rel_x == rel_x && views[i]->rel_y == rel_y) {
            index = i;
            break;
        }
    }

    return index;
}

void OrthoViews::set_Axo_scale(int rel_x,
                               int rel_y,
                               float axo_scale)  // set an axo scale independent of ortho ones
{
    int num = index(rel_x, rel_y);

    if (num != -1 && !views[num]->ortho) {
        views[num]->auto_scale = false;
        views[num]->setScale(axo_scale);
        views[num]->setPos();
        parent_doc->recompute();
    }
}

void OrthoViews::set_Axo(int rel_x,
                         int rel_y,
                         gp_Dir up,
                         gp_Dir right,
                         bool away,
                         int axo,
                         bool tri)  // set custom axonometric view
{
    double rotations[2];

    if (axo == 0) {
        rotations[0] = -0.7853981633974476;
        rotations[1] = -0.6154797086703873;
    }
    else if (axo == 1) {
        rotations[0] = -0.7853981633974476;
        rotations[1] = -0.2712637537260206;
    }
    else if (tri) {
        rotations[0] = -1.3088876392502007;
        rotations[1] = -0.6156624905260762;
    }
    else {
        rotations[0] = 1.3088876392502007 - M_PI / 2;
        rotations[1] = -0.6156624905260762;
    }

    if (away) {
        rotations[1] = -rotations[1];
    }

    gp_Ax2 cs = gp_Ax2(gp_Pnt(0, 0, 0), right);
    cs.SetYDirection(up);
    cs.Rotate(gp_Ax1(gp_Pnt(0, 0, 0), up), rotations[0]);
    gp_Dir dir;
    dir = cs.XDirection();
    cs.Rotate(gp_Ax1(gp_Pnt(0, 0, 0), dir), rotations[1]);

    int num = index(rel_x, rel_y);
    if (num != -1) {
        views[num]->ortho = false;
        views[num]->away = away;
        views[num]->tri = tri;
        views[num]->axo = axo;

        views[num]->up = up;
        views[num]->right = right;
        views[num]->set_projection(cs);
        views[num]->setPos();
    }

    parent_doc->recompute();
}

void OrthoViews::set_Axo(int rel_x, int rel_y)  // set view to default axo projection
{
    int num = index(rel_x, rel_y);

    if (num != -1) {
        gp_Dir up = primary.YDirection();  // default to view from up and right
        gp_Dir right = primary.XDirection();
        bool away = false;

        if (rel_x * rel_y != 0)  // but change default if it's a diagonal position
        {
            if (rotate_coeff == 1)  // third angle
            {
                away = (rel_y < 0);

                if (rel_x < 0) {
                    right = primary.Direction();
                }
                else {
                    right = primary.XDirection();
                }
            }
            else  // first angle
            {
                away = (rel_y > 0);

                if (rel_x > 0) {
                    right = primary.Direction();
                }
                else {
                    right = primary.XDirection();
                }
            }
        }
        set_Axo(rel_x, rel_y, up, right, away);
    }
}

void OrthoViews::set_Ortho(int rel_x, int rel_y)  // return view to orthographic
{
    int num = index(rel_x, rel_y);

    if (num != -1 && rel_x * rel_y == 0) {
        views[num]->ortho = true;
        views[num]->setScale(scale);
        views[num]->auto_scale = true;
        set_orientation(num);
        views[num]->setPos();

        parent_doc->recompute();
    }
}

bool OrthoViews::get_Axo(int rel_x,
                         int rel_y,
                         int& axo,
                         gp_Dir& up,
                         gp_Dir& right,
                         bool& away,
                         bool& tri,
                         float& axo_scale)
{
    int num = index(rel_x, rel_y);

    if (num != -1 && !views[num]->ortho) {
        axo = views[num]->axo;
        up = views[num]->up;
        right = views[num]->right;
        away = views[num]->away;
        tri = views[num]->tri;
        axo_scale = views[num]->getScale();
        return true;
    }
    else {
        return false;
    }
}

void OrthoViews::auto_dims(bool setting)
{
    autodims = setting;
    if (autodims) {
        process_views();
    }
}

void OrthoViews::set_configs(float configs[5])  // for autodims off, set scale & positionings
{
    if (!autodims) {
        scale = configs[0];
        offset_x = configs[1];
        offset_y = configs[2];
        gap_x = configs[3];
        gap_y = configs[4];
        process_views();
    }
}

void OrthoViews::get_configs(float configs[5])  // get scale & positionings
{
    configs[0] = scale;
    configs[1] = offset_x;
    configs[2] = offset_y;
    configs[3] = gap_x;
    configs[4] = gap_y;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

TaskOrthoViews::TaskOrthoViews(QWidget* parent)
    : ui(new Ui_TaskOrthoViews)
{
    Q_UNUSED(parent);
    ui->setupUi(this);
    std::vector<App::DocumentObject*> obj =
        Gui::Selection().getObjectsOfType(Part::Feature::getClassTypeId());
    const char* part = obj.front()->getNameInDocument();

    App::Document* doc = App::GetApplication().getActiveDocument();
    std::vector<App::DocumentObject*> pages =
        Gui::Selection().getObjectsOfType(Drawing::FeaturePage::getClassTypeId());
    if (pages.empty()) {
        pages = doc->getObjectsOfType(Drawing::FeaturePage::getClassTypeId());
    }

    std::string PageName = pages.front()->getNameInDocument();
    const char* page = PageName.c_str();


    // **********************************************************************
    // note that checkboxes are numbered increasing right & down
    // while OrthoViews relative positions are increasing right & up
    // doh!  I should renumber the checkboxes for clarity
    // **********************************************************************

    //   [x+2][y+2]
    c_boxes[0][2] = ui->cb02;  // left most, x = -2, y = 0
    c_boxes[1][1] = ui->cb11;
    c_boxes[1][2] = ui->cb12;
    c_boxes[1][3] = ui->cb13;
    c_boxes[2][0] = ui->cb20;  // top most, x = 0, y = -2
    c_boxes[2][1] = ui->cb21;
    c_boxes[2][2] = ui->cb22;  // centre (primary view) checkbox x = y = 0.
    c_boxes[2][3] = ui->cb23;
    c_boxes[2][4] = ui->cb24;  // bottom most, x = 0, y = 2
    c_boxes[3][1] = ui->cb31;
    c_boxes[3][2] = ui->cb32;
    c_boxes[3][3] = ui->cb33;
    c_boxes[4][2] = ui->cb42;  // right most, x = 2, y = 0

    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            if ((abs(i - 2) + abs(j - 2))
                < 3)  // if i,j combination corresponds to valid check box, then proceed with:
            {
                connect(c_boxes[i][j], SIGNAL(toggled(bool)), this, SLOT(cb_toggled(bool)));
                connect(c_boxes[i][j],
                        SIGNAL(customContextMenuRequested(const QPoint&)),
                        this,
                        SLOT(ShowContextMenu(const QPoint&)));
            }
        }
    }

    // access scale / position QLineEdits via array
    inputs[0] = ui->scale_0;
    inputs[1] = ui->x_1;
    inputs[2] = ui->y_2;
    inputs[3] = ui->spacing_h_3;
    inputs[4] = ui->spacing_v_4;

    for (int i = 0; i < 5; i++) {
        connect(inputs[i],
                SIGNAL(textEdited(const QString&)),
                this,
                SLOT(data_entered(const QString&)));
        connect(inputs[i], SIGNAL(returnPressed()), this, SLOT(text_return()));
    }

    connect(ui->projection, SIGNAL(currentIndexChanged(int)), this, SLOT(projectionChanged(int)));
    connect(ui->smooth, SIGNAL(stateChanged(int)), this, SLOT(smooth(int)));
    connect(ui->hidden, SIGNAL(stateChanged(int)), this, SLOT(hidden(int)));
    connect(ui->auto_tog, SIGNAL(stateChanged(int)), this, SLOT(toggle_auto(int)));

    connect(ui->view_from, SIGNAL(currentIndexChanged(int)), this, SLOT(setPrimary(int)));
    connect(ui->axis_right, SIGNAL(currentIndexChanged(int)), this, SLOT(setPrimary(int)));

    connect(ui->axoProj, SIGNAL(activated(int)), this, SLOT(change_axo(int)));
    connect(ui->axoUp, SIGNAL(activated(int)), this, SLOT(change_axo(int)));
    connect(ui->axoRight, SIGNAL(activated(int)), this, SLOT(change_axo(int)));
    connect(ui->vert_flip, SIGNAL(clicked()), this, SLOT(axo_button()));
    connect(ui->tri_flip, SIGNAL(clicked()), this, SLOT(axo_button()));
    connect(ui->axoScale,
            SIGNAL(textEdited(const QString&)),
            this,
            SLOT(axo_scale(const QString&)));
    connect(ui->axoScale, SIGNAL(returnPressed()), this, SLOT(text_return()));

    ui->tabWidget->setTabEnabled(1, false);

    gp_Dir facing = gp_Dir(1, 0, 0);
    gp_Dir right = gp_Dir(0, 1, 0);
    orthos = new OrthoViews(doc, page, part);
    orthos->set_primary(facing, right);

    txt_return = false;
}  // end of constructor

TaskOrthoViews::~TaskOrthoViews()
{
    delete orthos;
    delete ui;
}

void TaskOrthoViews::ShowContextMenu(const QPoint& pos)
{
    QString name = sender()->objectName().right(2);
    char letter = name.toStdString()[0];
    int dx = letter - '0' - 2;

    letter = name.toStdString()[1];
    int dy = letter - '0' - 2;

    if (c_boxes[dx + 2][dy + 2]->isChecked()) {
        QString str_1 = QObject::tr("Make axonometric...");
        QString str_2 = QObject::tr("Edit axonometric settings...");
        QString str_3 = QObject::tr("Make orthographic");

        QPoint globalPos = c_boxes[dx + 2][dy + 2]->mapToGlobal(pos);
        QMenu myMenu;
        if (orthos->is_Ortho(dx, -dy)) {
            myMenu.addAction(str_1);
        }
        else {
            myMenu.addAction(str_2);
            if (dx * dy == 0) {
                myMenu.addAction(str_3);
            }
        }

        QAction* selectedItem = myMenu.exec(globalPos);
        if (selectedItem) {
            QString text = selectedItem->text();

            if (text == str_1)  // make axo
            {
                orthos->set_Axo(dx, -dy);
                axo_r_x = dx;
                axo_r_y = dy;
                ui->tabWidget->setTabEnabled(1, true);
                ui->tabWidget->setCurrentIndex(1);
                setup_axo_tab();
            }
            else if (text == str_2)  // edit axo
            {
                axo_r_x = dx;
                axo_r_y = dy;
                ui->tabWidget->setTabEnabled(1, true);
                ui->tabWidget->setCurrentIndex(1);
                setup_axo_tab();
            }
            else if (text == str_3)  // make ortho
            {
                orthos->set_Ortho(dx, -dy);
                if (dx == axo_r_x && dy == axo_r_y) {
                    axo_r_x = 0;
                    axo_r_y = 0;
                    ui->tabWidget->setTabEnabled(1, false);
                }
            }
        }
    }
}

void TaskOrthoViews::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

void TaskOrthoViews::cb_toggled(bool toggle)
{
    QString name = sender()->objectName().right(2);
    char letter = name.toStdString()[0];
    int dx = letter - '0' - 2;

    letter = name.toStdString()[1];
    int dy = letter - '0' - 2;

    if (toggle) {
        orthos->add_view(dx, -dy);
        if (dx * dy != 0)  // adding an axo view
        {
            axo_r_x = dx;
            axo_r_y = dy;
            ui->tabWidget->setTabEnabled(1, true);
            ui->tabWidget->setCurrentIndex(1);
            setup_axo_tab();
        }
    }
    else  // removing a view
    {
        if (!orthos->is_Ortho(dx, -dy))  // is it an axo one?
        {
            if (dx == axo_r_x && dy == axo_r_y)  // is it the one currently being edited?
            {
                axo_r_x = 0;
                axo_r_y = 0;
                ui->tabWidget->setTabEnabled(1, false);
            }
        }
        orthos->del_view(dx, -dy);
    }

    set_configs();
}

void TaskOrthoViews::projectionChanged(int index)
{
    int proj = 3 - 2 * index;  // index = 0 = third angle
    orthos->set_projection(proj);

    set_configs();
}

void TaskOrthoViews::setPrimary(int /*dir*/)
{
    int p_sel = ui->view_from->currentIndex();   // index for entry selected for 'view from'
    int r_sel = ui->axis_right->currentIndex();  // index for entry selected for 'rightwards axis'

    int p_vec[3] = {0, 0, 0};  // will be the vector for 'view from'
    int r_vec[3] = {0, 0, 0};  // will be vector for 'rightwards axis'
    int r[2] = {0, 1};

    int pos = 1 - 2 * int(p_sel / 3);  // 1 if p_sel = 0, 1, 2  or -1 if p_sel = 3, 4, 5
    p_sel = p_sel % 3;                 // p_sel = 0, 1, 2
    p_vec[p_sel] = pos;

    for (int i = p_sel; i < 2;
         i++) {  // make r[2] to be, {0, 1}, {0, 2}, or {1, 2} depending upon p_sel
        r[i] += 1;
    }

    pos = 1 - 2 * int(r_sel / 2);  // 1 if r_sel = 0, 1  or -1 if r_sel = 3, 4
    r_sel = r_sel % 2;             // r_sel = 0, 1
    r_vec[r[r_sel]] = pos;

    gp_Dir facing = gp_Dir(p_vec[0], p_vec[1], p_vec[2]);
    gp_Dir right = gp_Dir(r_vec[0], r_vec[1], r_vec[2]);

    orthos->set_primary(facing, right);

    // update rightwards combobox in case of 'view from' change
    if (sender() == ui->view_from) {
        disconnect(ui->axis_right, SIGNAL(currentIndexChanged(int)), this, SLOT(setPrimary(int)));

        QStringList items;
        items << QString::fromUtf8("X +ve") << QString::fromUtf8("Y +ve")
              << QString::fromUtf8("Z +ve");
        items << QString::fromUtf8("X -ve") << QString::fromUtf8("Y -ve")
              << QString::fromUtf8("Z -ve");
        items.removeAt(p_sel + 3);
        items.removeAt(p_sel);

        ui->axis_right->clear();
        ui->axis_right->addItems(items);
        ui->axis_right->setCurrentIndex(r_sel - pos + 1);

        connect(ui->axis_right, SIGNAL(currentIndexChanged(int)), this, SLOT(setPrimary(int)));
    }

    set_configs();
}

void TaskOrthoViews::hidden(int i)
{
    orthos->set_hidden(i == 2);
}


void TaskOrthoViews::smooth(int i)
{
    orthos->set_smooth(i == 2);
}

void TaskOrthoViews::toggle_auto(int i)
{
    if (i == 2)  // auto scale switched on
    {
        orthos->auto_dims(true);
        ui->label_4->setEnabled(false);
        ui->label_5->setEnabled(false);
        ui->label_6->setEnabled(false);

        for (int j = 0; j < 5; j++) {
            inputs[j]->setEnabled(false);  // disable user input boxes
        }
    }
    else {
        orthos->auto_dims(false);
        ui->label_4->setEnabled(true);
        ui->label_5->setEnabled(true);
        ui->label_6->setEnabled(true);

        for (int j = 0; j < 5; j++) {
            inputs[j]->setEnabled(true);  // enable user input boxes
        }
        set_configs();
    }
}

void TaskOrthoViews::data_entered(const QString& text)
{
    bool ok;
    QString name = sender()->objectName().right(1);
    char letter = name.toStdString()[0];
    int index = letter - '0';

    float value = text.toFloat(&ok);

    if (ok) {
        data[index] = value;
        orthos->set_configs(data);
    }
    else {
        inputs[index]->setText(QString::number(data[index]));
        return;
    }
}

void TaskOrthoViews::clean_up()
{
    orthos->del_all();
}

void TaskOrthoViews::setup_axo_tab()
{
    int axo;
    gp_Dir up, right;
    bool away, tri;
    float axo_scale;
    int up_n, right_n;

    orthos->get_Axo(axo_r_x, -axo_r_y, axo, up, right, away, tri, axo_scale);

    // convert gp_Dirs into selections of comboboxes
    if (up.X() != 0) {
        up_n = (up.X() == -1) ? 3 : 0;
    }
    else if (up.Y() != 0) {
        up_n = (up.Y() == -1) ? 4 : 1;
    }
    else {
        up_n = (up.Z() == -1) ? 5 : 2;
    }

    if (right.X() != 0) {
        right_n = (right.X() == -1) ? 3 : 0;
    }
    else if (right.Y() != 0) {
        right_n = (right.Y() == -1) ? 4 : 1;
    }
    else {
        right_n = (right.Z() == -1) ? 5 : 2;
    }

    if (right_n > (up_n % 3 + 3)) {
        right_n -= 2;
    }
    else if (right_n > up_n) {
        right_n -= 1;
    }

    QStringList items;
    items << QString::fromUtf8("X +ve") << QString::fromUtf8("Y +ve") << QString::fromUtf8("Z +ve");
    items << QString::fromUtf8("X -ve") << QString::fromUtf8("Y -ve") << QString::fromUtf8("Z -ve");
    items.removeAt(up_n % 3 + 3);
    items.removeAt(up_n % 3);

    ui->axoUp->setCurrentIndex(up_n);
    ui->axoRight->clear();
    ui->axoRight->addItems(items);
    ui->axoRight->setCurrentIndex(right_n);

    ui->vert_flip->setChecked(away);
    ui->tri_flip->setChecked(tri);
    ui->axoProj->setCurrentIndex(axo);
    ui->axoScale->setText(QString::number(axo_scale));
}

void TaskOrthoViews::change_axo(int /*p*/)
{
    int u_sel = ui->axoUp->currentIndex();     // index for entry selected for 'view from'
    int r_sel = ui->axoRight->currentIndex();  // index for entry selected for 'rightwards axis'

    int u_vec[3] = {0, 0, 0};  // will be the vector for 'view from'
    int r_vec[3] = {0, 0, 0};  // will be vector for 'rightwards axis'
    int r[2] = {0, 1};

    int pos = 1 - 2 * int(u_sel / 3);  // 1 if p_sel = 0,1,2  or -1 if p_sel = 3,4,5
    u_sel = u_sel % 3;                 // p_sel = 0,1,2
    u_vec[u_sel] = pos;

    for (int i = u_sel; i < 2; i++) {
        r[i] += 1;
    }

    pos = 1 - 2 * int(r_sel / 2);
    r_sel = r_sel % 2;
    r_vec[r[r_sel]] = pos;

    gp_Dir up = gp_Dir(u_vec[0], u_vec[1], u_vec[2]);
    gp_Dir right = gp_Dir(r_vec[0], r_vec[1], r_vec[2]);

    orthos->set_Axo(axo_r_x,
                    -axo_r_y,
                    up,
                    right,
                    ui->vert_flip->isChecked(),
                    ui->axoProj->currentIndex(),
                    ui->tri_flip->isChecked());

    if (ui->axoProj->currentIndex() == 2) {
        ui->tri_flip->setEnabled(true);
    }
    else {
        ui->tri_flip->setEnabled(false);
    }


    QStringList items;
    items << QString::fromUtf8("X +ve") << QString::fromUtf8("Y +ve") << QString::fromUtf8("Z +ve");
    items << QString::fromUtf8("X -ve") << QString::fromUtf8("Y -ve") << QString::fromUtf8("Z -ve");
    items.removeAt(u_sel % 3 + 3);
    items.removeAt(u_sel % 3);

    ui->axoRight->clear();
    ui->axoRight->addItems(items);
    ui->axoRight->setCurrentIndex(r_sel - pos + 1);
}

void TaskOrthoViews::axo_button()
{
    change_axo();
}

void TaskOrthoViews::axo_scale(const QString& text)
{
    bool ok;
    float value = text.toFloat(&ok);

    if (ok) {
        orthos->set_Axo_scale(axo_r_x, -axo_r_y, value);
    }
}

void TaskOrthoViews::set_configs()
{
    orthos->get_configs(data);

    for (int i = 0; i < 5; i++) {
        inputs[i]->setText(QString::number(data[i]));
    }
}

bool TaskOrthoViews::user_input()
{
    if (txt_return) {
        txt_return = false;       // return was pressed while text box had focus
        ui->label_7->setFocus();  // move focus out of text box
        return true;              // return that we were editing
    }
    else {
        return false;  // return that we weren't editing ---> treat as clicking OK... we can close
                       // the GUI
    }
}

void TaskOrthoViews::text_return()
{
    txt_return = true;
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgOrthoViews::TaskDlgOrthoViews()
    : TaskDialog()
{
    widget = new TaskOrthoViews();
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/drawing-orthoviews"),
                                         widget->windowTitle(),
                                         true,
                                         nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgOrthoViews::~TaskDlgOrthoViews()
{}

//==== calls from the TaskView ===============================================================


void TaskDlgOrthoViews::open()
{}

void TaskDlgOrthoViews::clicked(int)
{}

bool TaskDlgOrthoViews::accept()
{
    bool check = widget->user_input();
    App::Document* doc = App::GetApplication().getDocument(this->getDocumentName().c_str());
    if (doc) {
        doc->commitTransaction();
    }
    return !check;
}

bool TaskDlgOrthoViews::reject()
{
    widget->clean_up();
    App::Document* doc = App::GetApplication().getDocument(this->getDocumentName().c_str());
    if (doc) {
        doc->abortTransaction();
    }
    return true;
}


#include "moc_TaskOrthoViews.cpp"
