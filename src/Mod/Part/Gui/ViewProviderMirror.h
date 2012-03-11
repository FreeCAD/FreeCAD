/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef PARTGUI_VIEWPROVIDERMIRROR_H
#define PARTGUI_VIEWPROVIDERMIRROR_H

#include <Mod/Part/Gui/ViewProvider.h>

namespace PartGui {


class PartGuiExport ViewProviderMirror : public ViewProviderPart
{
    PROPERTY_HEADER(PartGui::ViewProviderMirror);

public:
    ViewProviderMirror();
    virtual ~ViewProviderMirror();
    /** @name Edit methods */
    //@{
    void setupContextMenu(QMenu*, QObject*, const char*);
protected:
    bool setEdit(int ModNum);
    void unsetEdit(int ModNum);
    //@}

private:
    static void dragStartCallback(void * data, SoDragger * d);
    static void dragFinishCallback(void * data, SoDragger * d);
    static void dragMotionCallback(void * data, SoDragger * d);

private:
    SoSeparator* pcEditNode;
};

class PartGuiExport ViewProviderFillet : public ViewProviderPart
{
    PROPERTY_HEADER(PartGui::ViewProviderFillet);

public:
    ViewProviderFillet();
    virtual ~ViewProviderFillet();
    /** @name Edit methods */
    //@{
    void setupContextMenu(QMenu*, QObject*, const char*);
    std::vector<App::DocumentObject*> claimChildren() const;

protected:
    bool setEdit(int ModNum);
    void unsetEdit(int ModNum);
    //@}
};

class ViewProviderChamfer : public ViewProviderPart
{
    PROPERTY_HEADER(PartGui::ViewProviderChamfer);

public:
    /// constructor
    ViewProviderChamfer();
    /// destructor
    virtual ~ViewProviderChamfer();
    /** @name Edit methods */
    //@{
    void setupContextMenu(QMenu*, QObject*, const char*);
    std::vector<App::DocumentObject*> claimChildren() const;

protected:
    bool setEdit(int ModNum);
    void unsetEdit(int ModNum);
    //@}
};

class ViewProviderRevolution : public ViewProviderPart
{
    PROPERTY_HEADER(PartGui::ViewProviderRevolution);

public:
    /// constructor
    ViewProviderRevolution();
    /// destructor
    virtual ~ViewProviderRevolution();

    /// grouping handling 
    std::vector<App::DocumentObject*> claimChildren(void)const;
};

} // namespace PartGui


#endif // PARTGUI_VIEWPROVIDERMIRROR_H
