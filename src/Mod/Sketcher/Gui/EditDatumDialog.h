/***************************************************************************
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

#ifndef SKETCHERGUI_EditDatumDialog_H
#define SKETCHERGUI_EditDatumDialog_H

class SoSensor;

namespace Sketcher {
class Constraint;
class SketchObject;
}

namespace SketcherGui {
class ViewProviderSketch;

class EditDatumDialog {
public:
    EditDatumDialog(ViewProviderSketch* vp, int ConstrNbr);
    EditDatumDialog(Sketcher::SketchObject* pcSketch, int ConstrNbr);
    ~EditDatumDialog();

    static void run(void * data, SoSensor * sensor);   
    void exec(bool atCursor=true);

private:
    Sketcher::SketchObject* sketch;
    Sketcher::Constraint* Constr;
    int ConstrNbr;
};

}
#endif // SKETCHERGUI_DrawSketchHandler_H
