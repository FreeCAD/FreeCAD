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

#include <Base/Tools.h>
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

/* TRANSLATOR SketcherGui::EditDatumDialog */

EditDatumDialog::EditDatumDialog(ViewProviderSketch* vp, int ConstrNbr) : ConstrNbr(ConstrNbr)
{
    sketch = vp->getSketchObject();
    const std::vector<Sketcher::Constraint *> &Constraints = sketch->Constraints.getValues();
    Constr = Constraints[ConstrNbr];
}

EditDatumDialog::EditDatumDialog(Sketcher::SketchObject* pcSketch, int ConstrNbr) : sketch(pcSketch), ConstrNbr(ConstrNbr)
{
    const std::vector<Sketcher::Constraint *> &Constraints = sketch->Constraints.getValues();
    Constr = Constraints[ConstrNbr];
}

EditDatumDialog::~EditDatumDialog(){}

void EditDatumDialog::customEvent(QEvent*)
{
    this->exec();
    this->deleteLater();
}

void EditDatumDialog::exec(bool atCursor)
{
    // Return if constraint doesn't have editable value
    if (Constr->Type == Sketcher::Distance ||
        Constr->Type == Sketcher::DistanceX || 
        Constr->Type == Sketcher::DistanceY ||
        Constr->Type == Sketcher::Radius || 
        Constr->Type == Sketcher::Angle ||
        Constr->Type == Sketcher::SnellsLaw) {

        if (sketch->hasConflicts()) {
            QMessageBox::critical(qApp->activeWindow(), QObject::tr("Distance constraint"),
                                  QObject::tr("Not allowed to edit the datum because the sketch contains conflicting constraints"));
            return;
        }

        Gui::MDIView *mdi = Gui::Application::Instance->activeDocument()->getActiveView();
        Gui::View3DInventorViewer *viewer = static_cast<Gui::View3DInventor *>(mdi)->getViewer();

        QDialog dlg(viewer->getGLWidget());

        Ui::InsertDatum ui_ins_datum;
        ui_ins_datum.setupUi(&dlg);
        double datum = Constr->Value;
        Base::Quantity init_val;

        if (Constr->Type == Sketcher::Angle) {
            datum = Base::toDegrees<double>(datum);
            dlg.setWindowTitle(tr("Insert angle"));
            init_val.setUnit(Base::Unit::Angle);
            ui_ins_datum.label->setText(tr("Angle:"));
            ui_ins_datum.labelEdit->setParamGrpPath(QByteArray("User parameter:BaseApp/History/SketcherAngle"));
        }
        else if (Constr->Type == Sketcher::Radius) {
            dlg.setWindowTitle(tr("Insert radius"));
            init_val.setUnit(Base::Unit::Length);
            ui_ins_datum.label->setText(tr("Radius:"));
            ui_ins_datum.labelEdit->setParamGrpPath(QByteArray("User parameter:BaseApp/History/SketcherLength"));
        }
        else if (Constr->Type == Sketcher::SnellsLaw) {
            dlg.setWindowTitle(tr("Refractive index ratio", "Constraint_SnellsLaw"));
            ui_ins_datum.label->setText(tr("Ratio n2/n1:", "Constraint_SnellsLaw"));
            ui_ins_datum.labelEdit->setParamGrpPath(QByteArray("User parameter:BaseApp/History/SketcherRefrIndexRatio"));
        }
        else {
            dlg.setWindowTitle(tr("Insert length"));
            init_val.setUnit(Base::Unit::Length);
            ui_ins_datum.label->setText(tr("Length:"));
            ui_ins_datum.labelEdit->setParamGrpPath(QByteArray("User parameter:BaseApp/History/SketcherLength"));
        }


        //ui_ins_datum.lineEdit->setParamGrpPath("User parameter:History/Sketcher/SetDatum");

        // e.g. an angle or a distance X or Y applied on a line or two vertexes
        if (Constr->Type == Sketcher::Angle ||
            ((Constr->Type == Sketcher::DistanceX || Constr->Type == Sketcher::DistanceY) &&
             (Constr->FirstPos == Sketcher::none || Constr->Second != Sketcher::Constraint::GeoUndef)))
            // hide negative sign
            init_val.setValue(std::abs(datum));

        else // show negative sign
            init_val.setValue(datum);

        ui_ins_datum.labelEdit->setValue(init_val);
        ui_ins_datum.labelEdit->selectNumber();

        if (atCursor)
            dlg.setGeometry(QCursor::pos().x() - dlg.geometry().width() / 2, QCursor::pos().y(), dlg.geometry().width(), dlg.geometry().height());

        if (dlg.exec()) {
            Base::Quantity newQuant = ui_ins_datum.labelEdit->value();
            if (newQuant.isQuantity() || (Constr->Type == Sketcher::SnellsLaw && newQuant.isDimensionless())) {
                // save the value for the history 
                ui_ins_datum.labelEdit->pushToHistory();

                double newDatum = newQuant.getValue();
                if (Constr->Type == Sketcher::Angle ||
                    ((Constr->Type == Sketcher::DistanceX || Constr->Type == Sketcher::DistanceY) &&
                     Constr->FirstPos == Sketcher::none || Constr->Second != Sketcher::Constraint::GeoUndef)) {
                    // Permit negative values to flip the sign of the constraint
                    if (newDatum >= 0) // keep the old sign
                        newDatum = ((datum >= 0) ? 1 : -1) * std::abs(newDatum);
                    else // flip sign
                        newDatum = ((datum >= 0) ? -1 : 1) * std::abs(newDatum);
                }

                try {
                    Gui::Command::openCommand("Modify sketch constraints");
                    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.setDatum(%i,App.Units.Quantity('%f %s'))",
                                sketch->getNameInDocument(),
                                ConstrNbr, newDatum, (const char*)newQuant.getUnit().getString().toUtf8());
                    Gui::Command::commitCommand();
                    Gui::Command::updateActive();
                }
                catch (const Base::Exception& e) {
                    QMessageBox::critical(qApp->activeWindow(), QObject::tr("Dimensional constraint"), QString::fromUtf8(e.what()));
                    Gui::Command::abortCommand();
                }
            }
        }
    }
}
