/***************************************************************************
 *   Copyright (c) 2023 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef GUI_VIEW3DSETTINGS_H
#define GUI_VIEW3DSETTINGS_H

#include <Base/Parameter.h>

namespace Gui {
class View3DInventorViewer;

class View3DSettings : public ParameterGrp::ObserverType
{
public:
    View3DSettings(ParameterGrp::handle hGrp, View3DInventorViewer *);
    View3DSettings(ParameterGrp::handle hGrp, const std::vector<View3DInventorViewer *>&);
    ~View3DSettings() override;

    /// Observer message from the ParameterGrp
    void OnChange(ParameterGrp::SubjectType &rCaller,ParameterGrp::MessageType Reason) override;
    void applySettings();
    int stopAnimatingIfDeactivated() const;

    bool ignoreNavigationStyle = false;
    bool ignoreVBO = false;
    bool ignoreDimensions = false;
    bool ignoreRenderCache = false;
    bool ignoreTransparent = false;

private:
    ParameterGrp::handle hGrp;
    std::vector<View3DInventorViewer*> _viewers;
};

class NaviCubeSettings : public ParameterGrp::ObserverType
{
public:
    NaviCubeSettings(ParameterGrp::handle hGrp, View3DInventorViewer *);
    ~NaviCubeSettings() override;

    /// Observer message from the ParameterGrp
    void OnChange(ParameterGrp::SubjectType &rCaller,ParameterGrp::MessageType Reason) override;
    void applySettings();

private:
    ParameterGrp::handle hGrp;
    View3DInventorViewer * _viewer;
};

} // namespace Gui

#endif  // GUI_VIEW3DSETTINGS_H
