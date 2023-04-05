/***************************************************************************
 *   Copyright (c) 2012 Petar Perisin <petar.perisin@gmail.com>            *
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


#ifndef DLGCUSTOMIZESPNAVSETTINGS_H
#define DLGCUSTOMIZESPNAVSETTINGS_H

#include "PropertyPage.h"
#include <Base/Parameter.h>
#include <memory>

class Ui_DlgCustomizeSpNavSettings;

namespace Gui
{
    namespace Dialog
    {
        class DlgCustomizeSpNavSettings : public CustomizeActionPage
        {
            Q_OBJECT

        public:
            explicit DlgCustomizeSpNavSettings(QWidget *parent = nullptr);
            ~DlgCustomizeSpNavSettings() override;

        protected Q_SLOTS:
            void onAddMacroAction(const QByteArray&) override;
            void onRemoveMacroAction(const QByteArray&) override;
            void onModifyMacroAction(const QByteArray&) override;

        protected:
            void setupConnections();
            void on_CBDominant_clicked();
            void on_CBFlipYZ_clicked();
            void on_CBRotations_clicked();
            void on_CBTranslations_clicked();
            void on_SliderGlobal_sliderReleased();
            void on_CBEnablePanLR_clicked();
            void on_CBReversePanLR_clicked();
            void on_SliderPanLR_sliderReleased();
            void on_CBEnablePanUD_clicked();
            void on_CBReversePanUD_clicked();
            void on_SliderPanUD_sliderReleased();
            void on_CBEnableZoom_clicked();
            void on_CBReverseZoom_clicked();
            void on_SliderZoom_sliderReleased();
            void on_CBEnableTilt_clicked();
            void on_CBReverseTilt_clicked();
            void on_SliderTilt_sliderReleased();
            void on_CBEnableRoll_clicked();
            void on_CBReverseRoll_clicked();
            void on_SliderRoll_sliderReleased();
            void on_CBEnableSpin_clicked();
            void on_CBReverseSpin_clicked();
            void on_SliderSpin_sliderReleased();
            void on_ButtonDefaultSpNavMotions_clicked();
            void on_ButtonCalibrate_clicked();

        protected:
            void changeEvent(QEvent *e) override;

        private:
            ParameterGrp::handle spaceballMotionGroup() const;
            void setMessage(const QString& message);
            void initialize();

        private:
            std::unique_ptr<Ui_DlgCustomizeSpNavSettings> ui;
            bool init;
        };
    }
}

#endif // DLGCUSTOMIZESPNAVSETTINGS_H
