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
    virtual ~ViewProviderMirror();
    /** @name Edit methods */
    //@{
    virtual void setupContextMenu(QMenu*, QObject*, const char*) override;
    virtual std::vector<App::DocumentObject*> claimChildren() const override;
    virtual bool onDelete(const std::vector<std::string> &) override;
    
protected:
    virtual bool setEdit(int ModNum) override;
    virtual void unsetEdit(int ModNum) override;
    //@}

    virtual void onDragStart(SoDragger *d) override;
    virtual void onDragFinish(SoDragger *d) override;
    virtual void onDragMotion(SoDragger *d) override;
    virtual Base::Matrix4D getDragOffset() override;

private:
    bool editing = false;
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
    bool onDelete(const std::vector<std::string> &);

protected:
    void updateData(const App::Property*);
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
    bool onDelete(const std::vector<std::string> &);

protected:
    void updateData(const App::Property*);
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
    bool onDelete(const std::vector<std::string> &);
};

class ViewProviderLoft : public ViewProviderPart
{
    PROPERTY_HEADER(PartGui::ViewProviderLoft);

public:
    /// constructor
    ViewProviderLoft();
    /// destructor
    virtual ~ViewProviderLoft();

    /// grouping handling 
    std::vector<App::DocumentObject*> claimChildren(void)const;
    bool onDelete(const std::vector<std::string> &);
};

class ViewProviderSweep : public ViewProviderPart
{
    PROPERTY_HEADER(PartGui::ViewProviderSweep);

public:
    /// constructor
    ViewProviderSweep();
    /// destructor
    virtual ~ViewProviderSweep();

    /// grouping handling 
    std::vector<App::DocumentObject*> claimChildren(void)const;
    bool onDelete(const std::vector<std::string> &);
};

class ViewProviderOffset : public ViewProviderPart
{
    PROPERTY_HEADER(PartGui::ViewProviderOffset);

public:
    /// constructor
    ViewProviderOffset();
    /// destructor
    virtual ~ViewProviderOffset();

    /// grouping handling 
    std::vector<App::DocumentObject*> claimChildren(void)const;
    void setupContextMenu(QMenu*, QObject*, const char*);
    bool onDelete(const std::vector<std::string> &);

protected:
    virtual bool setEdit(int ModNum);
    virtual void unsetEdit(int ModNum);
};

class ViewProviderOffset2D : public ViewProviderOffset
{
    PROPERTY_HEADER(PartGui::ViewProviderOffset2D);

public:
    ViewProviderOffset2D(){
        sPixmap = "Part_Offset2D";
    }
};

class ViewProviderThickness : public ViewProviderPart
{
    PROPERTY_HEADER(PartGui::ViewProviderThickness);

public:
    /// constructor
    ViewProviderThickness();
    /// destructor
    virtual ~ViewProviderThickness();

    /// grouping handling 
    std::vector<App::DocumentObject*> claimChildren(void)const;
    void setupContextMenu(QMenu*, QObject*, const char*);
    bool onDelete(const std::vector<std::string> &);

protected:
    virtual bool setEdit(int ModNum);
    virtual void unsetEdit(int ModNum);
};

class ViewProviderRefine : public ViewProviderPart
{
    PROPERTY_HEADER(PartGui::ViewProviderRefine);

public:
    /// constructor
    ViewProviderRefine();
    /// destructor
    virtual ~ViewProviderRefine();
};

class ViewProviderReverse : public ViewProviderPart
{
    PROPERTY_HEADER(PartGui::ViewProviderReverse);

public:
    /// constructor
    ViewProviderReverse();
    /// destructor
    virtual ~ViewProviderReverse();
};

} // namespace PartGui


#endif // PARTGUI_VIEWPROVIDERMIRROR_H
