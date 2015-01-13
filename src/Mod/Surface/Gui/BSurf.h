/***************************************************************************
 *   Copyright (c) 2015 Balázs Bámer                                       *
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

// Part::CrossSections.*

#ifndef SURFACE_GUI_BSURF_H
#define SURFACE_GUI_BSURF_H

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Base/BoundBox.h>
#include <QDialog>
#include <QPointer>
#include "../FillType.h"
#include "ui_BSurf.h"

namespace SurfaceGui
{
    class ViewProviderBSurf;

    class BSurf : public QDialog
    {
        Q_OBJECT
    protected:
        filltype_t fillType;

    public:
        BSurf(ViewProviderBSurf* vp);
        ~BSurf();
        void accept();
        void apply();

    protected:
        void changeEvent(QEvent *e);

    private Q_SLOTS:
        void on_fillType_stretch_clicked();
        void on_fillType_coons_clicked();
        void on_fillType_curved_clicked();
        filltype_t getFillType() const;

/*    private:
        std::vector<double> getPlanes() const;
        void calcPlane(Plane, double);
        void calcPlanes(Plane);
        void makePlanes(Plane, const std::vector<double>&, double[4]);
        Plane plane() const;*/

    private:
        Ui_DlgBSurf* ui;
        Base::BoundBox3d bbox;
        ViewProviderBSurf* vp;
//        QPointer<Gui::View3DInventor> view;
    };

    class TaskBSurf : public Gui::TaskView::TaskDialog
    {
        Q_OBJECT

    public:
        TaskBSurf(ViewProviderBSurf* vp);
        ~TaskBSurf();

    public:
        bool accept();
        void clicked(int id);

        virtual QDialogButtonBox::StandardButtons getStandardButtons() const
        { return QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel; }

    private:
        BSurf* widget;
        Gui::TaskView::TaskBox* taskbox;
        ViewProviderBSurf* view;
    };

} //namespace Surface

#endif // SURFACE_GUI_BSURF_H
