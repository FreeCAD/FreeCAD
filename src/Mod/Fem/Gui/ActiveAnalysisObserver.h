/***************************************************************************
 *   Copyright (c) 2013 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef FEMGUI_ACTIVEANALYSISOBSERVER_H
#define FEMGUI_ACTIVEANALYSISOBSERVER_H

#include <App/DocumentObserver.h>
#include <Gui/Tree.h>

namespace Gui {
    class Document;
    class ViewProviderDocumentObject;
}

namespace Fem { class FemAnalysis; }

namespace FemGui {

class ActiveAnalysisObserver : public App::DocumentObserver
{
public:
    static ActiveAnalysisObserver* instance();

    void setActiveObject(Fem::FemAnalysis*);
    Fem::FemAnalysis* getActiveObject() const;
    bool hasActiveObject() const;
    void highlightActiveObject(const Gui::HighlightMode&, bool);

private:
    ActiveAnalysisObserver();
    ~ActiveAnalysisObserver();

    void slotDeletedDocument(const App::Document& Doc);
    void slotDeletedObject(const App::DocumentObject& Obj);

private:
    static ActiveAnalysisObserver* inst;
    Fem::FemAnalysis* activeObject;
    Gui::ViewProviderDocumentObject* activeView;
    Gui::Document* activeDocument;
};

} //namespace FemGui

#endif // FEMGUI_ACTIVEANALYSISOBSERVER_H
