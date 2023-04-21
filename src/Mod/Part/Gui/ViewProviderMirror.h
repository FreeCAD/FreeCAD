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
    PROPERTY_HEADER_WITH_OVERRIDE(PartGui::ViewProviderMirror);

public:
    ViewProviderMirror();
    ~ViewProviderMirror() override;
    /** @name Edit methods */
    //@{
    void setupContextMenu(QMenu*, QObject*, const char*) override;
    std::vector<App::DocumentObject*> claimChildren() const override;
    bool onDelete(const std::vector<std::string> &) override;

protected:
    bool setEdit(int ModNum) override;
    void unsetEdit(int ModNum) override;
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
    PROPERTY_HEADER_WITH_OVERRIDE(PartGui::ViewProviderFillet);

public:
    ViewProviderFillet();
    ~ViewProviderFillet() override;
    /** @name Edit methods */
    //@{
    void setupContextMenu(QMenu*, QObject*, const char*) override;
    std::vector<App::DocumentObject*> claimChildren() const override;
    bool onDelete(const std::vector<std::string> &) override;

protected:
    void updateData(const App::Property*) override;
    bool setEdit(int ModNum) override;
    void unsetEdit(int ModNum) override;
    //@}
};

class ViewProviderChamfer : public ViewProviderPart
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartGui::ViewProviderChamfer);

public:
    /// constructor
    ViewProviderChamfer();
    /// destructor
    ~ViewProviderChamfer() override;
    /** @name Edit methods */
    //@{
    void setupContextMenu(QMenu*, QObject*, const char*) override;
    std::vector<App::DocumentObject*> claimChildren() const override;
    bool onDelete(const std::vector<std::string> &) override;

protected:
    void updateData(const App::Property*) override;
    bool setEdit(int ModNum) override;
    void unsetEdit(int ModNum) override;
    //@}
};

class ViewProviderRevolution : public ViewProviderPart
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartGui::ViewProviderRevolution);

public:
    /// constructor
    ViewProviderRevolution();
    /// destructor
    ~ViewProviderRevolution() override;

    /// grouping handling
    std::vector<App::DocumentObject*> claimChildren()const override;
    bool onDelete(const std::vector<std::string> &) override;
};

class ViewProviderLoft : public ViewProviderPart
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartGui::ViewProviderLoft);

public:
    /// constructor
    ViewProviderLoft();
    /// destructor
    ~ViewProviderLoft() override;

    /// grouping handling
    std::vector<App::DocumentObject*> claimChildren()const override;
    bool onDelete(const std::vector<std::string> &) override;
};

class ViewProviderSweep : public ViewProviderPart
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartGui::ViewProviderSweep);

public:
    /// constructor
    ViewProviderSweep();
    /// destructor
    ~ViewProviderSweep() override;

    /// grouping handling
    std::vector<App::DocumentObject*> claimChildren()const override;
    bool onDelete(const std::vector<std::string> &) override;
};

class ViewProviderOffset : public ViewProviderPart
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartGui::ViewProviderOffset);

public:
    /// constructor
    ViewProviderOffset();
    /// destructor
    ~ViewProviderOffset() override;

    /// grouping handling
    std::vector<App::DocumentObject*> claimChildren()const override;
    void setupContextMenu(QMenu*, QObject*, const char*) override;
    bool onDelete(const std::vector<std::string> &) override;

protected:
    bool setEdit(int ModNum) override;
    void unsetEdit(int ModNum) override;
};

class ViewProviderOffset2D : public ViewProviderOffset
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartGui::ViewProviderOffset2D);

public:
    ViewProviderOffset2D(){
        sPixmap = "Part_Offset2D";
    }
};

class ViewProviderThickness : public ViewProviderPart
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartGui::ViewProviderThickness);

public:
    /// constructor
    ViewProviderThickness();
    /// destructor
    ~ViewProviderThickness() override;

    /// grouping handling
    std::vector<App::DocumentObject*> claimChildren()const override;
    void setupContextMenu(QMenu*, QObject*, const char*) override;
    bool onDelete(const std::vector<std::string> &) override;

protected:
    bool setEdit(int ModNum) override;
    void unsetEdit(int ModNum) override;
};

class ViewProviderRefine : public ViewProviderPart
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartGui::ViewProviderRefine);

public:
    /// constructor
    ViewProviderRefine();
    /// destructor
    ~ViewProviderRefine() override;
};

class ViewProviderReverse : public ViewProviderPart
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartGui::ViewProviderReverse);

public:
    /// constructor
    ViewProviderReverse();
    /// destructor
    ~ViewProviderReverse() override;
};

} // namespace PartGui


#endif // PARTGUI_VIEWPROVIDERMIRROR_H
