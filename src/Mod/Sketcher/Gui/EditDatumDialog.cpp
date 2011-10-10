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

#include "PreCompiled.h"

#ifndef _PreComp_
# include <Standard_math.hxx>
/// Qt Include Files
# include <QApplication>
# include <QDialog>
# include <QMessageBox>
#endif

# include <Inventor/sensors/SoSensor.h>

#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "ViewProviderSketch.h"
#include "ui_InsertDatum.h"
#include "EditDatumDialog.h"

using namespace SketcherGui;

EditDatumDialog::EditDatumDialog(ViewProviderSketch* vp, int ConstrNbr) : vp(vp), ConstrNbr(ConstrNbr)
{
    const std::vector<Sketcher::Constraint *> &Constraints = vp->getSketchObject()->Constraints.getValues();
    Constr = Constraints[ConstrNbr];
}

EditDatumDialog::~EditDatumDialog(){}

void EditDatumDialog::run(void * data, SoSensor * sensor)
{
    EditDatumDialog* self = reinterpret_cast<EditDatumDialog*>(data);
    self->exec();
    delete self;
    delete sensor;
}

void EditDatumDialog::exec(bool atCursor)
{
    // Return if constraint doesn't have editable value
    if (Constr->Type == Sketcher::Distance ||
        Constr->Type == Sketcher::DistanceX || Constr->Type == Sketcher::DistanceY ||
        Constr->Type == Sketcher::Radius || Constr->Type == Sketcher::Angle) {

        if (vp->getSketchObject()->hasConflicts()) {
            QMessageBox::critical(qApp->activeWindow(), QObject::tr("Distance constraint"),
                                  QObject::tr("Not allowed to edit the datum because the sketch contains conflicting constraints"));
            return;
		}

        double datum = Constr->Value;
        if (Constr->Type == Sketcher::Angle)
            datum = datum * 180./M_PI;

        Gui::MDIView *mdi = Gui::Application::Instance->activeDocument()->getActiveView();
        Gui::View3DInventorViewer *viewer = static_cast<Gui::View3DInventor *>(mdi)->getViewer();

        QDialog dlg(viewer->getGLWidget());

        Ui::InsertDatum ui_ins_datum;
        ui_ins_datum.setupUi(&dlg);

        ui_ins_datum.lineEdit->setText(QLocale::system().toString(datum,'g',6));
        ui_ins_datum.lineEdit->selectAll();

        if (atCursor)
            dlg.setGeometry(QCursor::pos().x() - dlg.geometry().width() / 2, QCursor::pos().y(), dlg.geometry().width(), dlg.geometry().height());

        if (dlg.exec()) {
            bool ok;
            double newDatum = ui_ins_datum.lineEdit->text().toDouble(&ok);
            if (ok) {
                if (Constr->Type == Sketcher::Angle)
                    newDatum = newDatum * M_PI/180.;
                try {
                    Gui::Command::openCommand("Modify sketch constraints");
                    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.setDatum(%i,%f)",
                                vp->getObject()->getNameInDocument(),
                                ConstrNbr, newDatum);
                }
                catch (const Base::Exception& e) {
                    QMessageBox::critical(qApp->activeWindow(), QObject::tr("Distance constraint"), QString::fromUtf8(e.what()));
                }
            }
        }
    }
}
