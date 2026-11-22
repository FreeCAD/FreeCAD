// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer                                    *
 *                                   <jrheinlaender@users.sourceforge.net> *
 *   Copyright (c) 2025 Kacper Donat <kacper@kadet.net>                    *
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


#pragma once

#include <optional>
#include <vector>

#include <App/DocumentObserver.h>
#include "ViewProvider.h"
#include <Gui/ViewProviderGeoFeatureGroupExtension.h>
#include <Gui/Inventor/SoToggleSwitch.h>
#include <fastsignals/connection.h>


namespace PartDesignGui
{

class PartDesignGuiExport ViewProviderBoolean: public ViewProvider,
                                               public Gui::ViewProviderGeoFeatureGroupExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(PartDesignGui::ViewProviderBoolean);

public:
    /// constructor
    ViewProviderBoolean();
    /// destructor
    ~ViewProviderBoolean() override;

    App::PropertyEnumeration Display;

    /// grouping handling
    void setupContextMenu(QMenu*, QObject*, const char*) override;

    void attach(App::DocumentObject*) override;
    void beforeDelete() override;
    bool onDelete(const std::vector<std::string>&) override;
    const char* getDefaultDisplayMode() const override;
    std::vector<App::DocumentObject*> claimChildren3D() const override;
    void onChanged(const App::Property* prop) override;
    void update(const App::Property* prop) override;

protected:
    void updateData(const App::Property* prop) override;

    void attachPreview() override;
    void updatePreview() override;

    TaskDlgFeatureParameters* getEditDialog() override;

    static const char* DisplayEnum[];

private:
    struct ViewProviderExposure
    {
        App::DocumentObjectWeakPtrT object;
        int mode;
        bool wasVisible;
        bool topLevelExposed;
    };

    struct ActiveBodyExposure
    {
        App::DocumentObjectWeakPtrT body;
        int booleanMode;
        bool booleanWasVisible;
        bool indirect;
        std::vector<ViewProviderExposure> viewProviders;
    };

    const char* getConfiguredDisplayMode();
    void updateBasePreviewVisibility();
    void updateToolsDisplay();
    void exposeViewProvider(
        ActiveBodyExposure& exposure,
        App::DocumentObject* object,
        bool groupMode,
        bool topLevel
    );
    void restoreActiveBodyExposure();
    void syncActiveBodyVisibility();
    void onBodyActivated(const Gui::ViewProviderDocumentObject* vp, const char* name);

    Gui::CoinPtr<SoGroup> pcToolsDisplay;
    Gui::CoinPtr<SoGroup> pcToolsPreview;
    Gui::CoinPtr<SoToggleSwitch> pcBasePreviewToggle;
    fastsignals::scoped_connection _bodyActivationConn;
    std::optional<ActiveBodyExposure> _activeBodyExposure;
};

}  // namespace PartDesignGui
