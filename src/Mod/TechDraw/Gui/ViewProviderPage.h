/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2012 Luke Parry <l.parry@warwick.ac.uk>                 *
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


#ifndef DRAWINGGUI_VIEWPROVIDERPAGE_H
#define DRAWINGGUI_VIEWPROVIDERPAGE_H

#include <QPointer>
#include <Gui/ViewProviderFeature.h>
#include <Gui/ViewProviderDocumentObjectGroup.h>
#include <Gui/Selection.h>

namespace TechDraw{
    class DrawPage;
}

namespace TechDrawGui {

class MDIViewPage;

class TechDrawGuiExport ViewProviderPage : public Gui::ViewProviderDocumentObject,
                                                 public Gui::SelectionObserver
{
    PROPERTY_HEADER(TechDrawGui::ViewProviderPage);

public:
    /// constructor
    ViewProviderPage();
    /// destructor
    virtual ~ViewProviderPage();

    //App::PropertyFloat         HintScale;
    //App::PropertyFloat         HintOffsetX;
    //App::PropertyFloat         HintOffsetY;

    virtual void attach(App::DocumentObject *);
    virtual void setDisplayMode(const char* ModeName);
    virtual bool useNewSelectionModel(void) const {return false;}
    /// returns a list of all possible modes
    virtual std::vector<std::string> getDisplayModes(void) const;
    /// Hides the view provider
    virtual void hide(void);
    /// Shows the view provider
    virtual void show(void);
    virtual bool isShow(void) const;

    void onSelectionChanged(const Gui::SelectionChanges& msg);

    /// Claim all the views for the page
    std::vector<App::DocumentObject*> claimChildren(void) const;

    /// Is called by the tree if the user double click on the object
    virtual bool doubleClicked(void);
    void setupContextMenu(QMenu*, QObject*, const char*);
    virtual bool onDelete(const std::vector<std::string> &);
    virtual void onChanged(const App::Property *prop);
    virtual void updateData(const App::Property* prop);
    virtual void startRestoring();
    virtual void finishRestoring();
    bool isRestoring(void) {return !m_docReady;}

    TechDraw::DrawPage* getDrawPage() const;
    void onGuiRepaint(const TechDraw::DrawPage* dp); 
    typedef boost::signals::scoped_connection Connection;
    Connection connectGuiRepaint;

    void unsetEdit(int ModNum);
    MDIViewPage* getMDIViewPage();
    bool showMDIViewPage();

protected:
    bool setEdit(int ModNum);

private:
    QPointer<MDIViewPage> m_mdiView;
    bool m_docReady;
};

} // namespace TechDrawGui


#endif // DRAWINGGUI_VIEWPROVIDERPAGE_H
