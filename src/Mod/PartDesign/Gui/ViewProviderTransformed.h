// SPDX-License-Identifier: LGPL-2.1-or-later

/******************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/


#pragma once

#include "ViewProvider.h"

#include <Inventor/nodes/SoMultipleCopy.h>

namespace PartDesign
{
class Transformed;
}

namespace PartDesignGui
{

class TaskDlgTransformedParameters;

class PartDesignGuiExport ViewProviderTransformed: public ViewProvider
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesignGui::ViewProviderTransformed);

public:
    ViewProviderTransformed() = default;
    ~ViewProviderTransformed() override = default;

    // The feature name of the subclass
    virtual const std::string& featureName() const;
    std::string featureIcon() const;

    void recomputeFeature(bool recompute = true);
    void setupContextMenu(QMenu*, QObject*, const char*) override;

    /// signals if the transformation contains errors
    fastsignals::signal<void(QString msg)> signalDiagnosis;

    // Name of menu dialog
    QString menuName;

    Gui::ViewProvider* startEditing(int ModNum = 0) override;

    QString getMessage() const
    {
        return diagMessage;
    }

protected:
    bool setEdit(int ModNum) override;

    void attachPreview() override;
    void updatePreview() override;

    bool checkDlgOpen(TaskDlgTransformedParameters* transformedDlg);
    void handleTransformedResult(PartDesign::Transformed* transformed);

    Gui::CoinPtr<SoMultipleCopy> pcMultipleCopy;
    QString diagMessage;
};


}  // namespace PartDesignGui
